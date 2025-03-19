package templateversions

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/errors"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

// delete license template version only if it deprecated
type DeleteAction struct {
	actions.BaseAction
}

type DeleteParams struct {
	ID string
}

func (ltvgp *DeleteParams) validate() error {
	return valid.Struct(ltvgp,
		valid.Value(&ltvgp.ID, rule.Required),
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
		ctxtools.Logger(ctx).Debug("license template version delete params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license template version delete params passed validation")
	return nil
}

func (a *DeleteAction) Do(ctx context.Context, params DeleteParams) error {
	if err := a.checkParams(ctx, params); err != nil {
		return err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	ltv, err := a.Adapters().DB().GetLicenseTemplateVersionByID(ctx, params.ID, tx)
	if err != nil {
		return err
	}
	if ltv.State != license.DeprecatedTemplateVersionState {
		err = errors.ErrLicenseTemplateVersionNotDeprecated
		return err
	}

	ltv.State = license.DeletedTemplateVersionState

	err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltv, tx)
	if err != nil {
		return err
	}

	return nil
}
