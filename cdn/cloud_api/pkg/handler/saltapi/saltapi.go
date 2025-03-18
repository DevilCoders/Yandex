package saltapi

import (
	"context"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/mapper"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/originservice"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/resourceservice"
	"a.yandex-team.ru/cdn/cloud_api/proto/saltapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type SaltServiceHandler struct {
	Logger             log.Logger
	OriginGroupService originservice.OriginsGroupService
	ResourceService    resourceservice.ResourceService
	RuleService        resourceservice.ResourceRuleService
}

func (h *SaltServiceHandler) Dump(ctx context.Context, request *saltapi.DumpRequest) (*saltapi.DumpResponse, error) {
	ctxlog.Info(ctx, h.Logger, "dump requested")

	page := makePage(request)
	resources, errorResult := h.ResourceService.GetAllResources(ctx, &model.GetAllResourceParams{
		FolderID: nil,
		Page:     page,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("get all resources", errorResult)
	}

	groups, rules, errorResult := h.getGroupsAndRulesByResources(ctx, resources)
	if errorResult != nil {
		return nil, mapper.MakePBError("get groups and rules", errorResult)
	}

	return &saltapi.DumpResponse{
		Resources:     mapper.MakePBResources(resources),
		OriginsGroups: mapper.MakePBOriginsGroups(groups),
		Rules:         mapper.MakePBRules(rules),
		NextPageToken: page.NextPageToken(),
	}, nil
}

func makePage(req *saltapi.DumpRequest) *model.Pagination {
	if req == nil || req.PageToken == nil {
		return nil
	}

	return &model.Pagination{
		PageToken: *req.PageToken,
		PageSize:  req.PageSize,
	}
}

func (h *SaltServiceHandler) getGroupsAndRulesByResources(ctx context.Context, resources []*model.Resource) ([]*model.OriginsGroup, []*model.ResourceRule, errors.ErrorResult) {
	originsGroup, errorResult := h.OriginGroupService.GetAllOriginsGroup(ctx, &model.GetAllOriginsGroupParams{
		FolderID: nil,
		Page:     nil,
		GroupIDs: extractGroupIDs(resources),
	})
	if errorResult != nil {
		return nil, nil, errorResult.Wrap("get origins groups")
	}

	rules, errorResult := h.RuleService.GetAllResourceRules(ctx, &model.GetAllResourceRulesParams{
		ResourcePairs: makeResourcePairs(resources),
	})
	if errorResult != nil {
		return nil, nil, errorResult.Wrap("get resource rules")
	}

	return originsGroup, rules, nil
}

func extractGroupIDs(resources []*model.Resource) *[]int64 {
	result := make([]int64, 0, len(resources))
	for _, resource := range resources {
		result = append(result, resource.OriginsGroupID)
	}

	return &result
}

func makeResourcePairs(resources []*model.Resource) []model.ResourceIDVersionPair {
	result := make([]model.ResourceIDVersionPair, 0, len(resources))
	for _, resource := range resources {
		result = append(result, model.ResourceIDVersionPair{
			ID:      resource.ID,
			Version: resource.Meta.Version,
		})
	}

	return result
}
