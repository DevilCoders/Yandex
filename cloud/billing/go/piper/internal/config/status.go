package config

import (
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
)

type statusContainer struct {
	statusServerOnce initSync
	statusServer     *http.Server
}

func (c *Container) GetHTTPStatusServer() (HTTPServer, error) {
	if err := c.initStatusServer(); err != nil {
		return nil, err
	}

	if c.statusServer == nil {
		return c.dummy(), nil
	}
	return c.statusServer, nil
}

func (c *Container) initStatusServer() error {
	return c.statusServerOnce.Do(c.makeHTTPStatusServer)
}

func (c *Container) makeHTTPStatusServer() error {
	c.initializedMetrics()

	if c.initFailed() {
		return c.initError
	}
	config, err := c.GetStatusServerConfig()
	if err != nil {
		return err
	}
	if !config.Enabled.Bool() {
		return nil
	}
	c.statusServer = &http.Server{
		Addr:        fmt.Sprintf(":%d", config.Port),
		ReadTimeout: time.Millisecond * 100,
		IdleTimeout: time.Second * 30,
		Handler:     c.makeStatusHandler(),
	}
	c.statesMgr.Add("status-server", c.statusServer)
	return nil
}

func (c *Container) makeStatusHandler() http.Handler {
	mux := chi.NewMux()
	mux.Mount("/debug", middleware.Profiler())

	router := mux.With(
		middleware.StripSlashes,
		middleware.GetHead,
		middleware.Recoverer,
	)
	router.HandleFunc("/", handleRoot)
	router.Handle("/metrics", metrics.GetHandler())

	router.Route("/ping", c.statesMgr.RouteHealthCheck)
	router.Route("/juggler", c.statesMgr.RouteJugglerCheck)
	router.Route("/stats", c.statesMgr.RouteStats)

	return router
}

func handleRoot(w http.ResponseWriter, r *http.Request) {
	type routes struct {
		Routes []string
	}

	data, _ := json.Marshal(routes{[]string{
		"/metrics",
		"/ping",
		"/juggler",
		"/stats",
		"/debug/pprof",
	}})
	w.Header().Set("Content-Type", "application/json")
	_, _ = w.Write(data)
}
