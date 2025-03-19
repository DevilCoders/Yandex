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

type ListByTariffIDAction struct {
	actions.BaseAction
}

type ListByTariffIDParams struct {
	PublisherID string
	ProductID   string
	TariffID    string
}

func (ltgp *ListByTariffIDParams) validate() error {
	return valid.Struct(ltgp,
		valid.Value(&ltgp.PublisherID, rule.Required),
		valid.Value(&ltgp.ProductID, rule.Required),
		valid.Value(&ltgp.TariffID, rule.Required),
	)
}

type ListByTariffIDResult struct {
	Templates []*license.Template
}

func NewListByTariffIDAction(env *env.Env) *ListByTariffIDAction {
	return &ListByTariffIDAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *ListByTariffIDAction) checkParams(ctx context.Context, params ListByTariffIDParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template list by tariff id params did not pass validation")
		return err
	}

	ctxtools.Logger(ctx).Debug("license template list by tariff id params passed validation")
	return nil
}

func (a *ListByTariffIDAction) Do(ctx context.Context, params ListByTariffIDParams) (*ListByTariffIDResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	lts, err := a.Adapters().DB().ListLicenseTemplatesByTariffID(ctx, params.PublisherID, params.ProductID, params.TariffID, nil)
	if err != nil {
		return nil, err
	}

	return &ListByTariffIDResult{
		Templates: lts,
	}, nil
}
