package console

import (
	"context"

	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/console/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

type CloudService struct {
	kfv1.UnimplementedCloudServiceServer

	console console.Console
}

var _ kfv1.CloudServiceServer = &CloudService{}

func NewCloudService(console console.Console) *CloudService {
	return &CloudService{
		console: console,
	}
}

func (c *CloudService) List(ctx context.Context, req *kfv1.ListCloudsRequest) (*kfv1.ListCloudsResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}
	pageToken.Offset = pagination.SanePageOffset(pageToken.Offset)
	pageSize := pagination.SanePageSize(req.GetPaging().GetPageSize())

	clouds, newOffset, err := c.console.GetCloudsByClusterType(ctx, clusters.TypeKafka, pageSize, pageToken.Offset)
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

	return &kfv1.ListCloudsResponse{
		Clouds:   kafka.CloudsToGRPC(clouds),
		NextPage: &apiv1.NextPage{Token: nextPageToken},
	}, nil
}
