package metadb

import (
	"context"
	"io"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/models"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

//go:generate ../../../../scripts/mockgen.sh Backend

type Backend interface {
	ready.Checker
	io.Closer
	sqlutil.TxBinder

	CreateTask(ctx context.Context, args models.CreateTaskArgs) (models.Operation, error)
	Task(ctx context.Context, taskID string) (models.Operation, error)
	LockCluster(ctx context.Context, cid, reqid string) (int64, error)
	CompleteClusterChange(ctx context.Context, cid string, revision int64) error
}
