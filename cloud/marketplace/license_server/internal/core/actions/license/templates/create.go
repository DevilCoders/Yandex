package templates

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/errors"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/marketplace"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type CreateAction struct {
	actions.BaseAction
}

type CreateParams struct {
	PublisherID string
	ProductID   string
	TariffID    string
}

func (p *CreateParams) validate() error {
	return valid.Struct(p,
		valid.Value(&p.ProductID, rule.Required),
		valid.Value(&p.PublisherID, rule.Required),
		valid.Value(&p.TariffID, rule.Required),
	)
}

type CreateResult struct {
	Template *license.Template
}

func NewCreateAction(env *env.Env) *CreateAction {
	return &CreateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *CreateAction) checkParams(ctx context.Context, params CreateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template create params did not pass validation")
		return err
	}

	ctxtools.Logger(ctx).Debug("license template create params passed validation")
	return nil
}

func (a *CreateAction) Do(ctx context.Context, params CreateParams) (*CreateResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tariff, err := a.Adapters().Marketplace().GetTariff(ctx, marketplace.GetTariffParams{
		PublisherID: params.PublisherID,
		ProductID:   params.ProductID,
		TariffID:    params.TariffID,
	})
	if err != nil {
		return nil, err
	}

	if tariff.State != model.ActiveTariffState && tariff.State != model.PendingTariffState {
		err = errors.ErrTariffNotActiveOrPending
		return nil, err
	}
	if tariff.Type != model.PAYGTariffType {
		err = errors.ErrTariffNotPAYG
		return nil, err
	}

	id := a.CloudIDGenerator().GenerateCloudID()
	timeNow := utils.GetTimeNow()

	lt := &license.Template{
		ID:          id,
		Period:      model.DefaultPeriod(),
		PublisherID: tariff.PublisherID,
		ProductID:   tariff.ProductID,
		TariffID:    tariff.ID,
		CreatedAt:   timeNow,
		UpdatedAt:   timeNow,
		State:       license.PendingTemplateState,
	}

	err = a.Adapters().DB().UpsertLicenseTemplate(ctx, lt, nil)
	if err != nil {
		return nil, err
	}

	return &CreateResult{
		Template: lt,
	}, nil
}
