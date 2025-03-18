package consoleapi

import (
	"context"

	"google.golang.org/protobuf/types/known/emptypb"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/grpcutil"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/resourceservice"
	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type ResourceRulesServiceHandler struct {
	Logger          log.Logger
	AuthClient      grpcutil.AuthClient
	ResourceService resourceservice.ResourceService
	RuleService     resourceservice.ResourceRuleService
	Cache           FolderIDCache
}

func (h *ResourceRulesServiceHandler) List(ctx context.Context, request *cdnpb.ListResourceRuleRequest) (*cdnpb.ListResourceRuleResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list resource rules", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.GetResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceRules, errorResult := h.RuleService.GetAllRulesByResource(ctx, &model.GetAllRulesByResourceParams{
		ResourceID:      request.ResourceId,
		ResourceVersion: nil,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get all resource rules", errorResult)
	}

	return &cdnpb.ListResourceRuleResponse{
		Rules: makePBRules(resourceRules),
	}, nil
}

func (h *ResourceRulesServiceHandler) Create(ctx context.Context, request *cdnpb.CreateResourceRuleRequest) (*cdnpb.CreateResourceRuleResponse, error) {
	ctxlog.Info(ctx, h.Logger, "create resource rule", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.CreateResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceOptions, err := makeResourceOptions(request.Options)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make resource options", err)
	}

	params := &model.CreateResourceRuleParams{
		ResourceID:      request.ResourceId,
		ResourceVersion: nil,
		Name:            request.Name,
		Pattern:         request.RulePattern,
		OriginsGroupID:  nil,
		OriginProtocol:  nil,
		Options:         resourceOptions,
	}

	resourceRule, errorResult := h.RuleService.CreateResourceRule(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("create resource rule", errorResult)
	}

	return &cdnpb.CreateResourceRuleResponse{
		Rule: makePBRule(resourceRule),
	}, nil
}

func (h *ResourceRulesServiceHandler) Get(ctx context.Context, request *cdnpb.GetResourceRuleRequest) (*cdnpb.GetResourceRuleResponse, error) {
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

	return &cdnpb.GetResourceRuleResponse{
		Rule: makePBRule(resourceRule),
	}, nil
}

func (h *ResourceRulesServiceHandler) Update(ctx context.Context, request *cdnpb.UpdateResourceRuleRequest) (*cdnpb.UpdateResourceRuleResponse, error) {
	ctxlog.Info(ctx, h.Logger, "update resource rule", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.UpdateResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceOptions, err := makeResourceOptions(request.Options)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make resource options", err)
	}

	params := &model.UpdateResourceRuleParams{
		RuleID:          request.RuleId,
		ResourceID:      request.ResourceId,
		ResourceVersion: nil,
		Name:            request.Name,
		Pattern:         request.RulePattern,
		OriginsGroupID:  nil,
		OriginProtocol:  nil,
		Options:         resourceOptions,
	}

	resourceRule, errorResult := h.RuleService.UpdateResourceRule(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("update resource rule", errorResult)
	}

	return &cdnpb.UpdateResourceRuleResponse{
		Rule: makePBRule(resourceRule),
	}, nil
}

func (h *ResourceRulesServiceHandler) Delete(ctx context.Context, request *cdnpb.DeleteResourceRuleRequest) (*emptypb.Empty, error) {
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

	return &emptypb.Empty{}, nil
}

func (h *ResourceRulesServiceHandler) authorizeWithResource(ctx context.Context, permission auth.Permission, resourceID string) error {
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

func (h *ResourceRulesServiceHandler) getFolderIDByResourceID(ctx context.Context, resourceID string) (string, errors.ErrorResult) {
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

func makePBRules(rules []*model.ResourceRule) []*cdnpb.Rule {
	result := make([]*cdnpb.Rule, 0, len(rules))
	for _, rule := range rules {
		result = append(result, makePBRule(rule))
	}

	return result
}

func makePBRule(rule *model.ResourceRule) *cdnpb.Rule {
	if rule == nil {
		return nil
	}

	return &cdnpb.Rule{
		Id:          rule.ID,
		Name:        rule.Name,
		RulePattern: rule.Pattern,
		Options:     makePBOptions(rule.Options),
	}
}
