package operations

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
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
	Description string
}

func NewCreateAction(env *env.Env) *CreateAction {
	return &CreateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (p *CreateParams) validate() error {
	return valid.Struct(p,
		valid.Value(&p.Description, rule.Required),
	)
}

func (a *CreateAction) checkParams(ctx context.Context, params CreateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("operation create params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("operation create params passed validation")
	return nil
}

func (a *CreateAction) Do(ctx context.Context, params CreateParams) (*model.Operation, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	id := a.CloudIDGenerator().GenerateCloudID()
	timeNow := utils.GetTimeNow()

	op := &model.Operation{
		ID:          id,
		Description: params.Description,
		CreatedAt:   timeNow,
		ModifiedAt:  timeNow,
		CreatedBy:   "me",
		Done:        false,
	}

	err := a.Adapters().DB().UpsertOperation(ctx, op, nil)
	if err != nil {
		return nil, err
	}

	return op, nil
}
