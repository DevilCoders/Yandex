package compress_test

import (
	"net/http"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/library/go/httputil/middleware/compress"
)

func Example_chi() {
	// create HTTP router
	r := chi.NewRouter()
	// apply compress middleware to all handlers
	r.Use(compress.NewHandler(5))

	// if request contains Accept-Encoding with supported encoder response body may be compressed
	r.Handle("/endpoint", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("Lorem ipsum dolor sit amet..."))
	}))
}

func Example_stdlib() {
	middleware := compress.NewHandler(5)

	myHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("Lorem ipsum dolor sit amet..."))
	})

	http.Handle("/endpoint", middleware(myHandler))
}
