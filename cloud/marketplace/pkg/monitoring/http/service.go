package monitoring

import (
	"context"
	"encoding/json"
	"net/http"
	"time"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
)

type Service struct {
	config struct {
		listenEndpoint string
	}

	statusChecker   *status.StatusCollector
	metricsRegestry *solomon.Registry
}

type Config struct {
	ListenEndpoint string

	StatusPollInterval time.Duration
}

func NewService(config Config, regestry *solomon.Registry, statusCheckers ...status.StatusChecker) *Service {
	service := &Service{
		metricsRegestry: regestry,
		statusChecker: status.NewStatusChecker(
			status.Config{PollInterval: config.StatusPollInterval},
			statusCheckers...,
		),
	}

	service.config.listenEndpoint = config.ListenEndpoint

	return service
}

func (s *Service) Run(runCtx context.Context) error {
	scoppedLogger := ctxtools.Logger(runCtx)
	scoppedLogger.Debug("running http monitoring service", log.String("endpoint", s.config.listenEndpoint))

	server := http.Server{
		Addr:    s.config.listenEndpoint,
		Handler: initRouter(scoppedLogger.Logger().WithName("mon"), s.metricsRegestry, s.statusChecker),
	}

	group, ctx := errgroup.WithContext(runCtx)

	group.Go(func() error {
		return server.ListenAndServe()
	})

	group.Go(func() error {
		return s.statusChecker.Run(ctx)
	})

	group.Go(func() error {
		<-ctx.Done()

		scoppedLogger.Info("shutting down moitoring http server")
		if err := server.Shutdown(ctx); err != nil {
			return err
		}

		return ctx.Err()
	})

	return group.Wait()
}

func sendJSONResponse(ctx context.Context, w http.ResponseWriter, status int, j interface{}) {
	logger := ctxtools.Logger(ctx)

	w.Header().Add("Content-type", "application/json")
	w.WriteHeader(status)

	if err := json.NewEncoder(w).Encode(j); err != nil {
		logger.Error("failed to encode json", log.Error(err))
	}
}

func sendJSONResponseSuccess(ctx context.Context, w http.ResponseWriter, j interface{}) {
	sendJSONResponse(ctx, w, http.StatusOK, j)
}
