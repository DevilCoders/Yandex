package userapi

import (
	"context"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/grpcutil"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/mapper"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/resourceservice"
	pbmodel "a.yandex-team.ru/cdn/cloud_api/proto/model"
	"a.yandex-team.ru/cdn/cloud_api/proto/userapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type ResourceRuleServiceHandler struct {
	Logger          log.Logger
	AuthClient      grpcutil.AuthClient
	ResourceService resourceservice.ResourceService
	RuleService     resourceservice.ResourceRuleService
	Cache           FolderIDCache
}

func (h *ResourceRuleServiceHandler) CreateResourceRule(ctx context.Context, request *userapi.CreateResourceRuleRequest) (*pbmodel.ResourceRule, error) {
	ctxlog.Info(ctx, h.Logger, "create resource rule", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.CreateResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceOptions, err := makeResourceOptions(request.ResourceOptions)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make resource options", err)
	}

	protocol, err := makeNullableOriginProtocol(request.OriginProtocol)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin protocol", err)
	}

	params := &model.CreateResourceRuleParams{
		ResourceID:      request.ResourceId,
		ResourceVersion: &request.ResourceVersion,
		Name:            request.Name,
		Pattern:         request.Pattern,
		OriginsGroupID:  request.OriginsGroupId,
		OriginProtocol:  protocol,
		Options:         resourceOptions,
	}

	resourceRule, errorResult := h.RuleService.CreateResourceRule(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("create resource rule", errorResult)
	}

	return mapper.MakePBRule(resourceRule), nil
}

func (h *ResourceRuleServiceHandler) GetResourceRule(ctx context.Context, request *userapi.GetResourceRuleRequest) (*pbmodel.ResourceRule, error) {
	ctxlog.Info(ctx, h.Logger, "get resource rule", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.GetResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceRule, errorResult := h.RuleService.GetResourceRule(ctx, &model.GetResourceRuleParams{
		RuleID:     request.RuleId,
		ResourceID: request.ResourceId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get resource rule", errorResult)
	}

	return mapper.MakePBRule(resourceRule), nil
}

func (h *ResourceRuleServiceHandler) ListResourceRules(ctx context.Context, request *userapi.ListResourceRulesRequest) (*userapi.ListResourceRulesResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list resource rules", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.ListResourcesPermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceRules, errorResult := h.RuleService.GetAllRulesByResource(ctx, &model.GetAllRulesByResourceParams{
		ResourceID:      request.ResourceId,
		ResourceVersion: request.ResourceVersion,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get all resource rules", errorResult)
	}

	return &userapi.ListResourceRulesResponse{
		Rules: mapper.MakePBRules(resourceRules),
	}, nil
}

func (h *ResourceRuleServiceHandler) UpdateResourceRule(ctx context.Context, request *userapi.UpdateResourceRuleRequest) (*pbmodel.ResourceRule, error) {
	ctxlog.Info(ctx, h.Logger, "update resource rule", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.UpdateResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceOptions, err := makeResourceOptions(request.ResourceOptions)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make resource options", err)
	}

	protocol, err := makeNullableOriginProtocol(request.OriginProtocol)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin protocol", err)
	}

	params := &model.UpdateResourceRuleParams{
		RuleID:          request.RuleId,
		ResourceID:      request.ResourceId,
		ResourceVersion: &request.ResourceVersion,
		Name:            request.Name,
		Pattern:         request.Pattern,
		OriginsGroupID:  request.OriginsGroupId,
		OriginProtocol:  protocol,
		Options:         resourceOptions,
	}

	resourceRule, errorResult := h.RuleService.UpdateResourceRule(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("update resource rule", errorResult)
	}

	return mapper.MakePBRule(resourceRule), nil
}

func (h *ResourceRuleServiceHandler) DeleteResourceRule(ctx context.Context, request *userapi.DeleteResourceRuleRequest) (*userapi.DeleteResourceRuleResponse, error) {
	ctxlog.Info(ctx, h.Logger, "delete resource rule", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.DeleteResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	errorResult := h.RuleService.DeleteResourceRule(ctx, &model.DeleteResourceRuleParams{
		RuleID:     request.RuleId,
		ResourceID: request.ResourceId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("delete resource rule", errorResult)
	}

	return &userapi.DeleteResourceRuleResponse{}, nil
}

func (h *ResourceRuleServiceHandler) authorizeWithResource(ctx context.Context, permission auth.Permission, resourceID string) error {
	folderID, errorResult := h.getFolderIDByResourceID(ctx, resourceID)
	if errorResult != nil {
		return grpcutil.ExtractErrorResult("get folderID", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, permission, entityResource(folderID, resourceID))
	if err != nil {
		return grpcutil.WrapAuthError(err)
	}

	return nil
}

func (h *ResourceRuleServiceHandler) getFolderIDByResourceID(ctx context.Context, resourceID string) (string, errors.ErrorResult) {
	val, ok := h.Cache.Get(resourceID)
	if ok {
		return val.(string), nil
	}

	activeResource, errorResult := h.ResourceService.GetResource(ctx, &model.GetResourceParams{
		ResourceID: resourceID,
	})
	if errorResult != nil {
		return "", errorResult
	}

	h.Cache.Add(resourceID, activeResource.FolderID)
	return activeResource.FolderID, nil
}
