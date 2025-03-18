package postgres

import (
	"context"
	"fmt"

	"gorm.io/gorm/clause"
	"gorm.io/plugin/dbresolver"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func (s *Storage) CreateOrigin(ctx context.Context, entity *storage.OriginEntity) (*storage.OriginEntity, errors.ErrorResult) {
	if entity == nil {
		return nil, errors.NewErrorResult("entity is nil", errors.InternalError)
	}

	// TODO: refactor
	actualOriginsGroup, errorResult := getOriginsGroup(ctx, s.db, dbresolver.Read, entity.OriginsGroupEntityID, false)
	if errorResult != nil {
		msg := fmt.Sprintf("not found origins group with id: %d", entity.OriginsGroupEntityID)
		return nil, errorResult.Wrap(msg)
	}
	entity.OriginsGroupEntityVersion = int64(actualOriginsGroup.EntityVersion)

	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Create(entity)
	if result.Error != nil {
		return nil, errors.WrapError("create origin", errors.InternalError, result.Error)
	}

	return entity, nil
}

func (s *Storage) GetOriginByID(ctx context.Context, originsGroupID int64, originID int64) (*storage.OriginEntity, errors.ErrorResult) {
	var entities []*storage.OriginEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Read).
		Where(map[string]interface{}{
			fieldOriginsGroupEntityID: originsGroupID,
			fieldEntityID:             originID,
			fieldDeletedAt:            nil,
		}).
		Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch len(entities) {
	case 0:
		msg := fmt.Sprintf("origin not found, originsGroupID: %d, originID: %d", originsGroupID, originID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	case 1:
		return entities[0], nil
	default:
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
}

func (s *Storage) GetAllOrigins(ctx context.Context, originsGroupID int64) ([]*storage.OriginEntity, errors.ErrorResult) {
	originsGroup, errorResult := s.GetOriginsGroupByID(ctx, originsGroupID, true)
	if errorResult != nil {
		return nil, errorResult
	}

	return originsGroup.Origins, nil
}

func (s *Storage) UpdateOrigin(ctx context.Context, params *storage.UpdateOriginParams) (*storage.OriginEntity, errors.ErrorResult) {
	if params == nil {
		return nil, errors.NewErrorResult("params is nil", errors.InternalError)
	}

	entity := updateOriginEntity(params)

	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(entity).Clauses(clause.Returning{}).
		Select(star).
		Omit(fieldOriginsGroupEntityID, fieldOriginsGroupEntityVersion, fieldCreatedAt, fieldDeletedAt).
		Where(map[string]interface{}{
			fieldEntityID:             params.ID,
			fieldOriginsGroupEntityID: params.OriginsGroupID,
			fieldDeletedAt:            nil,
		}).
		Updates(entity)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch result.RowsAffected {
	case 0:
		msg := fmt.Sprintf("origin not found: %d", params.ID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	case 1:
		return entity, nil
	default:
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
}

func (s *Storage) DeleteOriginByID(ctx context.Context, originsGroupID int64, originID int64) errors.ErrorResult {
	var entityModel storage.OriginEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(entityModel).
		Where(map[string]interface{}{
			fieldOriginsGroupEntityID: originsGroupID,
			fieldEntityID:             originID,
			fieldDeletedAt:            nil,
		}).
		Omit(fieldUpdatedAt).
		Update(fieldDeletedAt, s.db.NowFunc())
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return nil
}

func updateOriginEntity(params *storage.UpdateOriginParams) *storage.OriginEntity {
	return &storage.OriginEntity{
		EntityID:                  params.ID,
		OriginsGroupEntityID:      params.OriginsGroupID,
		OriginsGroupEntityVersion: 0,
		FolderID:                  params.FolderID,
		Source:                    params.Source,
		Enabled:                   params.Enabled,
		Backup:                    params.Backup,
		Type:                      params.Type,
		CreatedAt:                 storage.AutoGenTime,
		UpdatedAt:                 storage.AutoGenTime,
		DeletedAt:                 nil,
	}
}
