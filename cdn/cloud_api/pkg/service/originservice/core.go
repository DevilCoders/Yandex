package originservice

import (
	"context"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/core/log"
)

type OriginService interface {
	CreateOrigin(ctx context.Context, params *model.CreateOriginParams) (*model.Origin, errors.ErrorResult)
	GetOrigin(ctx context.Context, params *model.GetOriginParams) (*model.Origin, errors.ErrorResult)
	GetAllOrigins(ctx context.Context, params *model.GetAllOriginParams) ([]*model.Origin, errors.ErrorResult)
	UpdateOrigin(ctx context.Context, params *model.UpdateOriginParams) (*model.Origin, errors.ErrorResult)
	DeleteOrigin(ctx context.Context, params *model.DeleteOriginParams) errors.ErrorResult
}

type OriginsGroupService interface {
	CreateOriginsGroup(ctx context.Context, params *model.CreateOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult)
	GetOriginsGroup(ctx context.Context, params *model.GetOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult)
	GetGroupWithoutOrigins(ctx context.Context, params *model.GetOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult)
	GetAllOriginsGroup(ctx context.Context, params *model.GetAllOriginsGroupParams) ([]*model.OriginsGroup, errors.ErrorResult)
	UpdateOriginsGroup(ctx context.Context, params *model.UpdateOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult)
	DeleteOriginsGroup(ctx context.Context, params *model.DeleteOriginsGroupParams) errors.ErrorResult
	ActivateOriginsGroup(ctx context.Context, params *model.ActivateOriginsGroupParams) errors.ErrorResult
	ListOriginsGroupVersions(ctx context.Context, params *model.ListOriginsGroupVersionsParams) ([]*model.OriginsGroup, errors.ErrorResult)
}

type Service struct {
	Logger  log.Logger
	Storage storage.Storage

	AutoActivateEntities bool
}
