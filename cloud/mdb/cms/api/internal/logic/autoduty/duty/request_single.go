package duty

import (
	"context"
	"fmt"
	"strconv"
	"time"

	"github.com/opentracing/opentracing-go"
	tracelog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/dlog"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instructions"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/statemachine"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func (ad *AutoDuty) DecideSingleRequest(
	ctx context.Context,
	execCtx *steps.InstructionCtx,
	inst *instructions.Instructions,
	updLog func(d *models.AutomaticDecision, log string),
	toProcessFunc func(ctx context.Context, execCtx *steps.InstructionCtx) (*types.RequestDecisionTuple, error),
) error {
	span, ctx := opentracing.StartSpanFromContext(ctx, "DecideSingleRequest")
	defer span.Finish()
	ctx, err := ad.cmsdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return xerrors.Errorf("start tx: %w", err)
	}
	defer func() { _ = ad.cmsdb.Rollback(ctx) }()

	rd, err := toProcessFunc(ctx, execCtx)
	if err != nil {
		return xerrors.Errorf("get request-decision to process: %w", err)
	}

	tags.CMSRequestID.Set(span, rd.R.ExtID)
	tags.DecisionID.Set(span, strconv.FormatInt(rd.D.ID, 10))
	execCtx.SetActualRD(rd)
	startedAt := time.Now()
	ctx = ctxlog.WithFields(
		ctx,
		log.String("request_ext_id", rd.R.ExtID),
		log.Int64("decision_id", rd.D.ID),
	)
	step := inst.Find(rd.R).EntryPoint
	dutyLog := dlog.NewDecisionLog()
	// "workflow" is a stack
	workflow := []steps.DecisionStep{step}

	if rd.D.Status == models.DecisionNone || rd.D.Status == models.DecisionWait || rd.D.Status == models.DecisionEscalate {
		if err := ad.sm.TransitToState(ctx, rd, statemachine.Input{
			Action: steps.ProcessMe,
		}); err != nil {
			ctxlog.Error(ctx, ad.log, fmt.Sprintf("can not transit request to action %v", steps.ProcessMe), log.Error(err))
			return xerrors.Errorf("transit request to action %v: %w", steps.ProcessMe, err)
		}
	}

	origCtx := ctx
	for len(workflow) > 0 {
		step = workflow[len(workflow)-1]
		ctx = origCtx // to save orig ctx log fields

		ctx = ctxlog.WithFields(ctx,
			log.String("step", step.GetStepName()),
			log.Int("step_number", len(workflow)))

		ctxlog.Info(ctx, ad.log, fmt.Sprintf(
			"Step '%s' started", step.GetStepName()))
		res := func() steps.RunResult {
			stepSpan, ctx := opentracing.StartSpanFromContext(ctx, "RunStep", tags.StepName.Tag(step.GetStepName()))
			defer stepSpan.Finish()
			res := step.RunStep(ctx, execCtx)
			stepSpan.LogFields(
				tracelog.Error(res.Error),
				tracelog.Message(res.ForHuman),
				tracelog.String("resolution", string(res.Action)),
			)
			return res
		}()
		workflow = workflow[:len(workflow)-1]
		ctx = ctxlog.WithFields(
			ctx,
			log.String("resolution", string(res.Action)))
		if res.Error != nil {
			ctx = ctxlog.WithFields(
				ctx,
				log.Error(res.Error))
			ctxlog.Error(ctx, ad.log, fmt.Sprintf(
				"Step '%s' reported error to log: %v", step.GetStepName(), res.Error))
		}
		ctxlog.Info(ctxlog.WithFields(ctx, log.Bool("has_resolution", true)), ad.log, fmt.Sprintf(
			"Step '%s' resulted in '%s' and sent message for human: '%s'", step.GetStepName(), string(res.Action), res.ForHuman))

		dutyLog.Add(dlog.LogEntry{
			Result: res,
			Step:   step,
		})

		addedStepsCnt := len(res.AfterMeSteps)
		if addedStepsCnt > 0 {
			// in depth tree traversal of conditions
			// "workflow" is stack
			tmp := make([]steps.DecisionStep, addedStepsCnt)
			copy(tmp, res.AfterMeSteps)
			slices.Reverse(tmp)
			workflow = append(workflow, tmp...)
		}

		if res.Action != steps.AfterStepContinue {
			// we are in terminal state
			updLog(&rd.D, dutyLog.String())
			if err := ad.sm.TransitToState(
				ctx, rd, statemachine.Input{Action: res.Action, MustReview: ad.mustReview}); err != nil {
				ctxlog.Error(ctx, ad.log, fmt.Sprintf("can not transit request to action %v", res.Action), log.Error(err))
				return xerrors.Errorf("transit request to action %v: %w", res.Action, err)
			}
			if err := ad.cmsdb.UpdateDecisionFields(ctx, rd.D); err != nil {
				ctxlog.Error(ctx, ad.log, "can not update decision fields", log.Error(err))
				return xerrors.Errorf("update decision fields: %w", err)
			}
			if err := ad.UpdateExplanations(ctx, rd, dutyLog.ForCMS()); err != nil {
				ctxlog.Error(ctx, ad.log, "can not update explanations", log.Error(err))
				return xerrors.Errorf("update explanations: %w", err)
			}
			break
		}
	}

	dur := time.Since(startedAt)
	ctxlog.Infof(ctx, ad.log,
		fmt.Sprintf("Finished execution at step '%s' in %s", step.GetStepName(), dur),
		log.String("executed_in", dur.String()))

	err = ad.cmsdb.Commit(ctx)
	if err != nil {
		ctxlog.Error(ctx, ad.log, "can not commit work result", log.Error(err))
		return xerrors.Errorf("commit result: %w", err)
	}
	return nil
}
