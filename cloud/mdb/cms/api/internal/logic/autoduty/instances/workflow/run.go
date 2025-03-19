package workflow

import (
	"context"
	"sync"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func logUnknownLetterBeforeReturn(ctx context.Context, logger log.Logger, status models.InstanceOperationStatus, letter Letter) {
	ctxlog.Error(ctx, logger, "Undefined behavior for for tuple (status, letter)", log.String("status", string(status)), log.String("letter", string(letter)))
}

func RunWorkflow(ctx context.Context, wg *sync.WaitGroup, opCtx *opcontext.OperationContext, wf Workflow) {
	defer wg.Done()
	span, ctx := opentracing.StartSpanFromContext(ctx, "RunOperation",
		tags.CmsOperationID.Tag(opCtx.OperationID()),
		tags.CmsOperationType.Tag(string(opCtx.OperationType())),
		tags.InstanceID.Tag(opCtx.InstanceID()),
	)
	defer span.Finish()
	logger := wf.L()
	ctxlog.Info(ctx, logger, "Start workflow")
	defer ctxlog.Info(ctx, logger, "Finish workflow")

	opStatus := opCtx.GetStatus()
	if opStatus != models.InstanceOperationStatusOkPending && opStatus != models.InstanceOperationStatusRejectPending {
		opCtx.SetInProgress()
	}

	opStatus = opCtx.GetStatus()
	if opStatus == models.InstanceOperationStatusInProgress {
		for _, step := range wf.Steps() {
			result := steps.RunStep(ctx, opCtx, logger, step)
			wf.AddResult(result)

			if result.Error != nil {
				ctxlog.Error(ctx, logger, "Step finished with error, stop workflow", log.Error(result.Error))
				break
			}

			if !result.IsDone {
				ctxlog.Info(ctx, logger, "Step needs more time, stop workflow")
				break
			}

			if result.FinishWorkflow {
				ctxlog.Infof(ctx, logger, "Workflow will be finished")
				break
			}
		}
		switch l := wf.LetterAfterProcessingInProgress(); l {
		case OKPending:
			opCtx.SetOKPending()
		case RejectPending:
			opCtx.SetRejectPending()
		case InProgress:
			// do nothing
		default:
			logUnknownLetterBeforeReturn(ctx, logger, opStatus, l)
			return
		}
	}

	opStatus = opCtx.GetStatus()
	if opStatus == models.InstanceOperationStatusOkPending || opStatus == models.InstanceOperationStatusRejectPending {
		ctxlog.Info(ctx, logger, "Start cleanup steps")
		for _, step := range wf.CleanupSteps() {
			result := steps.RunStep(ctx, opCtx, logger, step)
			wf.AddCleanupResult(result)

			if result.Error != nil {
				ctxlog.Error(ctx, logger, "Step finished with error, it can't be cleaned up", log.Error(result.Error))
			}

			if !result.IsDone {
				ctxlog.Info(ctx, logger, "Step needs more time.")
				break
			}
		}
		if opStatus == models.InstanceOperationStatusOkPending {
			switch l := wf.LetterAfterCleanupInOKPending(); l {
			case OK:
				opCtx.SetOk()
			case Reject:
				opCtx.SetRejected()
			case OKPending:
				// do nothing
			default:
				logUnknownLetterBeforeReturn(ctx, logger, opStatus, l)
				return
			}
		} else if opStatus == models.InstanceOperationStatusRejectPending {
			switch l := wf.LetterAfterCleanupInRejectPending(); l {
			case Reject:
				opCtx.SetRejected()
			case RejectPending:
				// do nothing
			default:
				logUnknownLetterBeforeReturn(ctx, logger, opStatus, l)
				return
			}
		}
	}

	opCtx.SetLog(wf.StringLog())
	opCtx.SetExplanation(wf.LastResult().Description)
	opCtx.SetStepNames(wf.ExecutedStepNames())

	err := wf.Commit(ctx, opCtx)
	if err != nil {
		ctxlog.Error(ctx, logger, "We couldn't store data in cmsdb, it's a real bug. This is the way.", log.Error(err))
		return
	}
}
