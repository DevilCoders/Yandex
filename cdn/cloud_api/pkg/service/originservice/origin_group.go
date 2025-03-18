package originservice

import (
	"context"
	"math/rand"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (s *Service) CreateOriginsGroup(ctx context.Context, params *model.CreateOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "create origins group", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	entity := originsGroupEntityForSave(params)

	createdEntity, resultError := s.Storage.CreateOriginsGroup(ctx, entity)
	if resultError != nil {
		return nil, resultError.Wrap("create origins group entity")
	}

	if s.AutoActivateEntities {
		resultError = s.Storage.ActivateOriginsGroup(ctx, entity.EntityID, entity.EntityVersion)
		if resultError != nil {
			return nil, resultError.Wrap("activate origins group")
		}
	}

	return groupEntityToModel(createdEntity), nil
}

func (s *Service) GetOriginsGroup(ctx context.Context, params *model.GetOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get origins group", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	entity, resultError := s.Storage.GetOriginsGroupByID(ctx, params.OriginsGroupID, true)
	if resultError != nil {
		return nil, resultError.Wrap("get origins group entity")
	}

	return groupEntityToModel(entity), nil
}

func (s *Service) GetGroupWithoutOrigins(ctx context.Context, params *model.GetOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get group without origins", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	entity, resultError := s.Storage.GetOriginsGroupByID(ctx, params.OriginsGroupID, false)
	if resultError != nil {
		return nil, resultError.Wrap("get origins group entity")
	}

	return groupEntityToModel(entity), nil
}

// TODO: refactor
func (s *Service) GetAllOriginsGroup(ctx context.Context, params *model.GetAllOriginsGroupParams) ([]*model.OriginsGroup, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get all origins group", log.Sprintf("params", "%+v", params))

	entities, errorResult := s.Storage.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		FolderID:       params.FolderID,
		GroupIDs:       params.GroupIDs,
		PreloadOrigins: true,
		Page:           makePage(params.Page),
	})
	if errorResult != nil {
		return nil, errorResult.Wrap("get all origins group entities")
	}

	return groupEntitiesToModels(entities), nil
}

func (s *Service) UpdateOriginsGroup(ctx context.Context, params *model.UpdateOriginsGroupParams) (*model.OriginsGroup, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "update origins group", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	updateParams := originsGroupParamsForUpdate(params)

	updatedEntity, resultError := s.Storage.UpdateOriginsGroup(ctx, updateParams)
	if resultError != nil {
		return nil, resultError.Wrap("update origins group entity")
	}

	if s.AutoActivateEntities {
		resultError = s.Storage.ActivateOriginsGroup(ctx, updatedEntity.EntityID, updatedEntity.EntityVersion)
		if resultError != nil {
			return nil, resultError.Wrap("activate origins group")
		}
	}

	return groupEntityToModel(updatedEntity), nil
}

func (s *Service) DeleteOriginsGroup(ctx context.Context, params *model.DeleteOriginsGroupParams) errors.ErrorResult {
	ctxlog.Info(ctx, s.Logger, "delete origins group", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return errors.WrapError("validate params", errors.ValidationError, err)
	}

	resultError := s.Storage.DeleteOriginsGroupByID(ctx, params.OriginsGroupID)
	if resultError != nil {
		return resultError.Wrap("delete origins group entity")
	}

	return nil
}

func (s *Service) ActivateOriginsGroup(ctx context.Context, params *model.ActivateOriginsGroupParams) errors.ErrorResult {
	ctxlog.Info(ctx, s.Logger, "activate origins group", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return errors.WrapError("validate params", errors.ValidationError, err)
	}

	errorResult := s.Storage.ActivateOriginsGroup(ctx, params.OriginsGroupID, storage.EntityVersion(params.Version))
	if errorResult != nil {
		return errorResult.Wrap("activate origins group")
	}

	return nil
}

