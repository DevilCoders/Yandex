package grpc

import (
	"context"

	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/team/integration/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type AbcServiceClient struct {
	abcAPI integration.AbcServiceClient
}

var _ iam.AbcService = &AbcServiceClient{}

func NewAbcServiceClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*AbcServiceClient, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to IAM Yandex Team Integration API at %q: %w", target, err)
	}

	return &AbcServiceClient{
		abcAPI: integration.NewAbcServiceClient(conn),
	}, nil
}

func (a *AbcServiceClient) ResolveByABCSlug(ctx context.Context, abcSlug string) (iam.ABC, error) {
	resp, err := a.abcAPI.Resolve(ctx, &integration.ResolveRequest{Abc: &integration.ResolveRequest_AbcSlug{AbcSlug: abcSlug}})
	if err != nil {
		return iam.ABC{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	result := iam.ABC{
		CloudID:         resp.CloudId,
		AbcSlug:         resp.AbcSlug,
		AbcID:           resp.AbcId,
		DefaultFolderID: resp.DefaultFolderId,
		AbcFolderID:     resp.AbcFolderId,
	}
	return result, nil
}

func (a *AbcServiceClient) ResolveByCloudID(ctx context.Context, cloudID string) (iam.ABC, error) {
	resp, err := a.abcAPI.Resolve(ctx, &integration.ResolveRequest{Abc: &integration.ResolveRequest_CloudId{CloudId: cloudID}})
	if err != nil {
		return iam.ABC{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	result := iam.ABC{
		CloudID:         resp.CloudId,
		AbcSlug:         resp.AbcSlug,
		AbcID:           resp.AbcId,
		DefaultFolderID: resp.DefaultFolderId,
		AbcFolderID:     resp.AbcFolderId,
	}
	return result, nil
}

func (a *AbcServiceClient) ResolveByFolderID(ctx context.Context, folderID string) (iam.ABC, error) {
	resp, err := a.abcAPI.Resolve(ctx, &integration.ResolveRequest{Abc: &integration.ResolveRequest_AbcFolderId{AbcFolderId: folderID}})
	if err != nil {
		return iam.ABC{}, grpcerr.SemanticErrorFromGRPC(err)
	}
	result := iam.ABC{
		CloudID:         resp.CloudId,
		AbcSlug:         resp.AbcSlug,
		AbcID:           resp.AbcId,
		DefaultFolderID: resp.DefaultFolderId,
		AbcFolderID:     resp.AbcFolderId,
	}
	return result, nil
}
