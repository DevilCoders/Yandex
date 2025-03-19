package mongodb

import (
	"context"

	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
	"a.yandex-team.ru/library/go/core/log"
)

// BackupService implements gRPC methods for user management.
type BackupService struct {
	mongov1.UnimplementedBackupServiceServer

	mongo mongodb.MongoDB
	L     log.Logger
}

var _ mongov1.BackupServiceServer = &BackupService{}

func NewBackupService(mongo mongodb.MongoDB, l log.Logger) *BackupService {
	return &BackupService{mongo: mongo, L: l}
}

// Deletes specified MongoDB backup
func (bs *BackupService) Delete(ctx context.Context, req *mongov1.DeleteBackupRequest) (*operation.Operation, error) {
	op, err := bs.mongo.DeleteBackup(
		ctx,
		req.GetBackupId(),
	)
	if err != nil {
		return nil, err
	}

	return grpcapi.OperationToGRPC(ctx, op, bs.L)
}
