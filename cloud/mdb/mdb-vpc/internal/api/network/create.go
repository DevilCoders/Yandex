package network

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

func (s *Service) Create(ctx context.Context, req *network.CreateNetworkRequest) (*network.CreateNetworkResponse, error) {
	author, err := s.auth.Authorize(ctx, auth.NetworkCreatePermission, req.ProjectId)
	if err != nil {
		return nil, err
	}

	var provider models.Provider
	switch req.CloudType {
	case string(models.ProviderAWS):
		provider = models.ProviderAWS
	default:
		return nil, semerr.InvalidInput("unsupported cloud_type")
	}

	ctx, err = s.db.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return nil, err
	}

	networkID, err := s.db.CreateNetwork(
		ctx,
		req.ProjectId,
		provider,
		req.RegionId,
		req.Name,
		req.Description,
		req.Ipv4CidrBlock,
		nil,
	)
	if err != nil {
		return nil, err
	}

	opID, err := s.db.InsertOperation(
		ctx,
		req.ProjectId,
		"create operation",
		author,
		&models.CreateNetworkOperationParams{NetworkID: networkID},
		models.OperationActionCreateVPC,
		provider,
		req.RegionId,
	)
	if err != nil {
		return nil, err
	}

	err = s.db.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return &network.CreateNetworkResponse{NetworkId: networkID, OperationId: opID}, nil
}
