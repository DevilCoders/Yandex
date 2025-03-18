package application

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"

	"github.com/go-chi/chi/v5"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"

	"a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware"
	xhttp "a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware/http"
	xmetrics "a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware/metrics"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/adminhandler"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/doc"
	saltpb "a.yandex-team.ru/cdn/cloud_api/proto/saltapi"
	userapipb "a.yandex-team.ru/cdn/cloud_api/proto/userapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/httputil/middleware/httpmetrics"
	solomonhttp "a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	"a.yandex-team.ru/strm/common/go/pkg/xnet/xhttp/xhandlers"
	"a.yandex-team.ru/strm/common/go/pkg/xpprof"
)

type httpServer struct {
	logger log.Logger
	server *http.Server
}

func (a *App) newHTTPServer(
	ctx context.Context,
	saltPB *saltPB,
	userapiPB *userapiPB,
	adminHandler *adminhandler.Handler,
) (*httpServer, error) {
	mux := chi.NewMux()
	mux.Use(xhttp.RequestID(xmiddleware.RequestIDHeaderKey))
	mux.Use(xhttp.Name)
	mux.Use(xhttp.AccessLog(a.Logger))
	mux.Use(httpmetrics.New(
		a.MetricsRegistry.WithPrefix("http"),
		httpmetrics.WithDurationBuckets(xmetrics.DefaultDurationBuckets),
		httpmetrics.WithEndpointKey(func(r *http.Request) string {
			return xmiddleware.GetOperationName(r.Context())
		}),
	))
	mux.Use(xhttp.Recover(a.Logger, a.Config.ServerConfig.HTTPResponsePanicStacktrace))

	mux.Mount("/", a.adminHandler(adminHandler))

	saltAPIHandler, err := a.saltAPIHandler(ctx, saltPB.salt)
	if err != nil {
		return nil, fmt.Errorf("salt api handler: %w", err)
	}
	mux.Mount("/salt", saltAPIHandler)

	userAPIHandler, err := a.userAPIHandler(ctx, userapiPB)
	if err != nil {
		return nil, fmt.Errorf("user api handler: %w", err)
	}
	mux.Mount("/userapi", userAPIHandler)

	return &httpServer{
		logger: a.Logger,
		server: &http.Server{
			Addr:    fmt.Sprintf(":%d", a.Config.ServerConfig.HTTPPort),
			Handler: mux,
		},
	}, nil
}

func (a *App) adminHandler(adminHandler *adminhandler.Handler) http.Handler {
	mux := chi.NewMux()

	mux.Get("/ping", xhandlers.Ping())
	mux.Mount("/metrics", solomonhttp.NewHandler(
		a.MetricsRegistry,
		solomonhttp.WithLogger(a.Logger),
		solomonhttp.WithSpack()),
	)
	mux.Mount("/doc", doc.NewDocHandler())

	mux.Post("/run_database_gc", adminHandler.RunDatabaseGC)

	if a.Config.ServerConfig.ConfigHandlerEnabled {
		mux.Get("/debug/config", configHandler(a.Logger, a.Config))
	}

	if a.Config.ServerConfig.PprofEnabled {
		mux.Mount("/debug", xpprof.NewRouter("/debug/pprof"))
	}

	return mux
}

func configHandler(logger log.Logger, cfg interface{}) func(w http.ResponseWriter, r *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set(headers.ContentTypeKey, headers.TypeApplicationJSON.String())
		err := json.NewEncoder(w).Encode(cfg)
		if err != nil {
			ctxlog.Errorf(r.Context(), logger, "can't encode config: %v", err)
		}
	}
}

func (a *App) saltAPIHandler(ctx context.Context, server saltpb.SaltServiceServer) (*runtime.ServeMux, error) {
	mux := runtime.NewServeMux()

	err := saltpb.RegisterSaltServiceHandlerServer(ctx, mux, server)
	if err != nil {
		return nil, fmt.Errorf("register salt service: %w", err)
	}

	return mux, nil
}

func (a *App) userAPIHandler(ctx context.Context, userapiPB *userapiPB) (*runtime.ServeMux, error) {
	mux := runtime.NewServeMux()

	err := userapipb.RegisterResourceServiceHandlerServer(ctx, mux, userapiPB.resource)
	if err != nil {
		return nil, fmt.Errorf("register resource service: %w", err)
	}

	err = userapipb.RegisterOriginServiceHandlerServer(ctx, mux, userapiPB.origin)
	if err != nil {
		return nil, fmt.Errorf("register origins service: %w", err)
	}

	err = userapipb.RegisterOriginsGroupServiceHandlerServer(ctx, mux, userapiPB.originsGroup)
	if err != nil {
		return nil, fmt.Errorf("register origins group service: %w", err)
	}

	return mux, nil
}

func (s *httpServer) run() error {
	s.logger.Infof("run http server: %s", s.server.Addr)

	err := s.server.ListenAndServe()
	if err != nil && !errors.Is(err, http.ErrServerClosed) {
		return fmt.Errorf("stop http server: %w", err)
	}

	return nil
}

func (s *httpServer) stop() error {
	s.logger.Infof("stopping http server: %s", s.server.Addr)
	if err := s.server.Shutdown(context.Background()); err != nil {
		return fmt.Errorf("server shutdown: %w", err)
	}

	return nil
}
