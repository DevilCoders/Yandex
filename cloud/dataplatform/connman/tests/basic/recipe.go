package basic

import (
	"context"
	"net"

	grpc_middleware "github.com/grpc-ecosystem/go-grpc-middleware"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/encryption"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/serialization"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/server"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/storage"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/view"
	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
)

type ConnmanRecipe struct {
	Client connman.ConnectionServiceClient

	pg       *dbaas.PgHA
	listener net.Listener
	conn     *grpc.ClientConn
}

func (c *ConnmanRecipe) Close() error {
	if err := c.pg.Close(); err != nil {
		return xerrors.Errorf("unable to close pg: %w", err)
	}
	if err := c.listener.Close(); err != nil {
		return xerrors.Errorf("unable to close net listener: %w", err)
	}
	if err := c.conn.Close(); err != nil {
		return xerrors.Errorf("unable to close grpc connection: %w", err)
	}
	return nil
}

func NewConnmanRecipe() (*ConnmanRecipe, error) {
	grpcServer := grpc.NewServer(
		grpc.UnaryInterceptor(
			grpc_middleware.ChainUnaryServer(
				func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
					ctx = auth.WithSubject(ctx, cloudauth.UserAccount{ID: "test"})
					return handler(ctx, req)
				},
			),
		),
	)

	loggerFactory := func(ctx context.Context) log.Logger { return logger.Log }

	pg, err := NewRecipePG()
	if err != nil {
		return nil, xerrors.Errorf("unable to initialize pg: %w", err)
	}

	encryptionProvider := encryption.NewFakeProvider()

	serializer := serialization.NewSerializer(encryptionProvider)

	storage := storage.NewPgStorage(pg, serializer, loggerFactory)

	viewer := view.NewViewer(encryption.NewFakeLockboxDecryptor())

	authProvider := auth.AnonymousFakeProvider()

	connectionServer := server.NewConnectionServer(storage, authProvider, viewer)

	connman.RegisterConnectionServiceServer(grpcServer, connectionServer)

	listener, err := net.Listen("tcp", ":")
	if err != nil {
		return nil, xerrors.Errorf("unable to init listener: %w", err)
	}
	go func() {
		_ = grpcServer.Serve(listener)
	}()

	grpcURL := listener.Addr().String()
	conn, err := grpc.Dial(grpcURL, grpc.WithInsecure())
	if err != nil {
		return nil, xerrors.Errorf("unable to dial grpc: %w", err)
	}

	client := connman.NewConnectionServiceClient(conn)

	return &ConnmanRecipe{
		Client:   client,
		pg:       pg,
		listener: listener,
		conn:     conn,
	}, nil
}
