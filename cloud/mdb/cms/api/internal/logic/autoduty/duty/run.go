package duty

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func (ad *AutoDuty) Run(ctx context.Context) error {
	span, ctx := opentracing.StartSpanFromContext(ctx, "WalleAutodutyIteration")
	defer span.Finish()
	if err := ad.ProcessPendingRequests(ctx,
		ad.rdsToReturnFromWalle,
		ad.toReturn,
		func(d *models.AutomaticDecision, log string) {},
		0,
		"not_returned_yet",
	); err != nil {
		return err
	}

	if err := ad.ProcessPendingRequests(ctx,
		ad.rdsToAnalyse,
		ad.analysis,
		func(d *models.AutomaticDecision, log string) { d.AnalysisLog = log },
		0,
		"analyse",
	); err != nil {
		return err
	}

	if err := ad.ProcessPendingRequests(ctx,
		ad.rdsToLetTo,
		ad.letGo,
		func(d *models.AutomaticDecision, log string) { d.MutationsLog = log },
		ad.maxSimultaneousLetGoRequests,
		"let go",
	); err != nil {
		return err
	}

	if err := ad.ProcessPendingRequests(ctx,
		ad.rdsToFinish,
		ad.afterWalle,
		func(d *models.AutomaticDecision, log string) { d.AfterWalleLog = log },
		0,
		"finish",
	); err != nil {
		return err
	}

	if err := ad.ProcessPendingRequests(ctx,
		ad.rdsToCleanup,
		ad.cleanup,
		func(d *models.AutomaticDecision, log string) { d.CleanupLog = log },
		0,
		"cleanup",
	); err != nil {
		return err
	}
	return nil
}
