package sender

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
	"os"

	"a.yandex-team.ru/library/go/core/log"
)

type bufStore struct {
	queue chan *senderBuffer
}

func newBufStore(size int) *bufStore {
	return &bufStore{
		queue: make(chan *senderBuffer, size),
	}
}

// senderBuffer is NOT thread safe sender buffer!
type senderBuffer struct {
	maxSize int
	buf     *bytes.Buffer
	_len    int
}

func (b *senderBuffer) add(metric []byte) {
	b.buf.Write(metric)
	curSize := b.size()
	if b.maxSize < curSize {
		metricsBufsSize.Add(float64(curSize - b.maxSize))
		b.maxSize = curSize
	}
	b._len++
}
func (b *senderBuffer) len() int {
	return b._len
}
func (b *senderBuffer) size() int {
	return b.buf.Len()
}
func (b *senderBuffer) reset() {
	b.buf.Reset()
	b._len = 0
}
func (b *senderBuffer) bytes() []byte {
	return b.buf.Bytes()
}

func (s *bufStore) getBuffer() *senderBuffer {
	select {
	case buf := <-s.queue:
		return buf
	default:
		logger.Info("Make new buffer")
		metricsBufCount.Inc()
		return &senderBuffer{buf: new(bytes.Buffer)}
	}
}
func (s *bufStore) returnToQueue(buf *senderBuffer) {
	if buf == nil {
		return
	}
	buf.reset()
	select {
	case s.queue <- buf:
	default:
		metricsBufCount.Dec()
		metricsBufsSize.Add(-float64(buf.maxSize))
		logger.Info("Throw out extra buffer")
	}
}

func (s *bufStore) bufferFromFile(name string) (*senderBuffer, error) {
	bufLog := log.With(logger, log.String("file", name))
	file, err := os.Open(name)
	if err != nil {
		return nil, fmt.Errorf("can not open %s: %w", name, err)
	}
	defer func() { _ = file.Close() }()
	reader := bufio.NewReader(file)
	buf := s.getBuffer()
OuterLoop:
	for {
		metric, err := reader.ReadBytes('\n')
		if err != nil {
			switch err {
			case io.EOF:
				break OuterLoop
			default:
				bufLog.Errorf("Got an error: %v", err)
				break OuterLoop
			}
		}

		if logTrace {
			bufLog.Debugf("add metric to buf: %q", string(metric))
		}
		buf.add(metric)
	}
	bufLog.Debugf("Loaded buffer metrics=%d, size=%d", buf.len(), buf.size())
	return buf, nil
}
