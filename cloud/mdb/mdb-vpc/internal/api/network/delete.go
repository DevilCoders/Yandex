package network

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

func (s Service) Delete(ctx context.Context, req *network.DeleteNetworkRequest) (*network.DeleteNetworkResponse, error) {
	net, err := s.db.NetworkByID(ctx, req.NetworkId)
	if err != nil {
		return nil, err
	}

	author, err := s.auth.Authorize(ctx, auth.NetworkDeletePermission, net.ProjectID)
	if err != nil {
		return nil, err
	}

	ncs, err := s.db.NetworkConnectionsByNetworkID(ctx, req.NetworkId)
	if err != nil {
		return nil, err
	}

	if len(ncs) > 0 {
		ncIDS := make([]string, len(ncs))
		for i, nc := range ncs {
			ncIDS[i] = nc.ID
		}
		return nil, semerr.FailedPreconditionf("can not remove Network with Network Connections: %s", strings.Join(ncIDS, ", "))
	}

	ctx, err = s.db.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return nil, err
	}
	opID, err := s.db.InsertOperation(
		ctx,
		net.ProjectID,
		"",
		author,
		&models.DeleteNetworkOperationParams{
			NetworkID: req.NetworkId,
		},
		models.OperationActionDeleteVPC,
		net.Provider,
		net.Region,
	)
	if err != nil {
		return nil, err
	}

	if err := s.db.MarkNetworkDeleting(ctx, req.NetworkId, opID); err != nil {
		return nil, err
	}

	err = s.db.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return &network.DeleteNetworkResponse{OperationId: opID}, nil
}
