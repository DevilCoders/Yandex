package console

import (
	"context"
	"sort"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/console/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/config"
)

type CloudService struct {
	regions config.Regions
}

var _ console.CloudServiceServer = &CloudService{}

func NewService(regions config.Regions) console.CloudServiceServer {
	return &CloudService{regions: regions}
}

func (s CloudService) List(_ context.Context, _ *console.ListCloudsRequest) (*console.ListCloudsResponse, error) {
	awsRegions := s.regions.AWS
	sort.Strings(awsRegions)

	return &console.ListCloudsResponse{
		Clouds: []*console.Cloud{
			{
				CloudType: "aws",
				RegionIds: awsRegions,
			},
		},
	}, nil
}
