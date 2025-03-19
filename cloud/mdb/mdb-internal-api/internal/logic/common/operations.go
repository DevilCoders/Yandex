package common

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

type Operations interface {
	Operation(ctx context.Context, opID string) (operations.Operation, error)
	OperationWithFolderCoords(ctx context.Context, opID string) (operations.Operation, metadb.FolderCoords, error)
	OperationsByFolderID(ctx context.Context, folderExtID string, args models.ListOperationsArgs) ([]operations.Operation, error)
	OperationsByClusterID(ctx context.Context, cid string, limit, offset int64) ([]operations.Operation, error)
}
