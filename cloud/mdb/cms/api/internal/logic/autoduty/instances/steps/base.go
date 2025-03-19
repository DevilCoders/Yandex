package steps

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

//go:generate ../../../../../../../scripts/mockgen.sh InstanceStep

type RunResult struct {
	Description    string
	Error          error
	IsDone         bool
	Log            []string
	StepName       string
	FinishWorkflow bool
}

const (
	StepNameWhipMaster     = "whip master"
	StepNameCheckIfPrimary = "check if primary"
)

// All steps MUST be idempotent. They will be excuted EVERY run of operation.
type InstanceStep interface {
	Name() string
	RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) RunResult
}

func RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger, step InstanceStep) RunResult {
	span, ctx := opentracing.StartSpanFromContext(ctx, "RunStep", tags.StepName.Tag(step.Name()))
	defer span.Finish()
	lCtx := ctxlog.WithFields(ctx, log.String("stepName", step.Name()))
	ctxlog.Debug(lCtx, l, "Start step")
	result := step.RunStep(lCtx, stepCtx, l)
	result.StepName = step.Name()
	ctxlog.Debug(lCtx, l, "Finish step", log.String("result", result.Description))
	return result
}

func (r *RunResult) AddLog(s string) {
	r.Description = s
	r.Log = append(r.Log, s)
}
