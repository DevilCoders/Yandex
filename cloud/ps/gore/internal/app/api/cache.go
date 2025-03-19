package api

import (
	"net/http"
	"net/http/httptest"
	"time"
)

func (srv *Server) cache(timeout time.Duration) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			content, _ := srv.Config.Mongo.CacheGet(r.RequestURI)
			if content != nil {
				_, _ = w.Write(content)
			} else {
				rec := httptest.NewRecorder()
				next.ServeHTTP(rec, r)
				data := rec.Body.Bytes()
				_, _ = w.Write(data)
				if err := srv.Config.Mongo.CacheStore(r.RequestURI, data, timeout); err != nil {
					panic(err)
				}
			}
		}
		return http.HandlerFunc(fn)
	}
}
