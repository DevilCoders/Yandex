package main

import (
	"context"
	"fmt"
	"time"

	pb "a.yandex-team.ru/antirobot/captcha_cloud_api/proto"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	smartcaptcha_pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/smartcaptcha/v1"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"github.com/golang/protobuf/proto"
	"github.com/ydb-platform/ydb-go-sdk/v3/table"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type OperationServer struct {
	smartcaptcha_pb.UnimplementedOperationServiceServer
	Server
}

type OperationInfo struct {
	CaptchaID string
	FolderID  string
	Operation *operation.Operation
}

func fillOperationDatabaseRecord(data []byte, op *operation.Operation) error {
	return proto.Unmarshal(data, op)
}

func (server *OperationServer) getBy(ctx context.Context, fieldKey string, fieldValue string, pageSize int64) ([]*OperationInfo, error) {
	readTx := table.TxControl(table.BeginTx(table.WithOnlineReadOnly()), table.CommitTx())
	var res result.Result
	if pageSize < 1 || pageSize > 1000 {
		pageSize = 1000
	}

	err := (*server.YdbConnection).Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) (err error) {
			_, res, err = s.Execute(
				ctx,
				readTx,
				fmt.Sprintf(`-- select
					declare $%[2]v as String;
					declare $page_size as UInt64;

					SELECT operation_id, folder_id, captcha_id, created_at, data
					FROM %[1]v
					WHERE %[2]v == $%[2]v
					ORDER BY created_at DESC
					LIMIT $page_size
				`, "`cloud-operations`", fieldKey),
				table.NewQueryParameters(
					table.ValueParam("$"+fieldKey, types.StringValue([]byte(fieldValue))),
					table.ValueParam("$page_size", types.Uint64Value(uint64(pageSize))),
				),
			)
			return err
		},
		table.WithIdempotent(),
	)
	if err != nil {
		return nil, err
	}
	if res.Err() != nil {
		return nil, res.Err()
	}

	result := []*OperationInfo{}

	for res.NextResultSet(ctx, "operation_id", "folder_id", "captcha_id", "created_at", "data") {
		for res.NextRow() {
			var (
				operationID string
				folderID    string
				captchaID   string
				createdAt   time.Time
				data        string
			)
			err = res.ScanWithDefaults(&operationID, &folderID, &captchaID, &createdAt, &data)
			if err != nil {
				return nil, err
			}

			op := &operation.Operation{}
			err = fillOperationDatabaseRecord([]byte(data), op)
			if err != nil {
				return nil, err
			}

			result = append(result, &OperationInfo{
				CaptchaID: captchaID,
				FolderID:  folderID,
				Operation: op,
			})
		}
	}
	return result, nil
}

func (server *OperationServer) getInternal(ctx context.Context, in *smartcaptcha_pb.GetOperationRequest) (*operation.Operation, error) {
	items, err := server.getBy(ctx, "operation_id", in.OperationId, 1)
	if err != nil {
		return nil, err
	}
	if len(items) == 0 {
		return nil, status.Errorf(codes.NotFound, "Operation does not exists")
	}
	operationInfo := items[0]
	if _, err := server.Authorize(ctx, cloudauth.ResourceFolder(operationInfo.FolderID), CaptchaGetPermission); err != nil {
		return nil, err
	}
	return operationInfo.Operation, nil
}

func (server *OperationServer) Get(ctx context.Context, in *smartcaptcha_pb.GetOperationRequest) (*operation.Operation, error) {
	result, err := server.getInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{OperationServerGetRecord: &pb.TOperationServerGetRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func (server *OperationServer) listInternal(ctx context.Context, in *smartcaptcha_pb.ListOperationsRequest) (*smartcaptcha_pb.ListOperationsResponse, error) {
	if _, err := server.Authorize(ctx, cloudauth.ResourceFolder(in.FolderId), CaptchaGetPermission); err != nil {
		return nil, err
	}

	items, err := server.getBy(ctx, "folder_id", in.FolderId, in.PageSize)
	if err != nil {
		return nil, err
	}
	operations := make([]*operation.Operation, 0, len(items))
	for _, op := range items {
		operations = append(operations, op.Operation)
	}
	return &smartcaptcha_pb.ListOperationsResponse{
		Operations: operations,
	}, nil
}

func (server *OperationServer) List(ctx context.Context, in *smartcaptcha_pb.ListOperationsRequest) (*smartcaptcha_pb.ListOperationsResponse, error) {
	result, err := server.listInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{OperationServerListRecord: &pb.TOperationServerListRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}
