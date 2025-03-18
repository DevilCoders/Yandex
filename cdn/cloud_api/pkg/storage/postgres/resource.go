package postgres

import (
	"context"
	"fmt"
	"time"

	"gorm.io/gorm"
	"gorm.io/gorm/clause"
	"gorm.io/plugin/dbresolver"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func (s *Storage) ActivateResource(ctx context.Context, entityID string, version storage.EntityVersion) errors.ErrorResult {
	err := s.db.Transaction(func(tx *gorm.DB) error {
		err := activateResource(ctx, tx, entityID, version)
		if err != nil {
			return err
		}

		err = activateSecondaryHostnames(ctx, tx, entityID, version)
		if err != nil {
			return err
		}

		return nil
	})
	if err != nil {
		return errors.WrapError("resource", errors.InternalError, err)
	}

	return nil
}

func (s *Storage) CreateResource(ctx context.Context, entity *storage.ResourceEntity) (*storage.ResourceEntity, errors.ErrorResult) {
	return createResource(ctx, s.db, dbresolver.Write, entity)
}

func (s *Storage) GetResourceByID(ctx context.Context, resourceID string) (*storage.ResourceEntity, errors.ErrorResult) {
	return getResource(ctx, s.db, dbresolver.Read, resourceID)
}

func (s *Storage) GetResourceVersions(ctx context.Context, resourceID string, versions []int64) ([]*storage.ResourceEntity, errors.ErrorResult) {
	if len(versions) == 0 {
		return getResourceRows(ctx, s.db, dbresolver.Read, resourceID)
	}
	return getResourceRows(ctx, s.db, dbresolver.Read, resourceID, filterVersions(versions))
}

func (s *Storage) GetAllResources(ctx context.Context, page *storage.Pagination) ([]*storage.ResourceEntity, errors.ErrorResult) {
	return getAllResources(ctx, s.db, dbresolver.Read, page)
}

func (s *Storage) GetAllResourcesByFolderID(ctx context.Context, folderID string, page *storage.Pagination) ([]*storage.ResourceEntity, errors.ErrorResult) {
	return getAllResources(ctx, s.db, dbresolver.Read, page, filterFolderID(folderID))
}

func (s *Storage) UpdateResource(ctx context.Context, params *storage.UpdateResourceParams) (*storage.ResourceEntity, errors.ErrorResult) {
	if params == nil {
		return nil, errors.NewErrorResult("params is nil", errors.InternalError)
	}

	actualResource, errorResult := getResource(ctx, s.db, dbresolver.Write, params.ID)
	if errorResult != nil {
		return nil, errorResult.Wrap("get resource")
	}

	switch {
	case params.OriginsGroupID == 0:
		// copy the old origins group id
		params.OriginsGroupID = actualResource.OriginsGroupEntityID

	case params.OriginsGroupID != actualResource.OriginsGroupEntityID:
		// check that the new origins group has not been deleted
		_, errorResult = getOriginsGroup(ctx, s.db, dbresolver.Read, params.OriginsGroupID, false)
		if errorResult != nil {
			return nil, errorResult.Wrap("not found new origins group")
		}
	}

	var entity *storage.ResourceEntity
	err := s.db.Transaction(func(tx *gorm.DB) error {
		entity = updateResourceEntity(actualResource, params)
		result := tx.WithContext(ctx).Clauses(dbresolver.Write).
			Clauses(clause.Returning{Columns: []clause.Column{gormColumn(fieldRowID), gormColumn(fieldEntityVersion)}}).
			Create(entity)
		if result.Error != nil {
			return result.Error
		}

		_, errorResult = copyResourceRules(ctx, tx, dbresolver.Write, entity.EntityID, nil, entity.EntityVersion)
		if errorResult != nil {
			return errorResult.Error()
		}

		if entity.EntityVersion == 0 {
			return fmt.Errorf("couldn't update version")
		}

		return nil
	})
	if err != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, err)
	}

	return entity, nil
}

