package console

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/console/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

type CloudService struct {
	chv1.UnimplementedCloudServiceServer

	console console.Console
}

var _ chv1.CloudServiceServer = &CloudService{}

func NewCloudService(console console.Console) *CloudService {
	return &CloudService{
		console: console,
	}
}

func (c *CloudService) List(ctx context.Context, req *chv1.ListCloudsRequest) (*chv1.ListCloudsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}
	pageToken.Offset = pagination.SanePageOffset(pageToken.Offset)
	pageSize := pagination.SanePageSize(req.GetPaging().GetPageSize())

	clouds, newOffset, err := c.console.GetCloudsByClusterType(ctx, clusters.TypeClickHouse, pageSize, pageToken.Offset)
	if err != nil {
		return nil, err
	}

	var cloudPageToken pagination.OffsetPageToken
	if int64(len(clouds)) == pageSize {
		cloudPageToken = pagination.NewOffsetPageToken(newOffset)
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(cloudPageToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListCloudsResponse{
		Clouds:   clickhouse.CloudsToGRPC(clouds),
		NextPage: &apiv1.NextPage{Token: nextPageToken},
	}, nil
}
