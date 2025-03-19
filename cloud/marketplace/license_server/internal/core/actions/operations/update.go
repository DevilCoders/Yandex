package operations

import (
	"context"
	"encoding/json"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type UpdateAction struct {
	actions.BaseAction
}

type UpdateParams struct {
	ID       string
	Done     bool
	Metadata json.RawMessage
	Result   json.RawMessage
}

func NewUpdateAction(env *env.Env) *UpdateAction {
	return &UpdateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (p *UpdateParams) validate() error {
	return valid.Struct(p,
		valid.Value(&p.ID, rule.Required),
		valid.Value(&p.Done, rule.Required),
		valid.Value(&p.Result, rule.Required),
	)
}

func (a *UpdateAction) checkParams(ctx context.Context, params UpdateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("operation update params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("operation update params passed validation")
	return nil
}

func (a *UpdateAction) Do(ctx context.Context, params UpdateParams) (*model.Operation, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	op, err := a.Adapters().DB().GetOperation(ctx, params.ID, tx)
	if err != nil {
		return nil, err
	}

	timeNow := utils.GetTimeNow()

	op.Done = params.Done
	op.Metadata = params.Metadata
	op.Result = params.Result
	op.ModifiedAt = timeNow

	err = a.Adapters().DB().UpsertOperation(ctx, op, tx)
	if err != nil {
		return nil, err
	}

	return op, nil
}
