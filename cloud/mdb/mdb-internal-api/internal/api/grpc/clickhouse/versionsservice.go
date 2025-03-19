package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
)

type VersionsService struct {
	chv1.UnimplementedVersionsServiceServer

	l  log.Logger
	ch clickhouse.ClickHouse
}

var _ chv1.VersionsServiceServer = &VersionsService{}

func NewVersionsService(ch clickhouse.ClickHouse, l log.Logger) *VersionsService {
	return &VersionsService{ch: ch, l: l}
}

func (vs *VersionsService) List(ctx context.Context, req *chv1.ListVersionsRequest) (*chv1.ListVersionsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	chVersions := vs.ch.Versions()

	page := pagination.NewPage(int64(len(chVersions)), req.GetPageSize(), pageToken.Offset)

	grpcVersions := VersionsToGRPC(chVersions[page.LowerIndex:page.UpperIndex])

	versionPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}
	nextPageToken, err := api.BuildPageTokenToGRPC(versionPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListVersionsResponse{
		Version:       grpcVersions,
		NextPageToken: nextPageToken,
	}, nil
}
