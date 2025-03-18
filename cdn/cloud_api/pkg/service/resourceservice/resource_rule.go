package resourceservice

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// TODO: delete log request from service layer?

func (s *Service) CreateResourceRule(ctx context.Context, params *model.CreateResourceRuleParams) (*model.ResourceRule, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "create resource rule", log.Sprintf("params", "%+v", params))

	params.Options = model.SetResourceOptionsDefaults(params.Options)

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	needActivateResource := false
	resourceVersion := int64(0)
	if params.ResourceVersion == nil {
		resource, _, errorResult := s.Storage.CopyResource(ctx, params.ResourceID)
		if errorResult != nil {
			return nil, errorResult.Wrap("copy resource")
		}

		needActivateResource = true
		resourceVersion = int64(resource.EntityVersion)
	} else {
		activeResource, errorResult := s.Storage.GetResourceByID(ctx, params.ResourceID)
		if errorResult != nil {
			return nil, errorResult.Wrap("get active resource")
		}
		// TODO: update rules only in new resources
		if *params.ResourceVersion <= int64(activeResource.EntityVersion) {
			return nil, errors.NewErrorResult("resource rule can only be added to new resource versions", errors.ValidationError)
		}

		resourceVersion = *params.ResourceVersion
	}

	entity := ruleEntityForSave(params, resourceVersion)
	resourceRule, errorResult := s.Storage.CreateResourceRule(ctx, entity)
	if errorResult != nil {
		return nil, errorResult.Wrap("create resource rule entity")
	}

	if s.AutoActivateEntities && needActivateResource {
		errorResult = s.ActivateResource(ctx, &model.ActivateResourceParams{
			ResourceID: params.ResourceID,
			Version:    resourceVersion,
		})
		if errorResult != nil {
			return nil, errorResult.Wrap("activate resource")
		}
	}

	modelRule, err := makeResourceRule(resourceRule)
	if err != nil {
		return nil, errors.WrapError("make resource rule", errors.InternalError, err)
	}

	return modelRule, nil
}

func (s *Service) GetResourceRule(ctx context.Context, params *model.GetResourceRuleParams) (*model.ResourceRule, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get resource rule", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	resourceRule, errorResult := s.Storage.GetResourceRuleByID(ctx, params.RuleID, params.ResourceID)
	if errorResult != nil {
		return nil, errorResult.Wrap("get resource rule by id")
	}

	modelRule, err := makeResourceRule(resourceRule)
	if err != nil {
		return nil, errors.WrapError("make resource rule", errors.InternalError, err)
	}

	return modelRule, nil
}

func (s *Service) GetAllResourceRules(ctx context.Context, params *model.GetAllResourceRulesParams) ([]*model.ResourceRule, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "create resource rule", log.Sprintf("params", "%+v", params))

	rules, errorResult := s.Storage.GetAllResourceRules(ctx, makeResourceIDVersionPairs(params.ResourcePairs))
	if errorResult != nil {
		return nil, errorResult.Wrap("get all resource rules")
	}

	modelRules, err := makeResourceRules(rules)
	if err != nil {
		return nil, errors.WrapError("make resource rule", errors.InternalError, err)
	}

	return modelRules, nil
}

func (s *Service) GetAllRulesByResource(ctx context.Context, params *model.GetAllRulesByResourceParams) ([]*model.ResourceRule, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get all resource rules by resource", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	rules, errorResult := s.Storage.GetAllRulesByResource(ctx, params.ResourceID, params.ResourceVersion)
	if errorResult != nil {
		return nil, errorResult.Wrap("get all resource rules")
	}

	resourceRules, err := makeResourceRules(rules)
	if err != nil {
		return nil, errors.WrapError("make resource rules", errors.InternalError, err)
	}

	return resourceRules, nil
}

func (s *Service) UpdateResourceRule(ctx context.Context, params *model.UpdateResourceRuleParams) (*model.ResourceRule, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "update resource rule", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	needActivateResource := false
	ruleID := params.RuleID
	var resourceVersion int64
	if params.ResourceVersion == nil {
		newResource, ruleIDs, errorResult := s.Storage.CopyResource(ctx, params.ResourceID)
		if errorResult != nil {
			return nil, errorResult.Wrap("copy resource")
		}

		newID, ok := ruleIDs[ruleID]
		if !ok {
			return nil, errors.NewErrorResult("invalid state: not found new rule id", errors.FatalError)
		}
		ruleID = newID
		needActivateResource = true
		resourceVersion = int64(newResource.EntityVersion)
	} else {
		activeResource, errorResult := s.Storage.GetResourceByID(ctx, params.ResourceID)
		if errorResult != nil {
			return nil, errorResult.Wrap("get active resource")
		}
		// TODO: update rules only in new resources
		if *params.ResourceVersion <= int64(activeResource.EntityVersion) {
			return nil, errors.NewErrorResult("resource rule can only be updated in new resource versions", errors.ValidationError)
		}

		resourceVersion = *params.ResourceVersion
	}

	updateParams := ruleParamsForUpdate(ruleID, params, resourceVersion)
	resourceRule, errorResult := s.Storage.UpdateResourceRule(ctx, updateParams)
	if errorResult != nil {
		return nil, errorResult.Wrap("update resource rule entity")
	}

	if s.AutoActivateEntities && needActivateResource {
		errorResult = s.ActivateResource(ctx, &model.ActivateResourceParams{
			ResourceID: params.ResourceID,
			Version:    resourceVersion,
		})
		if errorResult != nil {
			return nil, errorResult.Wrap("activate resource")
		}
	}

	modelRule, err := makeResourceRule(resourceRule)
	if err != nil {
		return nil, errors.WrapError("make resource rule", errors.InternalError, err)
	}

	return modelRule, nil
}

