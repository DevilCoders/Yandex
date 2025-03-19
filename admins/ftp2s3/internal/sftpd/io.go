package sftpd

import (
	"io"
	"os"
	"sync"
	"time"

	"github.com/pkg/sftp"

	s3driver "a.yandex-team.ru/admins/ftp2s3/internal/s3-driver"
	"a.yandex-team.ru/admins/ftp2s3/internal/stats"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var ErrOutOfOrder = xerrors.New("S3FileStream: non-linearizable access")

type queue struct {
	sync.Mutex
	firstID  int64
	err      error
	inFlight map[int64]chan error
}

func (q *queue) push(current, next int64) (first bool, err error) {
	q.Lock()
	if q.err != nil {
		q.Unlock()
		return false, q.err
	}
	if q.inFlight == nil {
		q.inFlight = map[int64]chan error{}
		q.inFlight[next] = make(chan error, 1)
		q.firstID = current
		q.Unlock()
		first = true
	} else {
		if q.firstID > current {
			q.Unlock()
			return false, ErrOutOfOrder
		}

		ch, ok := q.inFlight[current]
		if !ok {
			ch = make(chan error, 1)
			q.inFlight[current] = ch
		}
		q.Unlock()
		err = <-ch
	}
	return
}

func (q *queue) pop(id int64) {
	q.Lock()
	defer q.Unlock()
	if v, ok := q.inFlight[id]; ok {
		close(v)
		delete(q.inFlight, id)
	}
}

func (q *queue) releaseNext(next int64, err error) {
	q.Lock()
	defer q.Unlock()
	if err != nil {
		if q.err == nil {
			q.err = err
		}
		q.close(err, false /*lock*/)
		return
	}
	ch, ok := q.inFlight[next]
	if !ok {
		ch = make(chan error, 1)
		q.inFlight[next] = ch
	}
	ch <- nil
}

func (q *queue) close(cause error, lock bool) {
	if lock {
		q.Lock()
		defer q.Unlock()
	}
	if q.err == nil {
		q.err = cause
	}
	keys := make([]int64, 0, len(q.inFlight))
	for k, v := range q.inFlight {
		keys = append(keys, k)
		select {
		case v <- cause:
		default:
		}
		close(v)
	}
	for _, k := range keys {
		delete(q.inFlight, k)
	}
}

type writerAt struct {
	wQueue *queue
	f      *s3driver.S3FileDestination
	path   string
	stats  *stats.SFTPIOStats
}

func (h *sftpHandler) newWriterAt(path string, pflags sftp.FileOpenFlags) (io.WriterAt, error) {
	m := h.metrics.WithPrefix("write")

	flags := os.O_WRONLY
	if pflags.Append {
		flags = flags | os.O_APPEND
	}

	f, err := s3driver.NewS3FileDestination(h.driver, path, flags, m.WithPrefix("s3"))
	if err != nil {
		return nil, err
	}
	stats := stats.GetSFTPIOStats(m)
	stats.Open.Inc()
	return &writerAt{
		wQueue: new(queue),
		f:      f,
		path:   path,
		stats:  stats,
	}, nil
}

// WriteAt implementation of io.WriterAt
func (writer *writerAt) WriteAt(buffer []byte, offset int64) (int, error) {
	log.Debugf("WriteAt=%d bufLen=%d", offset, len(buffer))
	nextOffset := offset + int64(len(buffer))

	writer.stats.Enqueue.Inc()
	push := time.Now()
	first, err := writer.wQueue.push(offset, nextOffset)
	writer.stats.EnqueueHist.RecordDuration(time.Since(push))

	if xerrors.Is(err, ErrOutOfOrder) {
		writer.stats.OutOfOrder.Inc()
	}

	if first && err == nil {
		_, err = writer.f.Seek(offset, io.SeekStart)
	}

	var tmp int
	var n int
	for err == nil && len(buffer) > 0 {
		buffer = buffer[tmp:]
		tmp, err = writer.f.Write(buffer)
		n += tmp
	}
	writer.wQueue.releaseNext(nextOffset, err)
	log.Debugf("WriteAt=%d n=%d err=%v complete", offset, n, err)
	if err != nil {
		log.Errorf("WriteAt %s off=%d n=%d, err=%v", writer.path, offset, n, err)
	}
	return n, err
}

func (writer *writerAt) Close() error {
	writer.stats.Close.Inc()
	writer.wQueue.close(xerrors.New("sftp.WriteAt: stream closed"), true /*lock*/)
	return writer.f.Close()
}

type readerAt struct {
	rQueue *queue
	f      *s3driver.S3FileSource
	size   int64
	path   string
	stats  *stats.SFTPIOStats
}

func (h *sftpHandler) newReaderAt(path string) (io.ReaderAt, error) {
	m := h.metrics.WithPrefix("read")

	f, err := s3driver.NewS3FileSource(h.driver, path, m.WithPrefix("s3"))
	if err != nil {
		return nil, err
	}
	stat, err := h.driver.GetFileInfo(path)
	if err != nil {
		return nil, err
	}
	stats := stats.GetSFTPIOStats(m)
	stats.Open.Inc()

	return &readerAt{
		rQueue: new(queue),
		f:      f,
		size:   stat.Size(),
		path:   path,
		stats:  stats,
	}, nil
}

// ReadAt implement io.ReaderAt interface
func (reader *readerAt) ReadAt(buffer []byte, offset int64) (int, error) {
	if offset >= reader.size {
		log.Debugf("ReadAt=%d >= size=%d err=EOF", offset, reader.size)
		reader.rQueue.pop(offset)
		return 0, io.EOF
	}
	bufLen := len(buffer)
	log.Debugf("ReadAt=%d bufLen=%d", offset, bufLen)

	nextOffset := offset + int64(bufLen)

	reader.stats.Enqueue.Inc()
	push := time.Now()
	first, err := reader.rQueue.push(offset, nextOffset)
	reader.stats.EnqueueHist.RecordDuration(time.Since(push))

	if xerrors.Is(err, ErrOutOfOrder) {
		return reader.outOfOrderRead(buffer, offset)
	}
	if first && err == nil {
		_, err = reader.f.Seek(offset, io.SeekStart)
	}

	var n, tmp int
	for err == nil && n < bufLen {
		// TODO Read timeout
		tmp, err = reader.f.Read(buffer[n:])
		n += tmp
	}
	reader.rQueue.releaseNext(nextOffset, err)
	log.Debugf("ReadAt=%d n=%d, err=%v complete", offset, n, err)
	if err != nil {
		log.Errorf("ReadAt %s off=%d n=%d, err=%v", reader.path, offset, n, err)
	}
	return n, err
}

// If we are out of luck and the first reading package
// wandered somewhere on the network too long
func (reader *readerAt) outOfOrderRead(buffer []byte, offset int64) (int, error) {
	reader.stats.OutOfOrder.Inc()
	log.Debugf("outOfOrderRead %s off=%d", reader.path, offset)
	n, err := reader.f.RangeRead(buffer, offset)
	if err != nil {
		reader.rQueue.close(err, true /*lock*/)
	}
	return n, err
}

func (reader *readerAt) Close() error {
	reader.stats.Close.Inc()
	reader.rQueue.close(xerrors.New("sftp.ReaderAt: stream closed"), true /*lock*/)
	return reader.f.Close()
}
