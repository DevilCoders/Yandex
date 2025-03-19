package duty

import (
	"context"
	"fmt"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instructions"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (ad *AutoDuty) ProcessPendingRequests(ctx context.Context,
	toProcessFunc func(ctx context.Context, execCtx *steps.InstructionCtx) (*types.RequestDecisionTuple, error),
	inst *instructions.Instructions,
	updLog func(d *models.AutomaticDecision, log string),
	maxSimultaneousRequests int,
	instructionName string,
) error {
	ctx = ctxlog.WithFields(ctx, log.String("duty_job_type", instructionName))
	sp, ctx := opentracing.StartSpanFromContext(ctx, "ProcessPendingRequests", tags.InstructionName.Tag(instructionName))
	defer sp.Finish()
	walleSeesRequests, err := ad.cmsdb.GetRequests(ctx)
	if err != nil {
		return err
	}
	instrCtx := steps.NewInstructionCtx(walleSeesRequests)

	var i int
	for {
		i += 1
		if maxSimultaneousRequests > 0 && i > maxSimultaneousRequests {
			ctxlog.Infof(ctx, ad.log, "Maximum simultaneous requests count is reached (%d), stop processing", maxSimultaneousRequests)
			break
		}
		err := ad.DecideSingleRequest(ctx, &instrCtx, inst, updLog, toProcessFunc)
		if err != nil {
			if semerr.IsNotFound(err) {
				ctxlog.Info(ctx, ad.log, "Nothing to process")
				break
			}
			ctxlog.Error(ctx, ad.log, fmt.Sprintf("Stopped decision loop at %dth request.", i), log.Error(err))
			return err
		}
	}

	ctxlog.Infof(ctx, ad.log, "Processed %d requests", i-1)
	return nil
}
