package compress

import (
	"io"
	"net/http"
	"sync"

	"a.yandex-team.ru/library/go/httputil/headers"
)

// Compressor is an interface for compression HTTP wrapper
type Compressor interface {
	// Close method must cleanup any state so Compressor is ready to be reused from pool
	io.Closer
	http.ResponseWriter

	// Setup properly initializes compressor.
	// It will be called before passing to next handler
	Setup(w http.ResponseWriter)
}

// compressHandler holds pools of compressors for corresponding accept encodings
type compressHandler struct {
	pools map[string]*sync.Pool
}

// NewHandler returns new middleware handler
func NewHandler(level int) func(http.Handler) http.Handler {
	ch := &compressHandler{
		pools: map[string]*sync.Pool{
			"gzip": &sync.Pool{
				New: func() interface{} { return NewGzipCompressor(level) },
			},
		},
	}

	return ch.wrap
}

// wrap is an http.Handler wrapper
func (c *compressHandler) wrap(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		ae := r.Header.Get(headers.AcceptEncodingKey)
		acceptables, err := headers.ParseAcceptEncoding(ae)

		var compressor Compressor
		if err == nil && len(acceptables) > 0 {
			for compressorName, pool := range c.pools {
				if acceptables.IsAcceptable(headers.ContentEncoding(compressorName)) {
					compressor = pool.Get().(Compressor)
					defer func() { pool.Put(compressor) }()
					break
				}
			}
		}

		cw := w
		if compressor != nil {
			compressor.Setup(w)
			defer compressor.Close()
			cw = compressor
		}

		next.ServeHTTP(cw, r)
	})
}
