package app

import (
	"context"
	"fmt"
	"net/http"
	"time"

	"a.yandex-team.ru/library/go/core/log"
)

// run start server and shutdown it when context canceled
func runStatServer(ctx context.Context, logger log.Logger, port int, jsGetter func() ([]byte, error)) {
	statHandle := func(rw http.ResponseWriter, req *http.Request) {
		bytes, err := jsGetter()
		if err != nil {
			logger.Warnf("stat marshal error: %s", err)
			http.Error(rw, err.Error(), http.StatusInternalServerError)
			return
		}

		rw.Header().Set("Content-Type", "application/json; charset=utf-8")
		if _, err := rw.Write(bytes); err != nil {
			logger.Warnf("stat write error: %s", err)
		}
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/", statHandle)

	statSrv := &http.Server{
		Addr:    fmt.Sprintf("127.0.0.1:%d", port),
		Handler: mux,
	}
	logger.Infof("starting stat server at %s", statSrv.Addr)
	go func() {
		err := statSrv.ListenAndServe()
		if err != http.ErrServerClosed {
			logger.Errorf("stat server exit unexpectedly: %s", err)
		}
	}()

	<-ctx.Done()

	shCtx := context.Background()
	shCtx, cancel := context.WithTimeout(shCtx, time.Second)
	defer cancel()
	if err := statSrv.Shutdown(shCtx); err != nil {
		logger.Warnf("stat server shutdown with error: %s", err)
	}
}
