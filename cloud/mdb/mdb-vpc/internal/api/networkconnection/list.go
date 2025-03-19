package networkconnection

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
)

func (s Service) List(ctx context.Context, req *network.ListNetworkConnectionsRequest) (*network.ListNetworkConnectionsResponse, error) {
	if _, err := s.auth.Authorize(ctx, auth.NetworkConnectionGetPermission, req.ProjectId); err != nil {
		return nil, err
	}

	ncs, err := s.db.NetworkConnectionsByProjectID(ctx, req.ProjectId)
	if err != nil {
		return nil, err
	}

	res := &network.ListNetworkConnectionsResponse{
		NetworkConnections: make([]*network.NetworkConnection, len(ncs)),
	}
	for i, nc := range ncs {
		res.NetworkConnections[i] = networkConnectionFromDB(nc)
	}
	return res, nil
}
