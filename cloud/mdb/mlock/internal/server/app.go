package server

import (
	"context"
	"fmt"
	"net"
	"time"

	grpc_prometheus "github.com/grpc-ecosystem/go-grpc-prometheus"
	"google.golang.org/grpc"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/deprecated/app"
	"a.yandex-team.ru/cloud/mdb/internal/fs/fsnotify"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	mlock "a.yandex-team.ru/cloud/mdb/mlock/api"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/mlockdb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	MlockDBPasswdEnvName = "MLOCKDB_PASSWORD"
)

// Config describes application config
type Config struct {
	app.Config   `json:"config" yaml:"config,inline"`
	UseAuth      bool               `json:"use_auth" yaml:"use_auth"`
	Auth         AuthConfig         `json:"auth" yaml:"auth"`
	GRPC         GRPCConfig         `json:"grpc" yaml:"grpc"`
	MlockDB      pgutil.Config      `json:"mlockdb" yaml:"mlockdb"`
	SLBCloseFile SLBCloseFileConfig `json:"slb_close_file" yaml:"slb_close_file"`
	Retry        retry.Config       `json:"retry" yaml:"retry"`
}

// SLBCloseFileConfig describes file closer configuration
type SLBCloseFileConfig struct {
	FilePath string `json:"file_path" yaml:"file_path"`
}

// DefaultSLBCloseFileConfig returns default SLBCloseFileConfig
func DefaultSLBCloseFileConfig() SLBCloseFileConfig {
	return SLBCloseFileConfig{
		FilePath: "/tmp/.mdb-mlock-close",
	}
}

// GRPCConfig describes grpc interface configuration
type GRPCConfig struct {
	Addr string `json:"addr" yaml:"addr"`
}

// DefaultGRPCConfig returns default GRPCConfig
func DefaultGRPCConfig() GRPCConfig {
	return GRPCConfig{
		Addr: "[::1]:30030",
	}
}

// AuthConfig describes accessservice client configuration
type AuthConfig struct {
	Addr         string                `json:"addr" yaml:"addr"`
	Permission   string                `json:"permission" yaml:"permission"`
	FolderID     string                `json:"folder_id" yaml:"folder_id"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
}

// DefaultAuthConfig returns default AuthConfig
func DefaultAuthConfig() AuthConfig {
	return AuthConfig{
		Addr: "as.cloud.yandex-team.ru:4286",
		ClientConfig: grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{CAFile: "/opt/yandex/allCAs.pem"},
			},
		},
	}
}

// DefaultRetryConfig returns default retry.Config
func DefaultRetryConfig() retry.Config {
	return retry.Config{
		MaxRetries:      3,
		InitialInterval: 2 * time.Millisecond,
		MaxInterval:     10 * time.Millisecond,
		MaxElapsedTime:  50 * time.Millisecond,
	}
}

// DefaultConfig returns default application config
func DefaultConfig() Config {
	cfg := Config{
		UseAuth:      true,
		Auth:         DefaultAuthConfig(),
		Config:       app.DefaultConfig(),
		GRPC:         DefaultGRPCConfig(),
		MlockDB:      pg.DefaultConfig(),
		SLBCloseFile: DefaultSLBCloseFileConfig(),
		Retry:        DefaultRetryConfig(),
	}
	cfg.MlockDB.Password.FromEnv(MlockDBPasswdEnvName)

	return cfg
}

// App is an application instance
type App struct {
	*app.App

	grpcServer   *grpc.Server
	grpcListener net.Listener
}

// NewApp is an application constructor
func NewApp(config Config) (*App, error) {
	logger, err := zap.New(zap.JSONConfig(config.LogLevel))

	if err != nil {
		return nil, fmt.Errorf("unable to init logger: %w", err)
	}

	mdb, err := pg.New(config.MlockDB, logger)

	if err != nil {
		return nil, fmt.Errorf("unable to init mlockdb: %w", err)
	}

	// TODO: graceful shutdown (use 'good' context)
	closer, err := fsnotify.NewFileWatcher(context.Background(), config.SLBCloseFile.FilePath, logger)

	if err != nil {
		return nil, fmt.Errorf("unable to init slb close watcher: %w", err)
	}

	application := &App{
		App: app.New(config.Config, logger),
	}

	backoff := retry.New(config.Retry)

	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(
			interceptors.ChainUnaryServerInterceptors(
				backoff,
				false,
				nil,
				interceptors.DefaultLoggingConfig(),
				logger,
			),
		),
	}

	server := grpc.NewServer(options...)

	health := HealthService{MlockDB: mdb, Closer: closer, Logger: logger}
	grpchealth.RegisterHealthServer(server, &health)

	service := MlockService{MlockDB: mdb, Logger: logger}
	if config.UseAuth {
		asClient, err := grpcas.NewClient(context.Background(), config.Auth.Addr, "MDB Mlock", config.Auth.ClientConfig, logger)

		if err != nil {
			return nil, fmt.Errorf("unable to init auth: %w", err)
		}

		service.Auth = &Auth{
			Client:     asClient,
			FolderID:   as.ResourceFolder(config.Auth.FolderID),
			Permission: config.Auth.Permission,
		}
	}

	mlock.RegisterLockServiceServer(server, &service)

	grpc_prometheus.EnableHandlingTimeHistogram()
	grpc_prometheus.Register(server)

	stats := Stats{L: logger, MlockDB: mdb}
	go stats.Run(application.ShutdownCtx())

	logger.Debug("Initializing GRPC listener", log.String("addr", config.GRPC.Addr))
	listener, err := net.Listen("tcp", config.GRPC.Addr)
	if err != nil {
		return nil, fmt.Errorf("unable to init listener: %w", err)
	}

	application.grpcServer = server
	application.grpcListener = listener

	return application, nil
}

// Run starts grpc server
func (application *App) Run() error {
	application.App.Logger().Info("Starting up")

	if err := application.grpcServer.Serve(application.grpcListener); err != nil {
		return fmt.Errorf("failed to serve gRPC: %w", err)
	}

	return nil
}
