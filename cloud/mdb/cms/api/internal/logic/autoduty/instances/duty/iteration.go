package duty

import (
	"context"
	"sync"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/workflow"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ad *AutoDuty) Iteration(ctx context.Context) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "InstanceAutodutyIteration")
	defer span.Finish()
	ad.log.Info("Start new iteration")
	defer ad.log.Info("Finish iteration")

	ctx, err := ad.cmsdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		ad.log.Error("Can't create tx", log.Error(err))
		return
	}

	defer func() { _ = ad.cmsdb.Rollback(ctx) }()

	if err = ad.cmsdb.GetLock(ctx, cmsdb.InstanceAutodutyLockKey); err != nil {
		if xerrors.Is(err, cmsdb.ErrLockNotTaken) {
			ad.log.Info("Lock is already taken by another worker")
		}
		ad.log.Error("Can not get lock", log.Error(err))
		return
	}

	ops, err := ad.cmsdb.InstanceOperationsToProcess(ctx, ad.maxConcurrentTasks)
	if err != nil {
		ad.log.Error("Can't get operations", log.Error(err))
		return
	}

	var wg sync.WaitGroup
	for _, op := range ops {
		var wf workflow.Workflow
		switch op.Type {
		case models.InstanceOperationMove:
			wf = ad.moveWorkflowFactory()
		case models.InstanceOperationWhipPrimaryAway:
			wf = ad.whipPrimaryWorkflowFactory()
		default:
			ad.log.Error(
				"unsupported operation type",
				log.String("type", string(op.Type)),
				log.String("operationID", op.ID),
				log.String("instanceID", op.InstanceID),
				log.String("author", string(op.Author)),
				log.Time("createdAt", op.CreatedAt),
			)
			continue
		}
		stpctx := opcontext.NewStepContext(op)
		wg.Add(1)
		opCtx := ctxlog.WithFields(
			ctx,
			log.String("instanceID", stpctx.InstanceID()),
			log.String("operationID", stpctx.OperationID()),
			log.String("workflowName", wf.Name()),
		)
		go workflow.RunWorkflow(opCtx, &wg, stpctx, wf)
	}
	wg.Wait()

	if err = ad.cmsdb.Commit(ctx); err != nil {
		ad.log.Error("Can't commit iteration result", log.Error(err))
		return
	}
}
