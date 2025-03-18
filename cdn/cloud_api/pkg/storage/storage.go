package storage

import (
	"context"
	"time"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
)

// TODO: use createParams instead entity

type Storage interface {
	ActivateResource(ctx context.Context, entityID string, version EntityVersion) errors.ErrorResult
	CreateResource(ctx context.Context, entity *ResourceEntity) (*ResourceEntity, errors.ErrorResult)
	GetResourceByID(ctx context.Context, resourceID string) (*ResourceEntity, errors.ErrorResult)
	GetResourceVersions(ctx context.Context, resourceID string, versions []int64) ([]*ResourceEntity, errors.ErrorResult)
	GetAllResourcesByFolderID(ctx context.Context, folderID string, page *Pagination) ([]*ResourceEntity, errors.ErrorResult)
	GetAllResources(ctx context.Context, page *Pagination) ([]*ResourceEntity, errors.ErrorResult)
	UpdateResource(ctx context.Context, params *UpdateResourceParams) (*ResourceEntity, errors.ErrorResult)
	DeleteResourceByID(ctx context.Context, resourceID string) errors.ErrorResult
	CopyResource(ctx context.Context, resourceID string) (*ResourceEntity, map[int64]int64, errors.ErrorResult)
	CountActiveResourcesByFolderID(ctx context.Context, folderID string) (int64, errors.ErrorResult)

	ActivateOriginsGroup(ctx context.Context, entityID int64, version EntityVersion) errors.ErrorResult
	CreateOriginsGroup(ctx context.Context, entity *OriginsGroupEntity) (*OriginsGroupEntity, errors.ErrorResult)
	GetOriginsGroupByID(ctx context.Context, originsGroupID int64, preloadOrigins bool) (*OriginsGroupEntity, errors.ErrorResult)
	GetOriginsGroupVersions(ctx context.Context, originsGroupID int64, versions []int64, preloadOrigins bool) ([]*OriginsGroupEntity, errors.ErrorResult)
	GetAllOriginsGroups(ctx context.Context, params *GetAllOriginsGroupParams) ([]*OriginsGroupEntity, errors.ErrorResult)
	UpdateOriginsGroup(ctx context.Context, params *UpdateOriginsGroupParams) (*OriginsGroupEntity, errors.ErrorResult)
	DeleteOriginsGroupByID(ctx context.Context, originsGroupID int64) errors.ErrorResult

	CreateOrigin(ctx context.Context, entity *OriginEntity) (*OriginEntity, errors.ErrorResult)
	GetOriginByID(ctx context.Context, originsGroupID int64, originID int64) (*OriginEntity, errors.ErrorResult)
	GetAllOrigins(ctx context.Context, originsGroupID int64) ([]*OriginEntity, errors.ErrorResult)
	UpdateOrigin(ctx context.Context, params *UpdateOriginParams) (*OriginEntity, errors.ErrorResult)
	DeleteOriginByID(ctx context.Context, originsGroupID int64, originID int64) errors.ErrorResult

	CreateResourceRule(ctx context.Context, entity *ResourceRuleEntity) (*ResourceRuleEntity, errors.ErrorResult)
	GetResourceRuleByID(ctx context.Context, ruleID int64, resourceID string) (*ResourceRuleEntity, errors.ErrorResult)
	GetAllResourceRules(ctx context.Context, resourceIDs []ResourceIDVersionPair) ([]*ResourceRuleEntity, errors.ErrorResult)
	GetAllRulesByResource(ctx context.Context, resourceID string, resourceVersion *int64) ([]*ResourceRuleEntity, errors.ErrorResult)
	UpdateResourceRule(ctx context.Context, params *UpdateResourceRuleParams) (*ResourceRuleEntity, errors.ErrorResult)
	DeleteResourceRule(ctx context.Context, ruleID int64, resourceID string) errors.ErrorResult
}

type AdminStorage interface {
	EraseSoftDeletedResource(ctx context.Context, threshold time.Time) errors.ErrorResult
	EraseOldResourceVersions(ctx context.Context, resourceID string, versionThreshold int64) errors.ErrorResult

	EraseSoftDeletedOriginsGroup(ctx context.Context, threshold time.Time) errors.ErrorResult
	EraseOldOriginsGroupVersions(ctx context.Context, groupID int64, versionThreshold int64) errors.ErrorResult
}
