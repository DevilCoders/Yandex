package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

type VersionService struct {
	chv1.UnimplementedVersionServiceServer

	cfg logic.CHConfig
}

var _ chv1.VersionServiceServer = &VersionService{}

func NewVersionService(cfg logic.CHConfig) *VersionService {
	return &VersionService{
		cfg: cfg,
	}
}

func (v *VersionService) List(ctx context.Context, req *chv1.ListVersionsRequest) (*chv1.ListVersionsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	page := pagination.NewPage(int64(len(v.cfg.Versions)), req.GetPaging().GetPageSize(), pageToken.Offset)

	grpcVersions := VersionsToGRPC(v.cfg.Versions[page.LowerIndex:page.UpperIndex])

	versionPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}
	nextPageToken, err := api.BuildPageTokenToGRPC(versionPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListVersionsResponse{
		Versions: grpcVersions,
		NextPage: &apiv1.NextPage{
			Token: nextPageToken,
		},
	}, nil
}
