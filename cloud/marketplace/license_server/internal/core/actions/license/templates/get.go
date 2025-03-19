package templates

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type GetAction struct {
	actions.BaseAction
}

type GetParams struct {
	ID string
}

func (ltgp *GetParams) validate() error {
	return valid.Struct(ltgp,
		valid.Value(&ltgp.ID, rule.Required),
	)
}

type GetResult struct {
	Template *license.Template
}

func NewGetAction(env *env.Env) *GetAction {
	return &GetAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *GetAction) checkParams(ctx context.Context, params GetParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template get params did not pass validation")
		return err
	}

	ctxtools.Logger(ctx).Debug("license template get params passed validation")
	return nil
}

func (a *GetAction) Do(ctx context.Context, params GetParams) (*GetResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	lt, err := a.Adapters().DB().GetLicenseTemplateByID(ctx, params.ID, nil)
	if err != nil {
		return nil, err
	}

	return &GetResult{
		Template: lt,
	}, nil
}
