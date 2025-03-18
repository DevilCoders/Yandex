package httpmetrics_test

import (
	"net/http"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/httputil/middleware/httpmetrics"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
)

func Example_chi() {
	// Create solomon registry.
	registry := solomon.NewRegistry(solomon.NewRegistryOpts())

	// Create HTTP router.
	r := chi.NewMux()

	// Collect http metrics.
	r.Use(httpmetrics.New(registry.WithPrefix("http"), httpmetrics.WithPathEndpoint()))

	// Expose metrics to solomon fetcher.
	r.Handle("/solomon", httppuller.NewHandler(registry))
}

func Example_stdlib() {
	// Create solomon registry.
	registry := solomon.NewRegistry(solomon.NewRegistryOpts())

	middleware := httpmetrics.New(registry.WithPrefix("http"), httpmetrics.WithPathEndpoint())

	myHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(200)
		_, _ = w.Write([]byte("Hello"))
	})

	http.Handle("/endpoint", middleware(myHandler))

	// Expose metrics to solomon fetcher.
	http.Handle("/solomon", httppuller.NewHandler(registry))
}
