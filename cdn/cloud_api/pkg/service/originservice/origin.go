package originservice

import (
	"context"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (s *Service) CreateOrigin(ctx context.Context, params *model.CreateOriginParams) (*model.Origin, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "create origin", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	entity := originEntityForSave(params)

	createdEntity, resultError := s.Storage.CreateOrigin(ctx, entity)
	if resultError != nil {
		return nil, resultError.Wrap("create origin entity")
	}

	return makeOrigin(createdEntity), nil
}

func (s *Service) GetOrigin(ctx context.Context, params *model.GetOriginParams) (*model.Origin, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get origin", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	entity, resultError := s.Storage.GetOriginByID(ctx, params.OriginsGroupID, params.OriginID)
	if resultError != nil {
		return nil, resultError.Wrap("get origin")
	}

	return makeOrigin(entity), nil
}

func (s *Service) GetAllOrigins(ctx context.Context, params *model.GetAllOriginParams) ([]*model.Origin, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get all origins", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	entities, resultError := s.Storage.GetAllOrigins(ctx, params.OriginsGroupID)
	if resultError != nil {
		return nil, resultError.Wrap("get all origin entities")
	}

	return makeOrigins(entities), nil
}

func (s *Service) UpdateOrigin(ctx context.Context, params *model.UpdateOriginParams) (*model.Origin, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "update origin", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	updateParams := originParamsForUpdate(params)

	updatedEntity, resultError := s.Storage.UpdateOrigin(ctx, updateParams)
	if resultError != nil {
		return nil, resultError.Wrap("updaye origin entity")
	}

	return makeOrigin(updatedEntity), nil
}

func (s *Service) DeleteOrigin(ctx context.Context, params *model.DeleteOriginParams) errors.ErrorResult {
	ctxlog.Info(ctx, s.Logger, "delete origin", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return errors.WrapError("validate params", errors.ValidationError, err)
	}

	resultError := s.Storage.DeleteOriginByID(ctx, params.OriginsGroupID, params.OriginID)
	if resultError != nil {
		return resultError.Wrap("delete origin entity")
	}

	return nil
}

func originEntityForSave(params *model.CreateOriginParams) *storage.OriginEntity {
	return &storage.OriginEntity{
		EntityID:                  storage.AutoGenID,
		OriginsGroupEntityID:      params.OriginsGroupID,
		OriginsGroupEntityVersion: 0,
		FolderID:                  params.FolderID,
		Source:                    params.Source,
		Enabled:                   params.Enabled,
		Backup:                    params.Backup,
		Type:                      storage.OriginType(params.Type),
		CreatedAt:                 storage.AutoGenTime,
		UpdatedAt:                 storage.AutoGenTime,
		DeletedAt:                 nil,
	}
}

func originParamsForUpdate(params *model.UpdateOriginParams) *storage.UpdateOriginParams {
	return &storage.UpdateOriginParams{
		ID:             params.OriginID,
		OriginsGroupID: params.OriginsGroupID,
		FolderID:       params.FolderID,
		Source:         params.Source,
		Enabled:        params.Enabled,
		Backup:         params.Backup,
		Type:           storage.OriginType(params.Type),
	}
}

func makeOrigins(entities []*storage.OriginEntity) []*model.Origin {
	result := make([]*model.Origin, 0, len(entities))
	for _, entity := range entities {
		result = append(result, makeOrigin(entity))
	}

	return result
}

func makeOrigin(entity *storage.OriginEntity) *model.Origin {
	return &model.Origin{
		ID:       entity.EntityID,
		FolderID: entity.FolderID,
		Source:   entity.Source,
		Enabled:  entity.Enabled,
		Backup:   entity.Backup,
		Type:     model.OriginType(entity.Type),
	}
}
