package resourceservice

import (
	"context"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/core/log"
)

type ResourceService interface {
	CreateResource(ctx context.Context, params *model.CreateResourceParams) (*model.Resource, errors.ErrorResult)
	GetResource(ctx context.Context, params *model.GetResourceParams) (*model.Resource, errors.ErrorResult)
	GetAllResources(ctx context.Context, params *model.GetAllResourceParams) ([]*model.Resource, errors.ErrorResult)
	UpdateResource(ctx context.Context, params *model.UpdateResourceParams) (*model.Resource, errors.ErrorResult)
	DeleteResource(ctx context.Context, params *model.DeleteResourceParams) errors.ErrorResult
	ActivateResource(ctx context.Context, params *model.ActivateResourceParams) errors.ErrorResult
	ListResourceVersions(ctx context.Context, params *model.ListResourceVersionsParams) ([]*model.Resource, errors.ErrorResult)
	CountActiveResourcesByFolderID(ctx context.Context, params *model.CountActiveResourcesParams) (int64, errors.ErrorResult)
}

type ResourceRuleService interface {
	CreateResourceRule(ctx context.Context, params *model.CreateResourceRuleParams) (*model.ResourceRule, errors.ErrorResult)
	GetResourceRule(ctx context.Context, params *model.GetResourceRuleParams) (*model.ResourceRule, errors.ErrorResult)
	GetAllResourceRules(ctx context.Context, params *model.GetAllResourceRulesParams) ([]*model.ResourceRule, errors.ErrorResult)
	GetAllRulesByResource(ctx context.Context, params *model.GetAllRulesByResourceParams) ([]*model.ResourceRule, errors.ErrorResult)
	UpdateResourceRule(ctx context.Context, params *model.UpdateResourceRuleParams) (*model.ResourceRule, errors.ErrorResult)
	DeleteResourceRule(ctx context.Context, params *model.DeleteResourceRuleParams) errors.ErrorResult
}

type Service struct {
	Logger            log.Logger
	ResourceGenerator ResourceGenerator
	Storage           storage.Storage

	AutoActivateEntities bool
}
