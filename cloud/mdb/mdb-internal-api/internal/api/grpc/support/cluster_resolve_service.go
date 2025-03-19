package support

import (
	"context"

	support "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/support"
	logic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/support"
	"a.yandex-team.ru/library/go/core/log"
)

type ClusterResolveService struct {
	support.ClusterResolveServiceServer

	support logic.Support
	l       log.Logger
}

var _ support.ClusterResolveServiceServer = &ClusterResolveService{}

func (crs *ClusterResolveService) Get(ctx context.Context, req *support.ClusterResolveRequest) (*support.Cluster, error) {

	cid := req.ClusterId
	cluster, err := crs.support.ResolveCluster(ctx, cid)
	if err != nil {
		return nil, err
	}

	result := ResultToGRPC(cluster)

	return result, nil
}

func NewClusterResolveService(support logic.Support, l log.Logger) *ClusterResolveService {
	return &ClusterResolveService{
		support: support,
		l:       l,
	}
}
