package main

import (
	"context"
	"log"
	"time"

	grpc_retry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/metadata"

	rmgrpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type FolderServiceClient interface {
	Resolve(ctx context.Context, req *rmgrpc.ResolveFoldersRequest, opts ...grpc.CallOption) (*rmgrpc.ResolveFoldersResponse, error)
}

func NewResourceManagerClient(ctx context.Context, args *Args) (client FolderServiceClient, err error) {
	log.Println("Resource manager client initialization started")

	type contextKey string
	retryInterceptor := grpc_retry.UnaryClientInterceptor(
		grpc_retry.WithMax(7),
		grpc_retry.WithPerRetryTimeout(2*time.Second),
	)
	var requestIDInterceptor grpc.UnaryClientInterceptor = func(ctx context.Context, method string, req, reply interface{},
		cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {

		reqID := ctx.Value(contextKey("requestId"))
		if reqID != nil {
			_ = grpc.SetHeader(ctx, metadata.Pairs(cloudauth.RequestID, reqID.(string)))
		}
		return retryInterceptor(ctx, method, req, reply, cc, invoker, opts...)
	}

	var transportCredentials grpc.DialOption
	if args.ResourceManagerInsecure {
		transportCredentials = grpc.WithInsecure()
	} else {
		creds, err := credentials.NewClientTLSFromFile(args.YandexInternalRootCACertPath, "")
		if err != nil {
			return nil, xerrors.Errorf("Cannot create client transport credentials for resource-manager (credentials.NewClientTLSFromFile): %w", err)
		}
		transportCredentials = grpc.WithTransportCredentials(creds)
	}

	conn, err := grpc.DialContext(ctx, args.ResourceManagerEndpoint,
		transportCredentials,
		grpc.WithUnaryInterceptor(requestIDInterceptor),
		grpc.WithUserAgent("YC-CAPTCHA"),
		grpc.WithKeepaliveParams(keepalive.ClientParameters{
			Time:                10 * time.Second,
			Timeout:             1 * time.Second,
			PermitWithoutStream: true,
		}),
	)
	if err != nil {
		return nil, xerrors.Errorf("Cannot create client transport credentials (grpc.DialContext): %w", err)
	}

	log.Println("Resource manager client initialization succeed")

	return rmgrpc.NewFolderServiceClient(conn), nil
}
