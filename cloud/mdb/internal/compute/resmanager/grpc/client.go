package grpc

import (
	"context"
	"time"

	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"

	resmanv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
)

type Client struct {
	clouds  resmanv1.CloudServiceClient
	folders resmanv1.FolderServiceClient
}

var _ resmanager.Client = &Client{}

func NewClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*Client, error) {
	conn, err := grpcutil.NewConn(
		ctx,
		target,
		userAgent,
		cfg,
		l,
		grpcutil.WithClientCredentials(creds),
		grpcutil.WithClientKeepalive(
			// https://st.yandex-team.ru/CLOUDINC-1322#5fddb68ef5bd1f2229a57d5c
			keepalive.ClientParameters{
				Time:                11 * time.Second,
				Timeout:             time.Second,
				PermitWithoutStream: true,
			},
		),
	)
	if err != nil {
		return nil, err
	}

	return &Client{
		clouds:  resmanv1.NewCloudServiceClient(conn),
		folders: resmanv1.NewFolderServiceClient(conn),
	}, nil
}
