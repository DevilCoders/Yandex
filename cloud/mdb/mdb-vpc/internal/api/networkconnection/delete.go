package networkconnection

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

func (s Service) Delete(ctx context.Context, req *network.DeleteNetworkConnectionRequest) (*network.DeleteNetworkConnectionResponse, error) {
	ctx, err := s.db.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return nil, err
	}

	nc, err := s.db.NetworkConnectionByID(ctx, req.NetworkConnectionId)
	if err != nil {
		return nil, err
	}

	author, err := s.auth.Authorize(ctx, auth.NetworkConnectionDeletePermission, nc.ProjectID)
	if err != nil {
		return nil, err
	}

	opID, err := s.db.InsertOperation(
		ctx,
		nc.ProjectID,
		"delete network connection operation",
		author,
		&models.DeleteNetworkConnectionOperationParams{
			NetworkConnectionID: req.NetworkConnectionId,
		},
		models.OperationActionDeleteNetworkConnection,
		nc.Provider,
		nc.Region,
	)
	if err != nil {
		return nil, err
	}

	if err := s.db.MarkNetworkConnectionDeleting(ctx, req.NetworkConnectionId, fmt.Sprintf("deleting by operation %q", opID)); err != nil {
		return nil, err
	}

	err = s.db.Commit(ctx)
	if err != nil {
		return nil, err
	}

	return &network.DeleteNetworkConnectionResponse{OperationId: opID}, nil
}
