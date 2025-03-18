package postgres

import (
	"context"
	"fmt"
	"time"

	"github.com/Masterminds/squirrel"
	"gorm.io/gorm"
	"gorm.io/gorm/clause"
	"gorm.io/plugin/dbresolver"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func (s *Storage) ActivateOriginsGroup(ctx context.Context, entityID int64, version storage.EntityVersion) errors.ErrorResult {
	err := s.db.Transaction(func(tx *gorm.DB) error {
		return activateOriginsGroup(ctx, tx, entityID, version)
	})
	if err != nil {
		return errors.WrapError("origins group", errors.InternalError, err)
	}

	return nil
}

func (s *Storage) CreateOriginsGroup(ctx context.Context, entity *storage.OriginsGroupEntity) (*storage.OriginsGroupEntity, errors.ErrorResult) {
	if entity == nil {
		return nil, errors.NewErrorResult("entity is nil", errors.InternalError)
	}

	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Create(entity)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return entity, nil
}

func (s *Storage) GetOriginsGroupByID(ctx context.Context, originsGroupID int64, preloadOrigins bool) (*storage.OriginsGroupEntity, errors.ErrorResult) {
	// TODO: refactor, use filterOriginsGroupID
	return getOriginsGroup(ctx, s.db, dbresolver.Read, originsGroupID, preloadOrigins)
}

func (s *Storage) GetOriginsGroupVersions(ctx context.Context, originsGroupID int64, versions []int64, preloadOrigins bool) ([]*storage.OriginsGroupEntity, errors.ErrorResult) {
	if len(versions) == 0 {
		return getOriginsGroupRows(ctx, s.db, dbresolver.Read, originsGroupID, preloadOrigins)
	}
	return getOriginsGroupRows(ctx, s.db, dbresolver.Read, originsGroupID, preloadOrigins, filterVersions(versions))
}

func (s *Storage) GetAllOriginsGroups(ctx context.Context, params *storage.GetAllOriginsGroupParams) ([]*storage.OriginsGroupEntity, errors.ErrorResult) {
	if params == nil {
		return nil, errors.NewErrorResult("params is nil", errors.InternalError)
	}

	var opts []filterOptions
	if params.FolderID != nil {
		opts = append(opts, filterFolderID(*params.FolderID))
	}
	if params.GroupIDs != nil {
		opts = append(opts, filterEntityIDs(*params.GroupIDs))
	}

	return getAllOriginsGroups(ctx, s.db, dbresolver.Read, params.Page, params.PreloadOrigins, opts...)
}

// TODO: refactor, optimize requests
func (s *Storage) UpdateOriginsGroup(ctx context.Context, params *storage.UpdateOriginsGroupParams) (*storage.OriginsGroupEntity, errors.ErrorResult) {
	if params == nil {
		return nil, errors.NewErrorResult("params is nil", errors.InternalError)
	}

	originsGroup, errorResult := getOriginsGroup(ctx, s.db, dbresolver.Read, params.ID, false)
	if errorResult != nil {
		return nil, errorResult.Wrap("get origins group")
	}

	entity := updateOriginsGroupEntity(originsGroup, params)
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Clauses(clause.Returning{Columns: []clause.Column{gormColumn(fieldRowID), gormColumn(fieldEntityVersion)}}).
		Create(entity)
	if result.Error != nil {
		return nil, errors.WrapError("update origins group", errors.InternalError, result.Error)
	}

	if entity.EntityVersion == 0 {
		//TODO error, rollback transaction?
		return nil, errors.NewErrorResult("couldn't update version", errors.FatalError)
	}

	origins := params.Origins
	for i, origin := range origins {
		origin.OriginsGroupEntityID = entity.EntityID
		origin.OriginsGroupEntityVersion = int64(entity.EntityVersion)
		origin.FolderID = entity.FolderID
		origins[i] = origin
	}

	if len(origins) == 0 {
		// copy old values
		actualOriginsGroup, errorResult := getOriginsGroup(ctx, s.db, dbresolver.Write, params.ID, true)
		if errorResult != nil {
			return nil, errorResult
		}
		existingOrigins := actualOriginsGroup.Origins

		// clean auto gen params
		for i, origin := range existingOrigins {
			origin.OriginsGroupEntityID = entity.EntityID
			origin.OriginsGroupEntityVersion = int64(entity.EntityVersion)
			origin.EntityID = storage.AutoGenID
			origin.CreatedAt = storage.AutoGenTime
			origin.UpdatedAt = storage.AutoGenTime
			existingOrigins[i] = origin
		}
		origins = existingOrigins
	}

	if len(origins) != 0 {
		result = s.db.WithContext(ctx).Clauses(dbresolver.Write).
			Create(origins)
		if result.Error != nil {
			return nil, errors.WrapError("create new origins", errors.InternalError, result.Error)
		}
	}

	entity.Origins = origins

	return entity, nil
}

func (s *Storage) DeleteOriginsGroupByID(ctx context.Context, originsGroupID int64) errors.ErrorResult {
	err := s.db.Transaction(func(tx *gorm.DB) error {
		originsGroup, errorResult := deleteOriginsGroup(ctx, tx, originsGroupID)
		if errorResult != nil {
			return errorResult.Error()
		}

		_, errorResult = deleteOrigins(ctx, tx, originsGroup.EntityID)
		if errorResult != nil {
			return errorResult.Error()
		}

		return nil
	})
	if err != nil {
		return errors.WrapError("delete origins group", errors.InternalError, err)
	}

	return nil
}

func (s *Storage) EraseSoftDeletedOriginsGroup(ctx context.Context, threshold time.Time) errors.ErrorResult {
	var originEntity storage.OriginEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Where(fmt.Sprintf("%s < ?", fieldDeletedAt), threshold).
		Delete(&originEntity)
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	var groupEntity storage.OriginsGroupEntity
	result = s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Where(fmt.Sprintf("%s < ?", fieldDeletedAt), threshold).
		Delete(&groupEntity)
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return nil
}

func (s *Storage) EraseOldOriginsGroupVersions(ctx context.Context, groupID int64, versionThreshold int64) errors.ErrorResult {
	var originEntity storage.OriginEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Where(map[string]interface{}{
			fieldOriginsGroupEntityID: groupID,
		}).
		Where(fmt.Sprintf("%s <= ?", fieldOriginsGroupEntityVersion), versionThreshold).
		Delete(&originEntity)
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	var groupEntity storage.OriginsGroupEntity
	result = s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Where(map[string]interface{}{
			fieldEntityID: groupID,
		}).
		Where(fmt.Sprintf("%s <= ?", fieldEntityVersion), versionThreshold).
		Delete(&groupEntity)
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return nil
}

func activateOriginsGroup(ctx context.Context, db *gorm.DB, groupID int64, version storage.EntityVersion) error {
	var originsGroupModel storage.OriginsGroupEntity
	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(originsGroupModel).
		Where(map[string]interface{}{
			fieldEntityID:     groupID,
			fieldDeletedAt:    nil,
			fieldEntityActive: true,
		}).
		Update(fieldEntityActive, false)
	if result.Error != nil {
		return fmt.Errorf("deactivate: %w", result.Error)
	}

	result = db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(originsGroupModel).
		Where(map[string]interface{}{
			fieldEntityID:      groupID,
			fieldDeletedAt:     nil,
			fieldEntityVersion: version,
		}).
		Update(fieldEntityActive, true)
	if result.Error != nil {
		return fmt.Errorf("activate: %w", result.Error)
	}

	switch result.RowsAffected {
	case 0:
		return fmt.Errorf("origins group not found, entityID: %d, version: %d", groupID, version)
	case 1:
		return nil
	default:
		return fmt.Errorf("invalid state")
	}
}

func getAllOriginsGroups(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, page *storage.Pagination, preloadOrigins bool, opts ...filterOptions) ([]*storage.OriginsGroupEntity, errors.ErrorResult) {
	filter := map[string]interface{}{
		fieldEntityActive: true,
		fieldDeletedAt:    nil,
	}

	for _, o := range opts {
		o(filter)
	}

	query := db.WithContext(ctx).Clauses(replica).
		Where(filter)

	if page != nil {
		query.Order(fieldRowID).Offset(page.Offset).Limit(page.Limit)
	}

	if preloadOrigins {
		query.Preload(preloadOriginsKey, map[string]interface{}{
			fieldDeletedAt: nil,
		})
	}

	var entities []*storage.OriginsGroupEntity
	result := query.Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return entities, nil
}

func getOriginsGroup(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, entityID int64, preloadOrigins bool, opts ...filterOptions) (*storage.OriginsGroupEntity, errors.ErrorResult) {
	filter := map[string]interface{}{
		fieldEntityID:     entityID,
		fieldEntityActive: true,
		fieldDeletedAt:    nil,
	}

	for _, o := range opts {
		o(filter)
	}

	query := db.WithContext(ctx).Clauses(replica).
		Where(filter)

	if preloadOrigins {
		query.Preload(preloadOriginsKey, map[string]interface{}{
			fieldDeletedAt: nil,
		})
	}

	var entities []*storage.OriginsGroupEntity
	result := query.Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch len(entities) {
	case 0:
		msg := fmt.Sprintf("not found, entityID: %d", entityID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	case 1:
		return entities[0], nil
	default:
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
}

func getOriginsGroupRows(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, entityID int64, preloadOrigins bool, opts ...filterOptions) ([]*storage.OriginsGroupEntity, errors.ErrorResult) {
	filter := map[string]interface{}{
		fieldEntityID:  entityID,
		fieldDeletedAt: nil,
	}

	for _, o := range opts {
		o(filter)
	}

	query := db.WithContext(ctx).Clauses(replica).
		Where(filter)

	if preloadOrigins {
		query.Preload(preloadOriginsKey, map[string]interface{}{
			fieldDeletedAt: nil,
		})
	}

	var entities []*storage.OriginsGroupEntity
	result := query.Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch len(entities) {
	case 0:
		msg := fmt.Sprintf("not found, entityID: %d", entityID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	default:
		return entities, nil
	}
}

func deleteOriginsGroup(ctx context.Context, db *gorm.DB, originsGroupID int64) (*storage.OriginsGroupEntity, errors.ErrorResult) {
	sql, args, err := squirrel.Select("true").
		From(storage.ResourceTable).
		Where(squirrel.Eq{
			fieldOriginsGroupEntityID: originsGroupID,
			fieldEntityActive:         true,
			fieldDeletedAt:            nil,
		}).
		Limit(1).
		ToSql()
	if err != nil {
		return nil, errors.WrapError("build query", errors.FatalError, err)
	}

	var groups []*storage.OriginsGroupEntity
	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(&groups).Clauses(clause.Returning{}).
		Where(map[string]interface{}{
			fieldEntityID:  originsGroupID,
			fieldDeletedAt: nil,
		}).
		Where(clause.Where{
			Exprs: []clause.Expression{
				clause.Expr{
					SQL:                fmt.Sprintf("true NOT IN (%s)", sql),
					Vars:               args,
					WithoutParentheses: true,
				},
			},
		}).
		Omit(fieldUpdatedAt).
		Update(fieldDeletedAt, db.NowFunc())
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch result.RowsAffected {
	case 0:
		msg := fmt.Sprintf("origins group %d can not be deleted", originsGroupID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	default:
		for _, group := range groups {
			if group.EntityActive {
				return group, nil
			}
		}
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
}

func deleteOrigins(ctx context.Context, db *gorm.DB, groupID int64) ([]*storage.OriginEntity, errors.ErrorResult) {
	var origins []*storage.OriginEntity
	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(&origins).Clauses(clause.Returning{}).
		Where(map[string]interface{}{
			fieldOriginsGroupEntityID: groupID,
			fieldDeletedAt:            nil,
		}).
		Omit(fieldUpdatedAt).
		Update(fieldDeletedAt, db.NowFunc())
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return origins, nil
}

func updateOriginsGroupEntity(actualEntity *storage.OriginsGroupEntity, params *storage.UpdateOriginsGroupParams) *storage.OriginsGroupEntity {
	return &storage.OriginsGroupEntity{
		RowID:         storage.AutoGenID,
		EntityID:      params.ID,
		EntityVersion: storage.AutoGenID,
		EntityActive:  false,
		FolderID:      actualEntity.FolderID,
		Name:          params.Name,
		UseNext:       params.UseNext,
		Origins:       nil,
		CreatedAt:     actualEntity.CreatedAt,
		UpdatedAt:     storage.AutoGenTime,
		DeletedAt:     nil,
	}
}