func (s *Service) ListOriginsGroupVersions(ctx context.Context, params *model.ListOriginsGroupVersionsParams) ([]*model.OriginsGroup, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "list origins group versions", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	originsGroupVersions, errorResult := s.Storage.GetOriginsGroupVersions(ctx, params.OriginsGroupID, params.Versions, true)
	if errorResult != nil {
		return nil, errorResult.Wrap("get origins group versions")
	}

	return groupEntitiesToModels(originsGroupVersions), nil
}

func originsGroupEntityForSave(params *model.CreateOriginsGroupParams) *storage.OriginsGroupEntity {
	if params == nil {
		return nil
	}

	entityID := rand.Int63() // TODO: refactor

	return &storage.OriginsGroupEntity{
		RowID:         storage.AutoGenID,
		EntityID:      entityID,
		EntityVersion: 1,
		EntityActive:  false,
		FolderID:      params.FolderID,
		Name:          params.Name,
		UseNext:       params.UseNext,
		Origins:       originParamsToEntitiesForSave(entityID, params.FolderID, params.Origins),
		CreatedAt:     storage.AutoGenTime,
		UpdatedAt:     storage.AutoGenTime,
		DeletedAt:     nil,
	}
}

func originsGroupParamsForUpdate(params *model.UpdateOriginsGroupParams) *storage.UpdateOriginsGroupParams {
	return &storage.UpdateOriginsGroupParams{
		ID:      params.OriginsGroupID,
		Name:    params.Name,
		UseNext: params.UseNext,
		Origins: originParamsToEntitiesForUpdate(params.OriginsGroupID, params.Origins),
	}
}

// TODO: refactor, use params instead entities
func originParamsToEntitiesForUpdate(originsGroupEntityID int64, params []*model.OriginParams) []*storage.OriginEntity {
	result := make([]*storage.OriginEntity, 0, len(params))
	for _, param := range params {
		result = append(result, &storage.OriginEntity{
			EntityID:                  storage.AutoGenID,
			OriginsGroupEntityID:      originsGroupEntityID,
			OriginsGroupEntityVersion: 0,
			FolderID:                  "",
			Source:                    param.Source,
			Enabled:                   param.Enabled,
			Backup:                    param.Enabled,
			Type:                      storage.OriginType(param.Type),
			CreatedAt:                 storage.AutoGenTime,
			UpdatedAt:                 storage.AutoGenTime,
			DeletedAt:                 nil,
		})
	}

	return result
}

func originParamsToEntitiesForSave(originsGroupEntityID int64, folderID string, params []*model.OriginParams) []*storage.OriginEntity {
	result := make([]*storage.OriginEntity, 0, len(params))
	for _, param := range params {
		result = append(result, &storage.OriginEntity{
			EntityID:                  storage.AutoGenID,
			OriginsGroupEntityID:      originsGroupEntityID,
			OriginsGroupEntityVersion: 0,
			FolderID:                  folderID,
			Source:                    param.Source,
			Enabled:                   param.Enabled,
			Backup:                    param.Enabled,
			Type:                      storage.OriginType(param.Type),
			CreatedAt:                 storage.AutoGenTime,
			UpdatedAt:                 storage.AutoGenTime,
			DeletedAt:                 nil,
		})
	}

	return result
}

func groupEntitiesToModels(entities []*storage.OriginsGroupEntity) []*model.OriginsGroup {
	result := make([]*model.OriginsGroup, 0, len(entities))
	for _, entity := range entities {
		result = append(result, groupEntityToModel(entity))
	}

	return result
}

func groupEntityToModel(entity *storage.OriginsGroupEntity) *model.OriginsGroup {
	return &model.OriginsGroup{
		ID:       entity.EntityID,
		FolderID: entity.FolderID,
		Name:     entity.Name,
		UseNext:  entity.UseNext,
		Origins:  makeOrigins(entity.Origins),
		Meta: &model.VersionMeta{
			Version: int64(entity.EntityVersion),
			Active:  entity.EntityActive,
		},
	}
}

func makePage(page *model.Pagination) *storage.Pagination {
	if page == nil {
		return nil
	}

	return &storage.Pagination{
		Offset: page.Offset(),
		Limit:  page.Limit(),
	}
}
