package tests

import (
	"context"
	"fmt"
	"net"
	"os"

	"github.com/DATA-DOG/godog"
	"github.com/golang/protobuf/proto"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	mlock "a.yandex-team.ru/cloud/mdb/mlock/api"
	"a.yandex-team.ru/cloud/mdb/mlock/internal/mlockdb/pg"
	mlockServer "a.yandex-team.ru/cloud/mdb/mlock/internal/server"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	envNameHost = "MLOCKDB_POSTGRESQL_RECIPE_HOST"
	envNamePort = "MLOCKDB_POSTGRESQL_RECIPE_PORT"
)

// Server is a restricted grpc server implementation for testing purposes
type Server struct {
	grpcServer   *grpc.Server
	grpcListener net.Listener
}

// Run starts server
func (server *Server) Run() error {
	if err := server.grpcServer.Serve(server.grpcListener); err != nil {
		return fmt.Errorf("failed to serve gRPC: %w", err)
	}

	return nil
}

// NewServer is a test server constructor
func NewServer(addr string, logger log.Logger) (*Server, error) {
	retryConfig := mlockServer.DefaultRetryConfig()
	retryConfig.MaxRetries = 0
	backoff := retry.New(retryConfig)

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

	config := pg.DefaultConfig()

	dbHost, ok := os.LookupEnv(envNameHost)
	if !ok {
		return nil, fmt.Errorf("missing env %s", envNameHost)
	}

	dbPort, ok := os.LookupEnv(envNamePort)
	if !ok {
		return nil, fmt.Errorf("missing env %s", envNamePort)
	}

	config.Addrs = []string{dbHost + ":" + dbPort}
	config.SSLMode = pgutil.DisableSSLMode

	logger.Debug("Running with config:", log.Reflect("config", config))

	mdb, err := pg.New(config, logger)
	if err != nil {
		return nil, fmt.Errorf("unable to init mlockdb: %w", err)
	}

	service := mlockServer.MlockService{MlockDB: mdb, Logger: logger}

	mlock.RegisterLockServiceServer(server, &service)

	listener, err := net.Listen("tcp", addr)
	if err != nil {
		return nil, err
	}

	return &Server{grpcServer: server, grpcListener: listener}, nil
}

// NewClient is a test client constructor
func NewClient(ctx context.Context, addr string, logger log.Logger) (mlock.LockServiceClient, error) {
	conn, err := grpcutil.NewConn(
		ctx,
		addr,
		"client",
		grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				Insecure: true,
			},
		},
		logger,
	)

	if err != nil {
		return nil, err
	}

	return mlock.NewLockServiceClient(conn), nil
}

// TestContext is a mlock-specific test context
type TestContext struct {
	Server       *Server
	Client       mlock.LockServiceClient
	UtilContext  *godogutil.TestContext
	LastResponse proto.Message
	LastError    error
}

// NewTestContext is a TestContext constructor from godogutil context
func NewTestContext(tc *godogutil.TestContext) (*TestContext, error) {
	logger, err := zap.New(zap.JSONConfig(log.DebugLevel))
	if err != nil {
		return nil, err
	}

	ctx := context.Background()

	port, err := testutil.GetFreePort()
	if err != nil {
		return nil, err
	}

	addr := "[::1]:" + port

	server, err := NewServer(addr, logger)
	if err != nil {
		return nil, err
	}

	go func() {
		if err := server.Run(); err != nil {
			panic(fmt.Errorf("error running server: %w", err))
		}
	}()

	client, err := NewClient(ctx, addr, logger)
	if err != nil {
		return nil, err
	}

	return &TestContext{Server: server, Client: client, UtilContext: tc}, nil
}

// RegisterSteps registers all steps in the suite
func (tctx *TestContext) RegisterSteps(suite *godog.Suite) {
	suite.Step(`service is ready`, tctx.waitReady)
	suite.Step(`we release lock "([^"]*)"$`, tctx.releaseLock)
	suite.Step(`we list locks with data`, tctx.listLocks)
	suite.Step(`we get locks list`, tctx.checkList)
	suite.Step(`we create lock with data`, tctx.createLock)
	suite.Step(`we get status of lock "([^"]*)"$`, tctx.getLockStatus)
	suite.Step(`we get status`, tctx.checkStatus)
	suite.Step(`we fail with "([^"]*)"$`, tctx.checkError)
	suite.Step(`we get no error`, tctx.noError)
}
