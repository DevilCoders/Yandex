package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type MoveInstance struct {
	client    tasksclient.Client
	isCompute bool
}

func (s MoveInstance) Name() string {
	return "resetup instance"
}

func (s MoveInstance) RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) RunResult {
	intOpID := stepCtx.State().MoveInstanceStep.TaskID
	if intOpID == "" {
		var from string
		if s.isCompute {
			from = stepCtx.InstanceID()
		} else {
			from = stepCtx.Dom0FQDN()
		}

		intOpID, err := s.client.CreateMoveInstanceTask(ctx, stepCtx.FQDN(), from)
		if err != nil {
			l.Error("can not create worker task", log.Error(err))
			return waitWithErrAndMessage(err, "can not create worker task")
		} else {
			stepCtx.State().MoveInstanceStep.TaskID = intOpID
			return waitWithMessageFmt("created worker task %q", intOpID)
		}
	}

	status, err := s.client.TaskStatus(ctx, intOpID)
	if err != nil {
		ctxlog.Error(ctx, l, "can not get task", log.Error(err), log.String("intOpID", intOpID))
		return waitWithErrAndMessage(err, "can not get task")
	}

	if status == models.TaskStatusDone {
		return continueWithMessageFmt("instance was successfully moved by worker task %q", intOpID)
	}

	if status == models.TaskStatusFailed {
		return waitWithMessageFmt("worker task %q has been failed, step is failed", intOpID)
	}

	return waitWithMessageFmt("worker task %q is not finished yet", intOpID)
}

func NewMoveInstance(client tasksclient.Client, isCompute bool) MoveInstance {
	return MoveInstance{
		client:    client,
		isCompute: isCompute,
	}
}
