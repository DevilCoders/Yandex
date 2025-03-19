package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/log"
)

// FormatSchemaService implements gRPC methods for format schema management.
type FormatSchemaService struct {
	chv1.UnimplementedFormatSchemaServiceServer

	l  log.Logger
	ch clickhouse.ClickHouse
}

var _ chv1.FormatSchemaServiceServer = &FormatSchemaService{}

func NewFormatSchemaService(ch clickhouse.ClickHouse, l log.Logger) *FormatSchemaService {
	return &FormatSchemaService{ch: ch, l: l}
}

// Get Returns the specified ClickHouse format schema.
func (fss *FormatSchemaService) Get(ctx context.Context, req *chv1.GetFormatSchemaRequest) (*chv1.FormatSchema, error) {
	formatSchema, err := fss.ch.FormatSchema(ctx, req.GetClusterId(), req.GetFormatSchemaName())
	if err != nil {
		return nil, err
	}

	return FormatSchemaToGRPC(formatSchema)
}

func (fss *FormatSchemaService) List(ctx context.Context, req *chv1.ListFormatSchemasRequest) (*chv1.ListFormatSchemasResponse, error) {
	var pageToken pagination.OffsetPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListFormatSchemasResponse{}, err
	}

	formatSchemas, formatSchemaPageToken, err := fss.ch.FormatSchemas(ctx, req.GetClusterId(), req.GetPageSize(), pageToken.Offset)
	if err != nil {
		return &chv1.ListFormatSchemasResponse{}, err
	}

	grpcFormatSchemas, err := FormatSchemasToGRPC(formatSchemas)
	if err != nil {
		return &chv1.ListFormatSchemasResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(formatSchemaPageToken, false)
	if err != nil {
		return &chv1.ListFormatSchemasResponse{}, err
	}

	return &chv1.ListFormatSchemasResponse{
		FormatSchemas: grpcFormatSchemas,
		NextPageToken: nextPageToken,
	}, nil
}

func (fss *FormatSchemaService) Create(ctx context.Context, req *chv1.CreateFormatSchemaRequest) (*operation.Operation, error) {
	fsType, err := FormatSchemaTypeFromGRPC(req.GetType())
	if err != nil {
		return nil, err
	}

	op, err := fss.ch.CreateFormatSchema(ctx, req.GetClusterId(), req.GetFormatSchemaName(), fsType, req.GetUri())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, fss.l)
}

func (fss *FormatSchemaService) Update(ctx context.Context, req *chv1.UpdateFormatSchemaRequest) (*operation.Operation, error) {
	op, err := fss.ch.UpdateFormatSchema(ctx, req.GetClusterId(), req.GetFormatSchemaName(), req.GetUri())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, fss.l)
}

func (fss *FormatSchemaService) Delete(ctx context.Context, req *chv1.DeleteFormatSchemaRequest) (*operation.Operation, error) {
	op, err := fss.ch.DeleteFormatSchema(ctx, req.GetClusterId(), req.GetFormatSchemaName())
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, fss.l)
}
