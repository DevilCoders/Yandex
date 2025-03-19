package server

import (
	"net"
	"net/http"
	"strings"
	"sync"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"

	grpc_opentracing "github.com/grpc-ecosystem/go-grpc-middleware/tracing/opentracing"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/snapshot/internal/auth"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"

	"github.com/golang/protobuf/proto"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"go.uber.org/zap"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/mon"
	"a.yandex-team.ru/cloud/compute/go-common/tracing/grpcinterceptors"
	"a.yandex-team.ru/cloud/compute/snapshot/pkg/internetwork"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/snapshot_rpc"
)

func intercept(ctx context.Context, w http.ResponseWriter, m proto.Message) error {
	// We need empty list instead of absent
	l, ok := m.(*snapshot_rpc.SnapshotList)
	if ok && l.Result == nil {
		l.Result = make([]*snapshot_rpc.SnapshotInfo, 0)
	}

	md, ok := runtime.ServerMetadataFromContext(ctx)
	if ok {
		for k, v := range md.HeaderMD {
			if strings.ToLower(k) == "content-type" {
				continue
			}
			for _, vi := range v {
				w.Header().Add(k, vi)
			}
		}
	}
	return nil
}

func configureHTTP2gRPCMux() *runtime.ServeMux {
	// We implement custom error conversion
	runtime.HTTPError = httpError

	// encoding/json is unable to emit defaults
	marshaler := &runtime.JSONPb{
		EmitDefaults: true,
		OrigName:     true,
	}

	mux := runtime.NewServeMux(
		runtime.WithForwardResponseOption(intercept),
		runtime.WithMarshalerOption(marshaler.ContentType(), marshaler),
	)

	return mux
}

type customServeMux struct {
	*runtime.ServeMux
}

func (mux *customServeMux) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	// Handle trainilg slashes
	r.URL.Path = strings.TrimSuffix(r.URL.Path, "/")
	r.URL.RawPath = strings.TrimSuffix(r.URL.RawPath, "/")
	mux.ServeMux.ServeHTTP(w, r)
}

// Listeners is a pack of listeners with incoming requests
// NOTE: To not mix up listeners they're passed as struct with explicit naming
type Listeners struct {
	GRPCListener, HTTPListener net.Listener
}

// Server handles both types of requests: gRPC and HTTP
type Server struct {
	ctx        context.Context
	cancelFunc context.CancelFunc

	// gRPC handler
	handler *snapshotServer

	// httpServer handles external HTTP API requests
	// translated to gRCP via proxy
	httpServer http.Server

	// grpcServer handles incoming grpc API requests
	gRPCServer *grpc.Server

	// http2gRPCServer server handles connections from
	// integrated http2gRPC proxy
	// it uses inmemory net.Listener
	http2gRPCServer *grpc.Server

	// inmemory net.Listener
	inmemListener internetwork.InternalNetwork
}

// NewServer creates new server for both gRPC and HTTP API
func NewServer(ctx context.Context, conf *config.Config) (*Server, error) {
	var interceptors = []grpc.UnaryServerInterceptor{
		grpcinterceptors.MetadataInterceptor(),
		grpc_opentracing.UnaryServerInterceptor(),
		misc.MetricsInterceptor(),
	}

	if conf.AccessService.Address != "" {
		accessService, err := auth.NewAccessServiceClient(ctx, conf.AccessService)
		log.DebugErrorCtx(ctx, err, "Create access service client")
		if err != nil {
			return nil, xerrors.Errorf("create access service client: %w", err)
		}
		interceptors = append(interceptors, auth.NewServerInterceptor(accessService))
	}

	// local metadata credentials token provider
	globalauth.InitCredentials(ctx, conf.General.TokenPolicy)

	// Set gRPC server options
	opts := []grpc.ServerOption{
		// TODO: configuration
		grpc.MaxSendMsgSize(100 * 1024 * 1024),
		grpc.MaxRecvMsgSize(100 * 1024 * 1024),
		grpc.ChainUnaryInterceptor(interceptors...),
	}

	// We never use SSL on inmemory gRCP server
	http2gRPCServer := grpc.NewServer(opts...)

	if conf.Server.SSL {
		tc, err := credentials.NewServerTLSFromFile(conf.Server.CertFile, conf.Server.KeyFile)
		if err != nil {
			return nil, err
		}
		opts = append(opts, grpc.Creds(tc))
	}

	grpcServer := grpc.NewServer(opts...)

	snapshotService, err := newSnapshotServer(ctx, conf)
	if err != nil {
		return nil, err
	}

	ctx, cancelFunc := context.WithCancel(ctx)
	server := &Server{
		ctx:        ctx,
		cancelFunc: cancelFunc,

		handler: snapshotService,

		gRPCServer:      grpcServer,
		http2gRPCServer: http2gRPCServer,

		inmemListener: internetwork.New(128),
	}
	snapshot_rpc.RegisterSnapshotServiceServer(grpcServer, snapshotService)
	snapshot_rpc.RegisterSnapshotServiceServer(http2gRPCServer, snapshotService)
	return server, nil
}

