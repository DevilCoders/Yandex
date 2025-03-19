package templateversions

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type ListByTemplateIDAction struct {
	actions.BaseAction
}

type ListByTemplateIDParams struct {
	TemplateID string
}

func (ltvgp *ListByTemplateIDParams) validate() error {
	return valid.Struct(ltvgp,
		valid.Value(&ltvgp.TemplateID, rule.Required),
	)
}

type ListByTemplateIDResult struct {
	TemplateVersions []*license.TemplateVersion
}

func NewListByTemplateIDAction(env *env.Env) *ListByTemplateIDAction {
	return &ListByTemplateIDAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *ListByTemplateIDAction) checkParams(ctx context.Context, params ListByTemplateIDParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template version list params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license template version list params passed validation")
	return nil
}

func (a *ListByTemplateIDAction) Do(ctx context.Context, params ListByTemplateIDParams) (*ListByTemplateIDResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	ltvs, err := a.Adapters().DB().ListLicenseTemplateVersionByLicenseTemplateID(ctx, params.TemplateID, tx)
	if err != nil {
		return nil, err
	}

	return &ListByTemplateIDResult{
		TemplateVersions: ltvs,
	}, nil
}
