package network

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
)

func (s Service) List(ctx context.Context, req *network.ListNetworksRequest) (*network.ListNetworksResponse, error) {
	if _, err := s.auth.Authorize(ctx, auth.NetworkGetPermission, req.ProjectId); err != nil {
		return nil, err
	}

	nets, err := s.db.NetworksByProjectID(ctx, req.ProjectId)
	if err != nil {
		return nil, err
	}

	res := &network.ListNetworksResponse{
		Networks: make([]*network.Network, len(nets)),
	}
	for i, net := range nets {
		res.Networks[i] = networkFromDB(net)
	}
	return res, nil
}
