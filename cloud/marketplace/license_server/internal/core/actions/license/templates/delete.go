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

type DeleteAction struct {
	actions.BaseAction
}

type DeleteParams struct {
	ID string
}

func (ltdp *DeleteParams) validate() error {
	return valid.Struct(ltdp,
		valid.Value(&ltdp.ID, rule.Required),
	)
}

func NewDeleteAction(env *env.Env) *DeleteAction {
	return &DeleteAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *DeleteAction) checkParams(ctx context.Context, params DeleteParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template delete params did not pass validation")
		return err
	}

	ctxtools.Logger(ctx).Debug("license template delete params passed validation")
	return nil
}

func (a *DeleteAction) Do(ctx context.Context, params DeleteParams) error {
	if err := a.checkParams(ctx, params); err != nil {
		return err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	lt, err := a.Adapters().DB().GetLicenseTemplateByID(ctx, params.ID, tx)
	if err != nil {
		return err
	}

	timeNow := utils.GetTimeNow()

	var ltv *license.TemplateVersion
	if lt.TemplateVersionID != "" {
		ltv, err = a.Adapters().DB().GetLicenseTemplateVersionByID(ctx, lt.TemplateVersionID, tx)
		if err != nil {
			return err
		}
		ltv.State = license.DeprecatedTemplateVersionState
		ltv.UpdatedAt = timeNow
	}

	var pendingLtv *license.TemplateVersion
	if ltv == nil || ltv.State != license.PendingTemplateVersionState {
		pendingLtv, err = a.Adapters().DB().GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx, lt.ID, tx)
		if errors.Is(err, ydb_errors.ErrNotFoundLicenseTemplateVersion) {
			err = nil
		}
		if err != nil {
			return err
		}
		if pendingLtv != nil {
			pendingLtv.State = license.DeprecatedTemplateVersionState
			pendingLtv.UpdatedAt = timeNow
		}
	}

	lt.State = license.DeletedTemplateState
	lt.UpdatedAt = timeNow

	err = a.Adapters().DB().UpsertLicenseTemplate(ctx, lt, tx)
	if err != nil {
		return err
	}

	if ltv != nil {
		err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltv, tx)
		if err != nil {
			return err
		}
	}

	if pendingLtv != nil {
		err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, pendingLtv, tx)
		if err != nil {
			return err
		}
	}

	return nil
}
