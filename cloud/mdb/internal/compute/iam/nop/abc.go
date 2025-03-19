package nop

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/team/integration/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
)

type AbcServiceClient struct {
	abcAPI integration.AbcServiceClient
}

var _ iam.AbcService = &AbcServiceClient{}

func (a *AbcServiceClient) ResolveByABCSlug(ctx context.Context, abcSlug string) (iam.ABC, error) {
	return iam.ABC{}, nil
}

func (a *AbcServiceClient) ResolveByCloudID(ctx context.Context, cloudID string) (iam.ABC, error) {
	return iam.ABC{}, nil
}

func (a *AbcServiceClient) ResolveByFolderID(ctx context.Context, folderID string) (iam.ABC, error) {
	return iam.ABC{}, nil
}
