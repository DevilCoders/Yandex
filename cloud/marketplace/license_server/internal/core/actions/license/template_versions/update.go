package templateversions

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/errors"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type UpdateAction struct {
	actions.BaseAction
}

type UpdateParams struct {
	ID     string
	Price  map[string]string
	Period string
	Name   string
}

func (ltvcp *UpdateParams) validate() error {
	return valid.Struct(ltvcp,
		valid.Value(&ltvcp.ID, rule.Required),
		valid.Value(&ltvcp.Price, rule.Required),
		valid.Value(&ltvcp.Period, rule.Required),
		valid.Value(&ltvcp.Name, rule.Required),
	)
}

type UpdateResult struct {
	TemplateVersion *license.TemplateVersion
}

func NewUpdateAction(env *env.Env) *UpdateAction {
	return &UpdateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *UpdateAction) checkParams(ctx context.Context, params UpdateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template version update params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license template version update params passed validation")
	return nil
}

func (a *UpdateAction) Do(ctx context.Context, params UpdateParams) (*UpdateResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	ltv, err := a.Adapters().DB().GetLicenseTemplateVersionByID(ctx, params.ID, tx)
	if err != nil {
		return nil, err
	}
	if ltv.State != license.PendingTemplateVersionState {
		err = errors.ErrLicenseTemplateVersionNotPending
		return nil, err
	}

	timeNow := utils.GetTimeNow()
	period, err := model.NewPeriod(params.Period)
	if err != nil {
		return nil, err
	}

	ltv.Price = params.Price
	ltv.Name = params.Name
	ltv.Period = period
	ltv.UpdatedAt = timeNow

	err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltv, tx)
	if err != nil {
		return nil, err
	}

	return &UpdateResult{
		TemplateVersion: ltv,
	}, nil
}
