package http

// NOTE: HTTP service is deprecated, implemented for testing/debugging purposes.

import (
	"context"
	"encoding/json"
	"net/http"

	"github.com/opentracing/opentracing-go"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"
)

type Service struct {
	*env.Env

	router http.Handler
	config Config
}

type Config struct {
	ListenEndpoint string
}

func NewService(env *env.Env, config Config) *Service {
	srv := &Service{
		Env:    env,
		config: config,
	}

	logging.Logger().Debug("initializing http service")

	srv.router = srv.initRouter()

	return srv
}

func (s *Service) Run(runCtx context.Context) error {
	scoppedLogger := ctxtools.Logger(runCtx)
	scoppedLogger.Debug("running http service", log.String("endpoint", s.config.ListenEndpoint))

	group, ctx := errgroup.WithContext(runCtx)

	server := http.Server{
		Addr:    s.config.ListenEndpoint,
		Handler: s.router,
	}

	group.Go(func() error {
		return server.ListenAndServe()
	})

	group.Go(func() error {
		<-ctx.Done()

		scoppedLogger.Info("shutting down http server")
		if err := server.Shutdown(ctx); err != nil {
			return err
		}

		return ctx.Err()
	})

	return group.Wait()
}

// logAndSendInternalError TODO: make depreacted, rewrite error handler.
func (s *Service) logAndSendInternalError(w http.ResponseWriter, err error) {
	logging.Logger().Error("http: failed to send response", log.Error(err))
	http.Error(w, "Internal Error", http.StatusInternalServerError)
}

func (s *Service) sendJSONResponseOk(ctx context.Context, w http.ResponseWriter, result interface{}) error {
	return s.sendJSONResponse(ctx, w, http.StatusOK, result)
}

func (s *Service) sendJSONResponse(ctx context.Context, w http.ResponseWriter, status int, result interface{}) error {
	span, spanCtx := tracing.Start(ctx, "SendingResponse", opentracing.Tag{Key: "http_status", Value: status})
	defer span.Finish()

	ctxtools.Logger(spanCtx).Debug("http: sending response", log.Int("http_status", status))

	w.Header().Set("Content-type", "application/json")
	if requestID := ctxtools.GetRequestIDOrEmpty(spanCtx); requestID != "" {
		w.Header().Add("X-Request-ID", requestID)
	}

	w.WriteHeader(status)

	if err := json.NewEncoder(w).Encode(result); err != nil {
		s.logAndSendInternalError(w, err)
		return err
	}

	return nil
}

func (s *Service) sendAPIError(ctx context.Context, w http.ResponseWriter, apiErr *apiError) {
	if err := s.sendJSONResponse(ctx, w, apiErr.status, apiErr); err != nil {
		logging.Logger().Error("failed to send api error", log.Error(err))
		return
	}

	logging.Logger().Debug("api error send without errors")
}