func (s *Storage) DeleteResourceByID(ctx context.Context, resourceID string) errors.ErrorResult {
	err := s.db.Transaction(func(tx *gorm.DB) error {
		err := deleteResource(ctx, tx, resourceID)
		if err != nil {
			return fmt.Errorf("delete resource: %w", err)
		}

		err = deleteSecondaryHostnames(ctx, tx, resourceID)
		if err != nil {
			return fmt.Errorf("delete secondary hostname: %w", err)
		}

		return nil
	})
	if err != nil {
		return errors.WrapError("db exec", errors.InternalError, err)
	}

	return nil
}

func (s *Storage) CopyResource(ctx context.Context, resourceID string) (*storage.ResourceEntity, map[int64]int64, errors.ErrorResult) {
	return copyResource(ctx, s.db, resourceID)
}

func (s *Storage) CountActiveResourcesByFolderID(ctx context.Context, folderID string) (int64, errors.ErrorResult) {
	var count int64
	var entityModel storage.ResourceEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Read).
		Model(entityModel).
		Where(map[string]interface{}{
			fieldFolderID:     folderID,
			fieldEntityActive: true,
			fieldDeletedAt:    nil,
		}).Count(&count)

	if result.Error != nil {
		return 0, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return count, nil
}

func deleteResource(ctx context.Context, db *gorm.DB, resourceID string) error {
	var entityModel storage.ResourceEntity
	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(entityModel).
		Where(map[string]interface{}{
			fieldEntityID:  resourceID,
			fieldDeletedAt: nil,
		}).
		Omit(fieldUpdatedAt).
		Update(fieldDeletedAt, db.NowFunc())
	if result.Error != nil {
		return result.Error
	}

	return nil
}

func deleteSecondaryHostnames(ctx context.Context, db *gorm.DB, resourceID string) error {
	var entityModel storage.SecondaryHostnameEntity
	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(entityModel).
		Where(map[string]interface{}{
			fieldResourceEntityID: resourceID,
			fieldDeletedAt:        nil,
		}).
		Omit(fieldUpdatedAt).
		Update(fieldDeletedAt, db.NowFunc())
	if result.Error != nil {
		return result.Error
	}

	return nil
}

func activateResource(ctx context.Context, db *gorm.DB, resourceID string, version storage.EntityVersion) error {
	var resourceModel storage.ResourceEntity
	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(resourceModel).
		Where(map[string]interface{}{
			fieldEntityID:     resourceID,
			fieldDeletedAt:    nil,
			fieldEntityActive: true,
		}).
		Update(fieldEntityActive, false)
	if result.Error != nil {
		return fmt.Errorf("deactivate: %w", result.Error)
	}

	result = db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(resourceModel).
		Where(map[string]interface{}{
			fieldEntityID:      resourceID,
			fieldDeletedAt:     nil,
			fieldEntityVersion: version,
		}).
		Update(fieldEntityActive, true)
	if result.Error != nil {
		return fmt.Errorf("activate: %w", result.Error)
	}

	return nil
}

func activateSecondaryHostnames(ctx context.Context, db *gorm.DB, resourceID string, version storage.EntityVersion) error {
	var secondaryHostnameModel storage.SecondaryHostnameEntity
	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(secondaryHostnameModel).
		Where(map[string]interface{}{
			fieldResourceEntityID:     resourceID,
			fieldResourceEntityActive: true,
		}).
		Update(fieldResourceEntityActive, false)
	if result.Error != nil {
		return fmt.Errorf("deactivate: %w", result.Error)
	}

	result = db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(secondaryHostnameModel).
		Where(map[string]interface{}{
			fieldResourceEntityID:      resourceID,
			fieldResourceEntityVersion: version,
		}).
		Update(fieldResourceEntityActive, true)
	if result.Error != nil {
		return fmt.Errorf("activate: %w", result.Error)
	}

	return nil
}

