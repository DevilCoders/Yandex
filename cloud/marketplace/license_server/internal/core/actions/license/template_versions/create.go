package templateversions

import (
	"context"
	errors_is "errors"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/errors"
	ydb_adapter "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/db/ydb"
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
	TemplateID string
	Price      map[string]string
	Period     string
	Name       string
}

func (ltvcp *CreateParams) validate() error {
	return valid.Struct(ltvcp,
		valid.Value(&ltvcp.TemplateID, rule.Required),
		valid.Value(&ltvcp.Period, rule.Required),
		valid.Value(&ltvcp.Name, rule.Required),
	)
}

type CreateResult struct {
	TemplateVersion *license.TemplateVersion
}

func NewCreateAction(env *env.Env) *CreateAction {
	return &CreateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *CreateAction) checkParams(ctx context.Context, params CreateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license template version create params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license template version create params passed validation")
	return nil
}

func (a *CreateAction) Do(ctx context.Context, params CreateParams) (*CreateResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	lt, err := a.Adapters().DB().GetLicenseTemplateByID(ctx, params.TemplateID, tx)
	if err != nil {
		return nil, err
	}

	pendingLtv, err := a.Adapters().DB().GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx, lt.ID, tx)
	if errors_is.Is(err, ydb_adapter.ErrNotFoundLicenseTemplateVersion) {
		err = nil
	} else {
		if err != nil {
			return nil, err
		}
		if pendingLtv != nil {
			return nil, errors.ErrLicenseTemplateVersionIsAlreadyExists
		}
	}

	id := a.CloudIDGenerator().GenerateCloudID()
	timeNow := utils.GetTimeNow()
	period, err := model.NewPeriod(params.Period)
	if err != nil {
		return nil, err
	}

	ltv := &license.TemplateVersion{
		ID:         id,
		TemplateID: lt.ID,
		Price:      params.Price,
		Name:       params.Name,
		Period:     period,
		CreatedAt:  timeNow,
		UpdatedAt:  timeNow,
		State:      license.PendingTemplateVersionState,
	}

	err = a.Adapters().DB().UpsertLicenseTemplateVersion(ctx, ltv, tx)
	if err != nil {
		return nil, err
	}

	return &CreateResult{
		TemplateVersion: ltv,
	}, nil
}
