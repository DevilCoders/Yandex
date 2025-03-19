package workeremulation

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log/nop"
)

var (
	queryAcquireTask = sqlutil.Stmt{
		Name:  "AcquireTask",
		Query: "SELECT code.acquire_task('dummy', :task_id)",
	}
	queryFinishTask = sqlutil.Stmt{
		Name:  "FinishTask",
		Query: "SELECT code.finish_task('dummy', :task_id, :res, CAST('{}' AS jsonb), '')",
	}
)

type Worker struct {
	name string
	mdb  *sqlutil.Cluster
}

func New(name string, mdb *sqlutil.Cluster) *Worker {
	return &Worker{
		name: name,
		mdb:  mdb,
	}
}

func (w *Worker) AcquireTask(ctx context.Context, id string) error {
	_, err := sqlutil.QueryContext(
		ctx,
		w.mdb.PrimaryChooser(),
		queryAcquireTask,
		map[string]interface{}{"task_id": id},
		sqlutil.NopParser,
		&nop.Logger{},
	)
	return err
}

func (w *Worker) CompleteTask(ctx context.Context, id string, res bool) error {
	_, err := sqlutil.QueryContext(
		ctx,
		w.mdb.PrimaryChooser(),
		queryFinishTask,
		map[string]interface{}{"task_id": id, "res": res},
		sqlutil.NopParser,
		&nop.Logger{},
	)
	return err
}

func (w *Worker) AcquireAndSuccessfullyCompleteTask(ctx context.Context, id string) error {
	if err := w.AcquireTask(ctx, id); err != nil {
		return err
	}
	return w.CompleteTask(ctx, id, true)
}