func (s *Service) DeleteResourceRule(ctx context.Context, params *model.DeleteResourceRuleParams) errors.ErrorResult {
	ctxlog.Info(ctx, s.Logger, "delete resource rule", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return errors.WrapError("validate params", errors.ValidationError, err)
	}

	errorResult := s.Storage.DeleteResourceRule(ctx, params.RuleID, params.ResourceID)
	if errorResult != nil {
		return errorResult.Wrap("delete resource rule entity")
	}

	return nil
}

func ruleEntityForSave(params *model.CreateResourceRuleParams, resourceVersion int64) *storage.ResourceRuleEntity {
	if params == nil {
		return nil
	}

	return &storage.ResourceRuleEntity{
		EntityID:              storage.AutoGenID,
		ResourceEntityID:      params.ResourceID,
		ResourceEntityVersion: resourceVersion,
		Name:                  params.Name,
		Pattern:               params.Pattern,
		OriginsGroupEntityID:  params.OriginsGroupID,
		OriginProtocol:        makeNullableOriginProtocol(params.OriginProtocol),
		Options:               resourceEntityOptions(params.Options),
		CreatedAt:             storage.AutoGenTime,
		UpdatedAt:             storage.AutoGenTime,
		DeletedAt:             nil,
	}
}

func ruleParamsForUpdate(ruleID int64, params *model.UpdateResourceRuleParams, resourceVersion int64) *storage.UpdateResourceRuleParams {
	if params == nil {
		return nil
	}

	return &storage.UpdateResourceRuleParams{
		ID:                   ruleID,
		ResourceID:           params.ResourceID,
		ResourceVersion:      resourceVersion,
		Name:                 params.Name,
		Pattern:              params.Pattern,
		OriginsGroupEntityID: params.OriginsGroupID,
		OriginProtocol:       makeNullableOriginProtocol(params.OriginProtocol),
		Options:              resourceEntityOptions(params.Options),
	}
}

func makeNullableOriginProtocol(originProtocol *model.OriginProtocol) *storage.OriginProtocol {
	if originProtocol != nil {
		protocol := storage.OriginProtocol(*originProtocol)
		return &protocol
	}
	return nil
}

func makeResourceRules(rules []*storage.ResourceRuleEntity) ([]*model.ResourceRule, error) {
	result := make([]*model.ResourceRule, 0, len(rules))
	for _, rule := range rules {
		resourceRule, err := makeResourceRule(rule)
		if err != nil {
			return nil, err
		}
		result = append(result, resourceRule)
	}

	return result, nil
}

func makeResourceRule(entity *storage.ResourceRuleEntity) (*model.ResourceRule, error) {
	options, err := makeResourceOptions(entity.Options)
	if err != nil {
		return nil, fmt.Errorf("make resource options: %w", err)
	}

	return &model.ResourceRule{
		ID:                   entity.EntityID,
		ResourceID:           entity.ResourceEntityID,
		ResourceVersion:      entity.ResourceEntityVersion,
		Name:                 entity.Name,
		Pattern:              entity.Pattern,
		OriginsGroupEntityID: entity.OriginsGroupEntityID,
		OriginProtocol:       makeNullableModelOriginProtocol(entity.OriginProtocol),
		Options:              options,
	}, nil
}

func makeNullableModelOriginProtocol(originProtocol *storage.OriginProtocol) *model.OriginProtocol {
	if originProtocol != nil {
		protocol := model.OriginProtocol(*originProtocol)
		return &protocol
	}
	return nil
}

func makeResourceIDVersionPairs(pairs []model.ResourceIDVersionPair) []storage.ResourceIDVersionPair {
	result := make([]storage.ResourceIDVersionPair, 0, len(pairs))
	for _, pair := range pairs {
		result = append(result, storage.ResourceIDVersionPair{
			ResourceID:      pair.ID,
			ResourceVersion: pair.Version,
		})
	}

	return result
}
