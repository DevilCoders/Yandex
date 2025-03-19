package yc

import (
	"context"
	"crypto/tls"
	"errors"
	"fmt"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

type grpcClient struct {
	conn *grpc.ClientConn
}

// until we use only compute api, we specify it here
// later could be modified like in gosdk or a like
const defaultEndpoint = "compute.api.cloud.yandex.net:443"
const defaultTimeout = 20 * time.Second

func newGRPCClient(token string) (*grpcClient, error) {
	if token == "" {
		return nil, errors.New("empty token")
	}

	tlsConfig := &tls.Config{}
	creds := credentials.NewTLS(tlsConfig)
	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(creds),
		grpc.WithBlock(),
		grpc.WithPerRPCCredentials(&rpcCreds{token: token}),
	}

	ctx, cancel := context.WithTimeout(context.Background(), defaultTimeout)
	defer func() { cancel() }()

	conn, err := grpc.DialContext(ctx, defaultEndpoint, opts...)
	if err != nil {
		return nil, fmt.Errorf("failed to dial server: %v", err)
	}

	return &grpcClient{conn}, nil
}

func (c *grpcClient) getConn() (*grpc.ClientConn, error) {
	if c.conn == nil {
		return nil, errors.New("connection already closed")
	}

	return c.conn, nil
}

func (c *grpcClient) close() error {
	if c.conn == nil {
		return errors.New("connection already closed")
	}

	return c.conn.Close()
}
