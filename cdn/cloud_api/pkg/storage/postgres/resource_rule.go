package postgres

import (
	"context"
	"fmt"
	"strconv"
	"strings"

	sq "github.com/Masterminds/squirrel"
	"gorm.io/gorm"
	"gorm.io/gorm/clause"
	"gorm.io/plugin/dbresolver"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func (s *Storage) CreateResourceRule(ctx context.Context, entity *storage.ResourceRuleEntity) (*storage.ResourceRuleEntity, errors.ErrorResult) {
	if entity == nil {
		return nil, errors.NewErrorResult("entity is nil", errors.InternalError)
	}
	if entity.ResourceEntityVersion == 0 {
		return nil, errors.NewErrorResult("entity version is zero", errors.FatalError)
	}

	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Create(entity)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return entity, nil
}

func (s *Storage) GetResourceRuleByID(ctx context.Context, ruleID int64, resourceID string) (*storage.ResourceRuleEntity, errors.ErrorResult) {
	var entities []*storage.ResourceRuleEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Read).
		Where(map[string]interface{}{
			fieldEntityID:         ruleID,
			fieldResourceEntityID: resourceID,
			fieldDeletedAt:        nil,
		}).
		Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch len(entities) {
	case 0:
		msg := fmt.Sprintf("not found, ruleID: %d", ruleID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	case 1:
		return entities[0], nil
	default:
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
}

func (s *Storage) GetAllResourceRules(ctx context.Context, resourceIDs []storage.ResourceIDVersionPair) ([]*storage.ResourceRuleEntity, errors.ErrorResult) {
	builder := strings.Builder{}
	builder.WriteRune('(')
	for i, pair := range resourceIDs {
		builder.WriteRune('(')
		builder.WriteString("'")
		builder.WriteString(pair.ResourceID)
		builder.WriteString("'")
		builder.WriteRune(',')
		builder.WriteString(strconv.FormatInt(pair.ResourceVersion, 10))
		builder.WriteRune(')')
		if i < len(resourceIDs)-1 {
			builder.WriteRune(',')
		}
	}
	builder.WriteRune(')')

	var entities []*storage.ResourceRuleEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Read).
		Where(map[string]interface{}{
			fieldDeletedAt: nil,
		}).
		Where(fmt.Sprintf("(%s, %s) IN %s", fieldResourceEntityID, fieldResourceEntityVersion, builder.String())).
		Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return entities, nil
}

func (s *Storage) GetAllRulesByResource(ctx context.Context, resourceID string, resourceVersion *int64) ([]*storage.ResourceRuleEntity, errors.ErrorResult) {
	return getAllResourceRules(ctx, s.db, dbresolver.Read, resourceID, resourceVersion)
}

func (s *Storage) UpdateResourceRule(ctx context.Context, params *storage.UpdateResourceRuleParams) (*storage.ResourceRuleEntity, errors.ErrorResult) {
	if params == nil {
		return nil, errors.NewErrorResult("params is nil", errors.InternalError)
	}

	entity := updateResourceRuleEntity(params)
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(entity).Clauses(clause.Returning{}).
		Select(star).
		Omit(fieldResourceEntityID, fieldResourceEntityVersion, fieldCreatedAt, fieldDeletedAt).
		Where(map[string]interface{}{
			fieldEntityID:              params.ID,
			fieldResourceEntityID:      params.ResourceID,
			fieldResourceEntityVersion: params.ResourceVersion,
			fieldDeletedAt:             nil,
		}).
		Updates(entity)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	switch result.RowsAffected {
	case 0:
		msg := fmt.Sprintf("resource rule %d not found", params.ID)
		return nil, errors.NewErrorResult(msg, errors.NotFoundError)
	case 1:
		return entity, nil
	default:
		return nil, errors.NewErrorResult("invalid state", errors.FatalError)
	}
}

func (s *Storage) DeleteResourceRule(ctx context.Context, ruleID int64, resourceID string) errors.ErrorResult {
	var entityModel storage.ResourceRuleEntity
	result := s.db.WithContext(ctx).Clauses(dbresolver.Write).
		Model(entityModel).
		Where(map[string]interface{}{
			fieldEntityID:         ruleID,
			fieldResourceEntityID: resourceID,
		}).
		Omit(fieldUpdatedAt).
		Update(fieldDeletedAt, s.db.NowFunc())
	if result.Error != nil {
		return errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return nil
}

func getAllResourceRules(ctx context.Context, db *gorm.DB, replica dbresolver.Operation, resourceID string, resourceVersion *int64) ([]*storage.ResourceRuleEntity, errors.ErrorResult) {
	filter := map[string]interface{}{
		fieldResourceEntityID: resourceID,
		fieldDeletedAt:        nil,
	}

	if resourceVersion != nil {
		filter[fieldResourceEntityVersion] = *resourceVersion
	} else {
		sql, args, errorResult := activeResourceVersionSQL(resourceID)
		if errorResult != nil {
			return nil, errorResult
		}
		filter[fieldResourceEntityVersion] = gorm.Expr(fmt.Sprintf("(%s)", sql), args...)
	}

	var entities []*storage.ResourceRuleEntity
	result := db.WithContext(ctx).Clauses(replica).
		Where(filter).
		Find(&entities)
	if result.Error != nil {
		return nil, errors.WrapError("db exec", errors.InternalError, result.Error)
	}

	return entities, nil
}

func updateResourceRuleEntity(params *storage.UpdateResourceRuleParams) *storage.ResourceRuleEntity {
	return &storage.ResourceRuleEntity{
		EntityID:              params.ID,
		ResourceEntityID:      "",
		ResourceEntityVersion: 0,
		Name:                  params.Name,
		Pattern:               params.Pattern,
		OriginsGroupEntityID:  params.OriginsGroupEntityID,
		OriginProtocol:        params.OriginProtocol,
		Options:               params.Options,
		CreatedAt:             storage.AutoGenTime,
		UpdatedAt:             storage.AutoGenTime,
		DeletedAt:             nil,
	}
}

func activeResourceVersionSQL(resourceID string) (string, []interface{}, errors.ErrorResult) {
	sql, args, err := sq.Select(fieldEntityVersion).
		From(storage.ResourceTable).
		Where(sq.Eq{
			fieldEntityID:     resourceID,
			fieldEntityActive: true,
			fieldDeletedAt:    nil,
		}).ToSql()
	if err != nil {
		return "", nil, errors.WrapError("build query", errors.FatalError, err)
	}

	return sql, args, nil
}
