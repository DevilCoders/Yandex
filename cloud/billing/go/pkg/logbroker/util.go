package logbroker

import (
	"bytes"
	"compress/gzip"
	"errors"
	"fmt"
	"io"
	"runtime"
	"sync"

	"github.com/klauspost/compress/zstd"

	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

// Locking helpers
type rlocker interface {
	RLock()
	RUnlock()
}

type unlocker interface {
	Unlock()
}

type runlocker func()

func (u runlocker) Unlock() {
	u()
}

func lock(l sync.Locker) unlocker {
	l.Lock()
	return l
}

func rlock(l rlocker) unlocker {
	l.RLock()
	return runlocker(l.RUnlock)
}

// sourceKey is helper type to store SourceID in terms of service.
type sourceKey struct {
	topic     string
	partition uint32
}

func (s sourceKey) String() string {
	return fmt.Sprintf("%s:%d", s.topic, s.partition)
}

func source(topic string, partition uint32) sourceKey {
	return sourceKey{topic: topic, partition: partition}
}

func writeWithLineEnd(w *bytes.Buffer, data []byte) {
	_, _ = w.Write(data)
	if data[len(data)-1] != '\n' {
		_ = w.WriteByte('\n')
	}
}

func readAll(b *bytes.Buffer) (result []byte) {
	return append(result, b.Bytes()...)
}

func castMessages(inp []persqueue.ReadMessage) []lbtypes.ReadMessage {
	result := make([]lbtypes.ReadMessage, len(inp))
	for i, m := range inp {
		var r lbtypes.DataReader
		switch m.Codec {
		case persqueue.Raw:
			rr := rawReader{}
			rr.reader.Reset(m.Data)
			r = &rr
		case persqueue.Gzip:
			rr := gzipReader{}
			rr.reader.Reset(m.Data)
			r = &rr
		case persqueue.Zstd:
			rr := zstdReader{}
			rr.reader.Reset(m.Data)
			r = &rr
		case persqueue.Lzop:
			panic("codec lzop is unsupported")
		default:
			panic(fmt.Sprintf("codec %d is unsupported", m.Codec))
		}

		result[i] = lbtypes.ReadMessage{
			Offset:     m.Offset,
			SeqNo:      m.SeqNo,
			SourceID:   m.SourceID,
			CreateTime: m.CreateTime,
			WriteTime:  m.WriteTime,
			IP:         m.IP,
			DataReader: r,
		}
	}
	return result
}

var (
	writeBuffers buffersPool
	gzipReaders  gzipReadersPool
	zstdReaders  zstdReadersPool
)

type buffersPool struct {
	pool sync.Pool
}

func (p *buffersPool) Get() *bytes.Buffer {
	pr := p.pool.Get()
	if pr == nil {
		return &bytes.Buffer{}
	}
	return pr.(*bytes.Buffer)
}

func (p *buffersPool) Put(b *bytes.Buffer) {
	b.Reset()
	p.pool.Put(b)
}

type gzipReadersPool struct {
	pool sync.Pool
}

func (p *gzipReadersPool) Get() *gzip.Reader {
	pr := p.pool.Get()
	if pr == nil {
		gr := &gzip.Reader{}
		runtime.SetFinalizer(gr, func(d *gzip.Reader) { _ = d.Close() })
		return gr
	}
	return pr.(*gzip.Reader)
}

func (p *gzipReadersPool) Put(r *gzip.Reader) {
	_ = r.Reset(eofReader{})
	p.pool.Put(r)
}

type zstdReadersPool struct {
	pool sync.Pool
}

func (p *zstdReadersPool) Get() *zstd.Decoder {
	pr := p.pool.Get()
	if pr == nil {
		r, err := zstd.NewReader(nil, zstd.WithDecoderLowmem(true))
		if err != nil {
			panic(err)
		}
		runtime.SetFinalizer(r, func(d *zstd.Decoder) { d.Close() })
		return r
	}
	return pr.(*zstd.Decoder)
}

func (p *zstdReadersPool) Put(r *zstd.Decoder) {
	_ = r.Reset(eofReader{})
	p.pool.Put(r)
}

type rawReader struct {
	reader bytes.Reader
}

func (r *rawReader) RawSize() int {
	return int(r.reader.Size())
}

func (r *rawReader) Close() error {
	r.reader.Reset(nil)
	return nil
}

func (r *rawReader) Read(b []byte) (n int, err error) {
	n, err = r.reader.Read(b)
	if errors.Is(err, io.EOF) {
		_ = r.Close()
	}
	return n, err
}

type gzipReader struct {
	rawReader
	gz *gzip.Reader
}

func (r *gzipReader) Close() error {
	_ = r.rawReader.Close()
	if r.gz == nil {
		return nil
	}
	gr := r.gz
	r.gz = nil
	gzipReaders.Put(gr)
	return nil
}

func (r *gzipReader) Read(b []byte) (n int, err error) {
	switch {
	case r.reader.Len() == 0: // read was done
		return r.reader.Read(b)
	case r.gz == nil:
		r.gz = gzipReaders.Get()
		if err := r.gz.Reset(&r.reader); err != nil {
			return 0, err
		}
	}
	n, err = r.gz.Read(b)
	if errors.Is(err, io.EOF) {
		_ = r.Close()
	}
	return n, err
}

type zstdReader struct {
	rawReader
	dec *zstd.Decoder
}

func (r *zstdReader) Close() error {
	_ = r.rawReader.Close()
	if r.dec == nil {
		return nil
	}
	zr := r.dec
	r.dec = nil
	zstdReaders.Put(zr)
	return nil
}

func (r *zstdReader) Read(b []byte) (n int, err error) {
	switch {
	case r.reader.Len() == 0: // read was done
		return r.reader.Read(b)
	case r.dec == nil:
		r.dec = zstdReaders.Get()
		if err := r.dec.Reset(&r.reader); err != nil {
			return 0, err
		}
	}
	n, err = r.dec.Read(b)
	if errors.Is(err, io.EOF) {
		_ = r.Close()
	}
	return n, err
}

type eofReader struct{}

func (eofReader) Read([]byte) (int, error) {
	return 0, io.EOF
}

func (eofReader) ReadByte() (byte, error) {
	return 0, io.EOF
}
