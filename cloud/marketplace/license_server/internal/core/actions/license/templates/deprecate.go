package templates

import (
	"context"
	"errors"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	ydb_errors "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type DeprecateAction struct {
	actions.BaseAction
}

type DeprecateParams struct {
	ID string
}

func (ltdp *DeprecateParams) validate() error {
	return valid.Struct(ltdp,
		valid.Value(&ltdp.ID, rule.Required),
	)
}

type DeprecateResult struct {
	Template *license.Template
}

func NewDeprecateAction(env *env.Env) *DeprecateAction {
	return &DeprecateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *DeprecateAction) checkParams(ctx context.Context, params DeprecateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template deprecate params did not pass validation")
		return err
	}

	ctxtools.Logger(ctx).Debug("license template deprecate params passed validation")
	return nil
}

func (a *DeprecateAction) Do(ctx context.Context, params DeprecateParams) (*DeprecateResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	lt, err := a.Adapters().DB().GetLicenseTemplateByID(ctx, params.ID, tx)
	if err != nil {
		return nil, err
	}

	timeNow := utils.GetTimeNow()

	var ltv *license.TemplateVersion
	if lt.TemplateVersionID != "" {
		ltv, err = a.Adapters().DB().GetLicenseTemplateVersionByID(ctx, lt.TemplateVersionID, tx)
		if err != nil {
			return nil, err
		}
		ltv.State = license.DeprecatedTemplateVersionState
		ltv.UpdatedAt = timeNow
	}

	var ltvPending *license.TemplateVersion
	if ltv == nil || ltv.State != license.PendingTemplateVersionState {
		ltvPending, err = a.Adapters().DB().GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx, lt.ID, tx)
		if errors.Is(err, ydb_errors.ErrNotFoundLicenseTemplateVersion) {
			err = nil
		}
		if err != nil {
			return nil, err
		}
		if ltvPending != nil {
			ltvPending.State = license.DeprecatedTemplateVersionState
			ltvPending.UpdatedAt = timeNow
		}
	}

	lt.State = license.DeprecatedTemplateState
	lt.UpdatedAt = timeNow

	err = a.Adapters().DB().UpsertLicenseTemplate(ctx, lt, tx)
	if err != nil {
		return nil, err
	}

	if ltv != nil {
		err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltv, tx)
		if err != nil {
			return nil, err
		}
	}

	if ltvPending != nil {
		err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltvPending, tx)
		if err != nil {
			return nil, err
		}
	}

	return &DeprecateResult{
		Template: lt,
	}, nil
}
