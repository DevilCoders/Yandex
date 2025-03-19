package keystoreclient

import (
	"context"
	"crypto/tls"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/metadata"

	eks "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1/keystore"
)

type KeystoreClient interface {
	ExportKeystore(ctx context.Context, keystoreID string, IAMToken string) (*eks.ExportKeystoreKeysResponse, error)
	Close() error
}

type client struct {
	conn    *grpc.ClientConn
	service eks.ExportableKeysServiceClient
}

func (c *client) ExportKeystore(ctx context.Context, keystoreID string, IAMToken string) (*eks.ExportKeystoreKeysResponse, error) {
	ctx = metadata.AppendToOutgoingContext(ctx, "Authorization", "Bearer "+IAMToken)
	return c.service.ExportKeys(ctx, &eks.ExportKeystoreKeysRequest{KeystoreId: keystoreID})
}

func (c *client) Close() (err error) {
	if c.conn != nil {
		return c.conn.Close()
	}
	return
}

func NewKeystoreClient(keystoreAddress string) (KeystoreClient, error) {
	var dialOptions []grpc.DialOption
	dialOptions = append(dialOptions, grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{})))
	dialOptions = append(dialOptions, grpc.WithKeepaliveParams(keepalive.ClientParameters{
		Time:                20 * time.Second,
		Timeout:             2 * time.Second,
		PermitWithoutStream: false,
	}))
	conn, err := grpc.Dial(
		keystoreAddress,
		dialOptions...,
	)
	if err != nil {
		return nil, err
	}
	c := &client{
		conn:    conn,
		service: eks.NewExportableKeysServiceClient(conn),
	}
	return c, nil
}
