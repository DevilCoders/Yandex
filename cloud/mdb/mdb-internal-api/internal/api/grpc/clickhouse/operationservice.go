package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	opmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// OperationService implements operation-specific gRPC methods
type OperationService struct {
	chv1.UnimplementedOperationServiceServer

	ch            clickhouse.ClickHouse
	ops           common.Operations
	saltEnvMapper grpcapi.SaltEnvMapper
	l             log.Logger
}

var _ chv1.OperationServiceServer = &OperationService{}

func NewOperationService(ops common.Operations, ch clickhouse.ClickHouse, saltEnvsCfg logic.SaltEnvsConfig, l log.Logger) *OperationService {
	return &OperationService{
		ch:  ch,
		ops: ops,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(chv1.Cluster_PRODUCTION),
			int64(chv1.Cluster_PRESTABLE),
			int64(chv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
		l: l,
	}
}

func (os *OperationService) Get(ctx context.Context, req *chv1.GetOperationRequest) (*operation.Operation, error) {
	op, err := os.ops.Operation(ctx, req.GetOperationId())
	if err != nil {
		return nil, err
	}

	db, err := op.Type.Database()
	if err != nil {
		return nil, err
	}
	if db != opmodels.DatabaseClickhouse {
		return nil, xerrors.Errorf("wrong operation database %+v, must be clickhouse operation", db)
	}

	return operationToGRPC(ctx, op, os.ch, os.saltEnvMapper, os.l)
}

func (os *OperationService) List(ctx context.Context, req *chv1.ListOperationsRequest) (*chv1.ListOperationsResponse, error) {
	var pageToken opmodels.OperationsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}

	pageSize := pagination.SanePageSize(req.GetPageSize())

	args := models.ListOperationsArgs{
		Limit:       optional.NewInt64(pageSize),
		ClusterType: clusters.TypeClickHouse,
	}
	if pageToken.OperationID != "" {
		args.PageTokenID = optional.NewString(pageToken.OperationID)
	}
	if !pageToken.OperationIDCreatedAt.IsZero() {
		args.PageTokenCreateTS = optional.NewTime(pageToken.OperationIDCreatedAt)
	}

	operations, err := os.ops.OperationsByFolderID(ctx, req.FolderId, args)
	if err != nil {
		return nil, err
	}

	opPageToken := models.NewOperationsPageToken(operations, pageSize)
	nextPageToken, err := api.BuildPageTokenToGRPC(opPageToken, false)
	if err != nil {
		return nil, err
	}

	ops, err := OperationsToGRPC(ctx, operations, os.ch, os.saltEnvMapper, os.l)
	if err != nil {
		return nil, err
	}

	return &chv1.ListOperationsResponse{
		Operations:    ops,
		NextPageToken: nextPageToken,
	}, nil
}
