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

type ApplyAction struct {
	actions.BaseAction
}

type ApplyParams struct {
	ID string
}

func (ltvgp *ApplyParams) validate() error {
	return valid.Struct(ltvgp,
		valid.Value(&ltvgp.ID, rule.Required),
	)
}

type ApplyResult struct {
	Template        *license.Template
	TemplateVersion *license.TemplateVersion
}

func NewApplyAction(env *env.Env) *ApplyAction {
	return &ApplyAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *ApplyAction) checkParams(ctx context.Context, params ApplyParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template version apply params did not pass validation")
		return err
	}

	ctxtools.Logger(ctx).Debug("license template version apply params passed validation")
	return nil
}

func (a *ApplyAction) Do(ctx context.Context, params ApplyParams) (*ApplyResult, error) {
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
		return nil, errors.ErrLicenseTemplateVersionNotPending
	}

	lt, err := a.Adapters().DB().GetLicenseTemplateByID(ctx, ltv.TemplateID, tx)
	if err != nil {
		return nil, err
	}
	if lt.State != license.PendingTemplateState && lt.State != license.ActiveTemplateState {
		return nil, errors.ErrLicenseTemplateNotPendingAndActive
	}

	// TODO: do this then main license servere will tested
	// sku, err := a.Adapters().Billing().CreateSku(ctx, adapters.CreateSkuParams{})
	// if err != nil {
	// 	return nil, err
	// }
	sku := &model.Sku{ID: "sku_id"}

	timeNow := utils.GetTimeNow()

	var ltvOld *license.TemplateVersion
	if lt.TemplateVersionID != "" {
		ltvOld, err = a.Adapters().DB().GetLicenseTemplateVersionByID(ctx, lt.TemplateVersionID, tx)
		if err != nil {
			return nil, err
		}
		ltvOld.State = license.DeprecatedTemplateVersionState
		ltvOld.UpdatedAt = timeNow
	}

	ltv.State = license.ActiveTemplateVersionState
	ltv.LicenseSkuID = sku.ID

	lt.TemplateVersionID = ltv.ID
	lt.LicenseSkuID = ltv.LicenseSkuID
	lt.Name = ltv.Name
	lt.Period = ltv.Period
	lt.UpdatedAt = timeNow
	lt.State = license.ActiveTemplateState

	err = a.Adapters().DB().UpsertLicenseTemplate(ctx, lt, tx)
	if err != nil {
		return nil, err
	}

	err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltv, tx)
	if err != nil {
		return nil, err
	}

	if ltvOld != nil {
		err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltvOld, tx)
		if err != nil {
			return nil, err
		}
	}

	return &ApplyResult{
		Template:        lt,
		TemplateVersion: ltv,
	}, nil
}
