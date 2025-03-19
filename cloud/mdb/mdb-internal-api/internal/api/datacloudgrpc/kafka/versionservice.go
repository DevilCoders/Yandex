package kafka

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

type VersionService struct {
	kfv1.UnimplementedVersionServiceServer
}

var _ kfv1.VersionServiceServer = &VersionService{}

func NewVersionService() *VersionService {
	return &VersionService{}
}

func (v *VersionService) List(ctx context.Context, req *kfv1.ListVersionsRequest) (*kfv1.ListVersionsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	page := pagination.NewPage(int64(len(kfmodels.VersionsVisibleInConsoleDC)), req.GetPaging().GetPageSize(), pageToken.Offset)

	grpcVersions := VersionsToGRPC()

	versionPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}
	nextPageToken, err := api.BuildPageTokenToGRPC(versionPageToken, false)
	if err != nil {
		return nil, err
	}

	return &kfv1.ListVersionsResponse{
		Versions: grpcVersions,
		NextPage: &apiv1.NextPage{
			Token: nextPageToken,
		},
	}, nil
}