// Close releases resources if Server was not launched
func (s *Server) Close(ctx context.Context) {
	if err := s.handler.Close(); err != nil {
		log.G(ctx).Warn("handler.Close error", zap.Error(err))
	}
}

// Start uses passed listener to server on
func (s *Server) Start(ls Listeners) error {
	var wg sync.WaitGroup

	// start internal grpc proxy
	mux := configureHTTP2gRPCMux()
	// NOTE: although Dialer is blocking function, this Dial never
	opts := []grpc.DialOption{
		//nolint:staticcheck
		//nolint:SA1019
		grpc.WithDialer(s.inmemListener.DialgRPC),
		grpc.WithInsecure()}
	err := snapshot_rpc.RegisterSnapshotServiceHandlerFromEndpoint(s.ctx, mux, "inmemory_grpc_snapshot_server", opts)
	if err != nil {
		return err
	}

	startServer := func(name string, starter func() error) {
		wg.Add(1)
		go func() {
			defer wg.Done()
			if err := starter(); err != nil && !s.Stopped() {
				log.G(s.ctx).Error(name+".Serve failed", zap.Error(err))
			}
		}()
	}

	// Start in-memory gRPC server
	startServer("http2gRPCServer", func() error {
		return s.http2gRPCServer.Serve(s.inmemListener)
	})

	// Start gRPC server
	startServer("gRPCServer", func() error {
		return s.gRPCServer.Serve(ls.GRPCListener)
	})

	// Start HTTP Server
	s.httpServer.Handler = &customServeMux{mux}
	startServer("httpServer", func() error {
		return s.httpServer.Serve(ls.HTTPListener)
	})

	wg.Wait()
	return nil
}

// Stop closes open listeners
func (s *Server) Stop() {
	// we need it to close internal grpcConnection
	s.cancelFunc()
	s.gRPCServer.Stop()
	if err := s.httpServer.Close(); err != nil {
		log.G(s.ctx).Error("httpServer.Close failed", zap.Error(err))
	}
	s.http2gRPCServer.Stop()
}

// GracefulStop does its best to stop the server gracefully
func (s *Server) GracefulStop(shutdownCtx context.Context) {
	// we need it to close internal grpcConnection
	log.G(shutdownCtx).Info("calling cancelFunc")
	s.cancelFunc()
	log.G(shutdownCtx).Info("calling gRPCServer.GracefulStop")
	s.gRPCServer.GracefulStop()
	log.G(shutdownCtx).Info("calling httpServer.Shutdown")
	if err := s.httpServer.Shutdown(shutdownCtx); err != nil {
		log.G(s.ctx).Error("httpServer.Shutdown failed", zap.Error(err))
	}
	log.G(shutdownCtx).Info("calling http2gRPCServer.GracefulStop")
	s.http2gRPCServer.GracefulStop()
}

// Stopped shows whether server is stopped
func (s *Server) Stopped() bool {
	select {
	case <-s.ctx.Done():
		return true
	default:
		return false
	}
}

// RegisterMon registers monitoring handler
func (s *Server) RegisterMon(r mon.Repository) error {
	return s.handler.f.RegisterMon(r)
}