func createResource(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, entity *storage.ResourceEntity) (*storage.ResourceEntity, errors.ErrorResult) {
	if entity == nil {
		return nil, errors.NewErrorResult("entity is nil", errors.InternalError)
	}

	result := db.WithContext(ctx).Clauses(replica).
		Create(entity)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return entity, nil
}

func copyResource(ctx context.Context, db *gorm.DB, resourceID string) (*storage.ResourceEntity, map[int64]int64, errors.ErrorResult) {
	activeResource, errorResult := getResource(ctx, db, dbresolver.Write, resourceID)
	if errorResult != nil {
		return nil, nil, errorResult
	}

	oldVersion := int64(activeResource.EntityVersion)

	activeResource.RowID = storage.AutoGenID
	activeResource.EntityVersion += 1
	activeResource.EntityActive = false
	activeResource.CreatedAt = storage.AutoGenTime
	activeResource.UpdatedAt = storage.AutoGenTime
	activeResource.DeletedAt = nil

	createdResource, errorResult := createResource(ctx, db, dbresolver.Write, activeResource)
	if errorResult != nil {
		return nil, nil, errorResult
	}

	ruleIDs, errorResult := copyResourceRules(ctx, db, dbresolver.Write, resourceID, &oldVersion, createdResource.EntityVersion)
	if errorResult != nil {
		return nil, nil, errorResult
	}

	return createdResource, ruleIDs, nil
}

