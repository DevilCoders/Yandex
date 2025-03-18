package main

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/vanity/internal/logger"
)

func main() {
	logger.Setup(log.InfoLevel)

	if err := run(); err != nil {
		logger.Log.Fatal("unexpected error", log.Error(err))
		os.Exit(1)
	}
}

func run() error {
	srv := &http.Server{
		Addr:    flagRunAddr,
		Handler: newRouter(),
	}

	errChan := make(chan error, 1)
	go func() {
		logger.Log.Info("starting HTTP server", log.String("addr", flagRunAddr))
		if err := srv.ListenAndServe(); err != nil {
			errChan <- fmt.Errorf("cannot run HTTP server: %w", err)
		}
	}()

	stop := make(chan os.Signal, 1)
	signal.Notify(stop, syscall.SIGINT, syscall.SIGTERM)

	select {
	case sig := <-stop:
		logger.Log.Info("shutting down gracefully", log.String("signal", sig.String()))

		ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()

		return srv.Shutdown(ctx)
	case err := <-errChan:
		return err
	}
}

func newRouter() http.Handler {
	r := chi.NewRouter()

	r.Get("/", handleIndex)
	r.Get("/*", handleGoGet)
	r.Get("/ping", handlePing)

	return r
}
