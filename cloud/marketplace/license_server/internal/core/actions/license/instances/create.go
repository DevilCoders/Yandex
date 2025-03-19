package instances

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/errors"
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
	CloudID    string
	Name       string
}

func (licp *CreateParams) validate() error {
	return valid.Struct(licp,
		valid.Value(&licp.TemplateID, rule.Required),
		valid.Value(&licp.CloudID, rule.Required),
	)
}

type CreateResult struct {
	Instance *license.Instance
}

func NewCreateAction(env *env.Env) *CreateAction {
	return &CreateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *CreateAction) checkParams(ctx context.Context, params CreateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license instance create params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license instance create params passed validation")
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

	lt, err := a.Adapters().DB().GetLicenseTemplateByID(ctx, params.TemplateID, nil)
	if err != nil {
		return nil, err
	}

	if lt.State != license.ActiveTemplateState {
		err = errors.ErrLicenseTemplateIsNotActive
		return nil, err
	}

	id := a.CloudIDGenerator().GenerateCloudID()
	timeNow := utils.GetTimeNow()
	timeEnd, err := lt.Period.AddToTime(timeNow)
	if err != nil {
		return nil, err
	}

	name := params.Name
	if name == "" {
		name = lt.Name
	}

	li := &license.Instance{
		ID:                id,
		TemplateID:        lt.ID,
		TemplateVersionID: lt.TemplateVersionID,
		CloudID:           params.CloudID,
		Name:              name,
		StartTime:         timeNow,
		EndTime:           timeEnd,
		CreatedAt:         timeNow,
		UpdatedAt:         timeNow,
		State:             license.PendingInstanceState,
	}

	err = a.Adapters().DB().UpsertLicenseInstance(ctx, li, tx)
	if err != nil {
		return nil, err
	}

	return &CreateResult{
		Instance: li,
	}, nil
}