func copyResourceRules(
	ctx context.Context,
	db *gorm.DB,
	replica dbresolver.Operation,
	resourceID string,
	oldVersion *int64,
	newVersion storage.EntityVersion,
) (map[int64]int64, errors.ErrorResult) {
	rules, errorResult := getAllResourceRules(ctx, db, replica, resourceID, oldVersion)
	if errorResult != nil {
		return nil, errorResult
	}

	oldIDs := make([]int64, 0, len(rules))
	for i, rule := range rules {
		oldIDs = append(oldIDs, rule.EntityID)

		rule.EntityID = storage.AutoGenID
		rule.ResourceEntityVersion = int64(newVersion)
		rule.CreatedAt = storage.AutoGenTime
		rule.UpdatedAt = storage.AutoGenTime
		rule.DeletedAt = nil

		rules[i] = rule
	}

	errorResult = createResourceRules(ctx, db, rules)
	if errorResult != nil {
		return nil, errorResult
	}
	newIDs := make([]int64, 0, len(rules))
	for _, rule := range rules {
		newIDs = append(newIDs, rule.EntityID)
	}
	if len(oldIDs) != len(newIDs) {
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
	ruleIDs := make(map[int64]int64, len(oldIDs))
	for i, id := range oldIDs {
		ruleIDs[id] = newIDs[i]
	}

	return ruleIDs, nil
}

func createResourceRules(ctx context.Context, db *gorm.DB, entities []*storage.ResourceRuleEntity) errors.ErrorResult {
	if len(entities) == 0 {
		return nil
	}

	result := db.WithContext(ctx).Clauses(dbresolver.Write).
		Create(entities)
	if result.Error != nil {
		return errors.WrapError("create rules", errors.InternalError, result.Error)
	}

	return nil
}

// TODO: filter resourceIDs
func (s *Storage) EraseSoftDeletedResource(ctx context.Context, threshold time.Time) errors.ErrorResult {
	var resourceEntity storage.ResourceEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Where(fmt.Sprintf("%s < ?", fieldDeletedAt), threshold).
		Delete(&resourceEntity)
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return nil
}

func (s *Storage) EraseOldResourceVersions(ctx context.Context, resourceID string, versionThreshold int64) errors.ErrorResult {
	var resourceEntity storage.ResourceEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Where(map[string]interface{}{
			fieldEntityID: resourceID,
		}).
		Where(fmt.Sprintf("%s <= ?", fieldEntityVersion), versionThreshold).
		Delete(&resourceEntity)
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return nil
}

func getAllResources(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, page *storage.Pagination, opts ...filterOptions) ([]*storage.ResourceEntity, errors.ErrorResult) {
	filter := map[string]interface{}{
		fieldEntityActive: true,
		fieldDeletedAt:    nil,
	}

	for _, o := range opts {
		o(filter)
	}

	query := db.WithContext(ctx).Clauses(replica)

	if page != nil {
		query.Order(fieldRowID).Offset(page.Offset).Limit(page.Limit)
	}

	var entities []*storage.ResourceEntity
	result := query.
		Where(filter).
		Preload(preloadSecondaryHostnames).
		Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return entities, nil
}

func getResource(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, entityID string, opts ...filterOptions) (*storage.ResourceEntity, errors.ErrorResult) {
	filter := map[string]interface{}{
		fieldEntityID:     entityID,
		fieldEntityActive: true,
		fieldDeletedAt:    nil,
	}

	for _, o := range opts {
		o(filter)
	}

	var entities []*storage.ResourceEntity
	result := db.WithContext(ctx).Clauses(replica).
		Where(filter).
		Preload(preloadSecondaryHostnames).
		Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch len(entities) {
	case 0:
		msg := fmt.Sprintf("not found, entityID: %s", entityID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	case 1:
		return entities[0], nil
	default:
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
}

func getResourceRows(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, entityID string, opts ...filterOptions) ([]*storage.ResourceEntity, errors.ErrorResult) {
	filter := map[string]interface{}{
		fieldEntityID:  entityID,
		fieldDeletedAt: nil,
	}

	for _, o := range opts {
		o(filter)
	}

	var entities []*storage.ResourceEntity
	result := db.WithContext(ctx).Clauses(replica).
		Where(filter).
		Preload(preloadSecondaryHostnames).
		Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch len(entities) {
	case 0:
		msg := fmt.Sprintf("not found, entityID: %s", entityID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	default:
		return entities, nil
	}
}

func updateResourceEntity(actualEntity *storage.ResourceEntity, params *storage.UpdateResourceParams) *storage.ResourceEntity {
	originProtocol := actualEntity.OriginProtocol
	if params.OriginProtocol != nil {
		originProtocol = *params.OriginProtocol
	}

	hostnames := cleanSecondaryHostnames(actualEntity.SecondaryHostnames)
	if params.SecondaryHostnames != nil {
		hostnames = secondaryHostnames(*params.SecondaryHostnames)
	}

	return &storage.ResourceEntity{
		RowID:                storage.AutoGenID,
		EntityID:             params.ID,
		EntityVersion:        storage.AutoGenID,
		EntityActive:         false,
		FolderID:             actualEntity.FolderID,
		OriginsGroupEntityID: params.OriginsGroupID,
		Active:               params.Active,
		Name:                 actualEntity.Name,
		Cname:                actualEntity.Cname,
		SecondaryHostnames:   hostnames,
		OriginProtocol:       originProtocol,
		Options:              params.Options,
		CreatedAt:            actualEntity.CreatedAt,
		UpdatedAt:            storage.AutoGenTime,
		DeletedAt:            nil,
	}
}

func cleanSecondaryHostnames(hostnames []*storage.SecondaryHostnameEntity) []*storage.SecondaryHostnameEntity {
	result := make([]*storage.SecondaryHostnameEntity, 0, len(hostnames))
	for _, hostname := range hostnames {
		result = append(result, &storage.SecondaryHostnameEntity{
			RowID:                 0,
			ResourceEntityID:      "",
			ResourceEntityVersion: 0,
			ResourceEntityActive:  false,
			Hostname:              hostname.Hostname,
		})
	}

	return result
}

func secondaryHostnames(hostnames []string) []*storage.SecondaryHostnameEntity {
	result := make([]*storage.SecondaryHostnameEntity, 0, len(hostnames))
	for _, hostname := range hostnames {
		result = append(result, &storage.SecondaryHostnameEntity{
			RowID:                 0,
			ResourceEntityID:      "",
			ResourceEntityVersion: 0,
			ResourceEntityActive:  false,
			Hostname:              hostname,
		})
	}

	return result
}
