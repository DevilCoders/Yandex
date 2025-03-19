package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
)

type OperationService struct {
	chv1.UnimplementedOperationServiceServer

	ops common.Operations
	l   log.Logger
}

var _ chv1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, l log.Logger) *OperationService {
	return &OperationService{
		ops: ops,
		l:   l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *chv1.GetOperationRequest) (*datacloud.Operation, error) {
	op, folder, err := os.ops.OperationWithFolderCoords(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}
	if db != opmodels.DatabaseClickhouse {
		return nil, semerr.FailedPreconditionf("wrong operation database %+v, must be clickhouse operation", db)
	}

	res, err := datacloudgrpc.OperationToGRPC(ctx, op, folder.FolderExtID, os.l)
	if err != nil {
		return nil, err
	}

	return res, nil
}

func (os *OperationService) List(ctx context.Context, req *chv1.ListOperationsRequest) (*chv1.ListOperationsResponse, error) {
	var pageToken opmodels.OperationsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	pageSize := pagination.SanePageSize(req.GetPaging().GetPageSize())

	args := models.ListOperationsArgs{
		Limit: optional.NewInt64(pageSize),
	}
	if pageToken.OperationID != "" {
		args.PageTokenID = optional.NewString(pageToken.OperationID)
	}
	if !pageToken.OperationIDCreatedAt.IsZero() {
		args.PageTokenCreateTS = optional.NewTime(pageToken.OperationIDCreatedAt)
	}

	operations, err := os.ops.OperationsByFolderID(ctx, req.ProjectId, args)
	if err != nil {
		return nil, err
	}

	opPageToken := models.NewOperationsPageToken(operations, pageSize)
	nextPageToken, err := api.BuildPageTokenToGRPC(opPageToken, false)
	if err != nil {
		return nil, err
	}

	opsGrpc, err := datacloudgrpc.OperationsToGRPC(ctx, operations, req.ProjectId, os.l)
	if err != nil {
		return nil, err
	}

	return &chv1.ListOperationsResponse{
		Operations: opsGrpc,
		NextPage:   &datacloud.NextPage{Token: nextPageToken},
	}, nil
}
