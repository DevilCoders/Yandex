package compress

import (
	"bytes"
	"compress/gzip"
	"io"
	"net/http"
	"strconv"

	"a.yandex-team.ru/library/go/httputil/headers"
)

const (
	// gzipMinSize is the minimum size starting of we enable gzip compression.
	// 1500 bytes is the MTU size for the internet since that is the largest size allowed at the network layer.
	// If you take a file that is 1300 bytes and compress it to 800 bytes, it’s still transmitted in that same 1500 byte packet regardless, so you’ve gained nothing.
	// That being the case, you should restrict the gzip compression to files with a size greater than a single packet, 1400 bytes (1.4KB) is a safe value.
	gzipMinSize = 1400
)

var _ Compressor = new(GzipCompressor)

// GzipCompressor compresses bytes written to http.Response body in efficient way
type GzipCompressor struct {
	rw   http.ResponseWriter
	gw   *gzip.Writer
	buf  *bytes.Buffer
	bufc []byte
	size int
}

// NewGzipCompressor returns new GzipCompressor instance
func NewGzipCompressor(level int) *GzipCompressor {
	if level < gzip.DefaultCompression {
		level = gzip.DefaultCompression
	}
	if level > gzip.BestCompression {
		level = gzip.BestCompression
	}

	buf := new(bytes.Buffer)
	gw, _ := gzip.NewWriterLevel(buf, level)
	return &GzipCompressor{
		gw:  gw,
		buf: buf,
	}
}

// Setup ensures reusable compressor is properly instantiated.
// This method must be called before any write into compressor that has been newly fetched from pool
func (g *GzipCompressor) Setup(w http.ResponseWriter) {
	g.rw = w
}

// Header proxies HTTP headers from underlying http.ResponseWriter
func (g *GzipCompressor) Header() http.Header {
	return g.rw.Header()
}

// WriteHeader writes HTTP header to underlying http.ResponseWriter
func (g *GzipCompressor) WriteHeader(statusCode int) {
	g.rw.WriteHeader(statusCode)
}

// Writes compressable data to http.ResponseWriter in efficient way.
// Result data may be compressed at any time or stay uncompressed at all
func (g *GzipCompressor) Write(b []byte) (int, error) {
	// start with raw buffer by default
	var target io.Writer = g.buf
	if g.compressed() {
		target = g.gw
	}

	// if given chunk or cumulative uncompressed payload
	// is big enough to compress - copy everything from buffer
	// to gzip writer and mark payload as compressed
	if !g.compressed() && g.size+len(b) >= gzipMinSize {
		if g.buf.Len() > 0 {
			// allocate buffer on first call only
			if len(g.bufc) == 0 {
				g.bufc = make([]byte, gzipMinSize)
			}
			copied := copy(g.bufc, g.buf.Bytes())
			g.buf.Reset()
			_, err := g.gw.Write(g.bufc[:copied])
			if err != nil {
				return 0, err
			}
		}
		// switch to gzip writer immediately
		target = g.gw
	}

	// write bytes to target writer
	written, err := target.Write(b)
	if err != nil {
		return 0, err
	}

	g.size += written
	return written, nil
}

// Close flushes any uncompressed data if exists, sets proper HTTP headers
// and resets compressor state so it can be safely returned into pool.
// This method must be call before returning object to pool or returning response to client.
func (g *GzipCompressor) Close() (err error) {
	// cleanup state to be ready for reuse from pool
	defer func() {
		g.gw.Reset(g.buf)
		g.buf.Reset()
		g.size = 0
	}()

	// flush compressed data
	if g.compressed() {
		// ensure all data has been written
		err = g.gw.Close()

		// set appropriate headers
		g.rw.Header().Set(headers.ContentEncodingKey, string(headers.EncodingGZIP))
	}

	g.rw.Header().Set(headers.ContentLength, strconv.Itoa(g.buf.Len()))
	_, err = g.buf.WriteTo(g.rw)
	return err
}

// compressed checks if data must be or have already been compressed
func (g *GzipCompressor) compressed() bool {
	return g.size >= gzipMinSize
}
