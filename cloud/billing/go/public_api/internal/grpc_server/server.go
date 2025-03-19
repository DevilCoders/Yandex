package grpcserver

import (
	"context"
	"crypto/tls"
	"net"
	"os/user"
	"strconv"
	"syscall"

	grpc_middleware "github.com/grpc-ecosystem/go-grpc-middleware"
	grpc_recovery "github.com/grpc-ecosystem/go-grpc-middleware/recovery"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/reflection"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/config"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server/services"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/scope"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	"a.yandex-team.ru/library/go/core/log"
)

func ConfigureServices(server *grpc.Server, client console.Client) {

	billing_grpc.RegisterSkuServiceServer(server, services.NewSkuService(client))
	billing_grpc.RegisterServiceServiceServer(server, services.NewServiceService(client))
	billing_grpc.RegisterOperationServiceServer(server, services.NewOperationService(client))
	billing_grpc.RegisterCustomerServiceServer(server, services.NewCustomerService(client))
	billing_grpc.RegisterBudgetServiceServer(server, services.NewBudgetService(client))
	billing_grpc.RegisterBillingAccountServiceServer(server, services.NewBillingAccountService(client))
	reflection.Register(server)
}

func ToolingInterceptor(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
	ctx = scope.StartGRPCRequest(ctx, info.FullMethod)
	defer func() { scope.FinishGRPCRequest(ctx, err) }()

	resp, err = handler(ctx, req)
	return
}

func setUIDByName(userName string) error {
	userInfo, err := user.Lookup(userName)
	if err != nil {
		return err
	}

	uid, err := strconv.Atoi(userInfo.Uid)
	if err != nil {
		return err
	}

	err = syscall.Setuid(uid)
	if err != nil {
		return err
	}

	return nil
}

func Create(ctx context.Context, appCfg *config.Config) (*grpc.Server, error) {

	// read certificates as root
	tlsCreds, err := loadTLSCredentials(appCfg)
	if err != nil {
		return nil, err
	}

	// run as non root user
	err = setUIDByName(appCfg.ProcessUserName)
	if err != nil {
		return nil, err
	}

	server := grpc.NewServer(
		grpc.Creds(tlsCreds),
		grpc.UnaryInterceptor(grpc_middleware.ChainUnaryServer(
			ToolingInterceptor,
			grpc_recovery.UnaryServerInterceptor(),
		)),
	)
	return server, nil
}

func Run(ctx context.Context, server *grpc.Server, addr string) (err error) {
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	go func() {
		defer cancel()
		var listener net.Listener

		scope.Logger(ctx).Info("start grpc server", log.String("address", addr))
		if listener, err = net.Listen("tcp", addr); err != nil {
			scope.Logger(ctx).Error("cant start listener")
			return
		}
		err = server.Serve(listener)
	}()

	if <-ctx.Done(); err != nil {
		scope.Logger(ctx).Error("grpc server failed with error")
		return
	}

	scope.Logger(ctx).Error("shutdown grpc server")
	server.GracefulStop()
	return
}

func loadTLSCredentials(appCfg *config.Config) (credentials.TransportCredentials, error) {
	serverCert, err := tls.LoadX509KeyPair(appCfg.SSLCertificatePath, appCfg.SSLCertificateKeyPath)

	if err != nil {
		return nil, err
	}

	tlsCfg := &tls.Config{
		Certificates: []tls.Certificate{serverCert},
		ClientAuth:   tls.NoClientCert,
	}

	return credentials.NewTLS(tlsCfg), nil
}
