package duty

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
)

func (ad *AutoDuty) UpdateExplanations(ctx context.Context, rd *types.RequestDecisionTuple, expl string) error {
	r := rd.R
	r.ResolveExplanation = expl
	if err := ad.cmsdb.UpdateRequestFields(ctx, r); err != nil {
		return err
	}
	return nil
}
