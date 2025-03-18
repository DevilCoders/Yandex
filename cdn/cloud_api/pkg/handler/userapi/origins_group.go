package userapi

import (
	"context"
	"fmt"
	"sort"

	"a.yandex-team.ru/cdn/cloud_api/pkg/diff"
	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/grpcutil"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/mapper"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/originservice"
	pbmodel "a.yandex-team.ru/cdn/cloud_api/proto/model"
	"a.yandex-team.ru/cdn/cloud_api/proto/userapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type OriginsGroupServiceHandler struct {
	Logger        log.Logger
	AuthClient    grpcutil.AuthClient
	OriginService originservice.OriginsGroupService
	Cache         FolderIDCache
}

func (h *OriginsGroupServiceHandler) CreateOriginsGroup(ctx context.Context, request *userapi.CreateOriginsGroupRequest) (*pbmodel.OriginsGroup, error) {
	ctxlog.Info(ctx, h.Logger, "create origins group", log.Sprintf("request", "%+v", request))

	err := h.AuthClient.Authorize(ctx, auth.CreateOriginPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	originParams, err := makeOriginParams(request.Origins)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origins params", err)
	}

	params := &model.CreateOriginsGroupParams{
		FolderID: request.FolderId,
		Name:     request.Name,
		UseNext:  request.UseNext,
		Origins:  originParams,
	}

	originsGroup, errorResult := h.OriginService.CreateOriginsGroup(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("create origins group", errorResult)
	}

	return mapper.MakePBOriginsGroup(originsGroup), nil
}

func (h *OriginsGroupServiceHandler) GetOriginsGroup(ctx context.Context, request *userapi.GetOriginsGroupRequest) (*pbmodel.OriginsGroup, error) {
	ctxlog.Info(ctx, h.Logger, "get origins group", log.Sprintf("request", "%+v", request))

	originsGroup, errorResult := h.OriginService.GetOriginsGroup(ctx, &model.GetOriginsGroupParams{
		OriginsGroupID: request.GroupId,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("get origins group", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, auth.GetOriginPermission, folder(originsGroup.FolderID))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	return mapper.MakePBOriginsGroup(originsGroup), nil
}

func (h *OriginsGroupServiceHandler) ListOriginsGroup(ctx context.Context, request *userapi.ListOriginsGroupRequest) (*userapi.ListOriginsGroupResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list origins groups", log.Sprintf("request", "%+v", request))

	err := h.AuthClient.Authorize(ctx, auth.ListOriginsPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	page := makePage(request.PageSize, request.PageToken)

	params := &model.GetAllOriginsGroupParams{
		FolderID: &request.FolderId,
		GroupIDs: nil,
		Page:     page,
	}

	originsGroups, errorResult := h.OriginService.GetAllOriginsGroup(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("get all origins group", errorResult)
	}

	return &userapi.ListOriginsGroupResponse{
		Groups:        mapper.MakePBOriginsGroups(originsGroups),
		NextPageToken: calculateNextPage(page, len(originsGroups)),
	}, nil
}

func (h *OriginsGroupServiceHandler) UpdateOriginsGroup(ctx context.Context, request *userapi.UpdateOriginsGroupRequest) (*pbmodel.OriginsGroup, error) {
	ctxlog.Info(ctx, h.Logger, "update origins group", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithGroupFolderID(ctx, auth.UpdateOriginPermission, request.GroupId)
	if err != nil {
		return nil, err
	}

	originParams, err := makeOriginParams(request.Origins)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin params", err)
	}

	params := &model.UpdateOriginsGroupParams{
		OriginsGroupID: request.GroupId,
		Name:           request.Name,
		UseNext:        request.UseNext,
		Origins:        originParams,
	}

	originsGroup, errorResult := h.OriginService.UpdateOriginsGroup(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("update origins group", errorResult)
	}

	return mapper.MakePBOriginsGroup(originsGroup), nil
}

func (h *OriginsGroupServiceHandler) DeleteOriginsGroup(ctx context.Context, request *userapi.DeleteOriginsGroupRequest) (*userapi.DeleteOriginsGroupResponse, error) {
	ctxlog.Info(ctx, h.Logger, "delete origins group", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithGroupFolderID(ctx, auth.DeleteOriginPermission, request.GroupId)
	if err != nil {
		return nil, err
	}

	params := &model.DeleteOriginsGroupParams{
		OriginsGroupID: request.GroupId,
	}

	errorResult := h.OriginService.DeleteOriginsGroup(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("delete origins group", errorResult)
	}

	return &userapi.DeleteOriginsGroupResponse{}, nil
}

func (h *OriginsGroupServiceHandler) ActivateOriginsGroup(ctx context.Context, request *userapi.ActivateOriginsGroupRequest) (*userapi.ActivateOriginsGroupResponse, error) {
	ctxlog.Info(ctx, h.Logger, "activate origins group", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithGroupFolderID(ctx, auth.UpdateOriginPermission, request.GroupId)
	if err != nil {
		return nil, err
	}

	errorResult := h.OriginService.ActivateOriginsGroup(ctx, &model.ActivateOriginsGroupParams{
		OriginsGroupID: request.GroupId,
		Version:        request.Version,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("activate origins group", errorResult)
	}

	return &userapi.ActivateOriginsGroupResponse{}, nil
}

func (h *OriginsGroupServiceHandler) ListOriginsGroupVersions(ctx context.Context, request *userapi.ListOriginsGroupVersionsRequest) (*userapi.ListOriginsGroupVersionsResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list origins group versions", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithGroupFolderID(ctx, auth.GetOriginPermission, request.GroupId)
	if err != nil {
		return nil, err
	}

	groupVersions, errorResult := h.OriginService.ListOriginsGroupVersions(ctx, &model.ListOriginsGroupVersionsParams{
		OriginsGroupID: request.GroupId,
		Versions:       nil,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("list origins group versions", errorResult)
	}

	return &userapi.ListOriginsGroupVersionsResponse{
		Groups: mapper.MakePBOriginsGroups(groupVersions),
	}, nil
}

func (h *OriginsGroupServiceHandler) CalculateOriginsGroupDiff(ctx context.Context, request *userapi.CalculateOriginsGroupDiffRequest) (*userapi.CalculateOriginsGroupDiffResponse, error) {
	ctxlog.Info(ctx, h.Logger, "calculate origins group diff", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.GroupId, rule.Required),
		valid.Value(&request.VersionLeft, rule.Required),
		valid.Value(&request.VersionRight, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}
	if request.VersionLeft >= request.VersionRight {
		return nil, grpcutil.ValidationError("version right must be greater than version left")
	}

	err = h.authorizeWithGroupFolderID(ctx, auth.GetOriginPermission, request.GroupId)
	if err != nil {
		return nil, err
	}

	groups, errorResult := h.OriginService.ListOriginsGroupVersions(ctx, &model.ListOriginsGroupVersionsParams{
		OriginsGroupID: request.GroupId,
		Versions:       []int64{request.VersionLeft, request.VersionRight},
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("list origins group versions", errorResult)
	}
	if len(groups) != 2 {
		return nil, grpcutil.NotFoundError("origins group not found")
	}
	sort.Slice(groups, func(i, j int) bool {
		return groups[i].Meta.Version < groups[j].Meta.Version
	})

	groupDiff, err := diff.CalculateDiff(
		fmt.Sprintf("origins group version: %d", groups[0].Meta.Version), mapper.MakePBOriginsGroup(groups[0]),
		fmt.Sprintf("origins group version: %d", groups[1].Meta.Version), mapper.MakePBOriginsGroup(groups[1]),
	)
	if err != nil {
		return nil, grpcutil.WrapInternalError("calculate diff", err)
	}

	return &userapi.CalculateOriginsGroupDiffResponse{
		Diff: groupDiff,
	}, nil
}

func (h *OriginsGroupServiceHandler) authorizeWithGroupFolderID(ctx context.Context, permission auth.Permission, groupID int64) error {
	folderID, errorResult := h.getFolderIDByGroupID(ctx, groupID)
	if errorResult != nil {
		return grpcutil.ExtractErrorResult("get folderID", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, permission, folder(folderID))
	if err != nil {
		return grpcutil.WrapAuthError(err)
	}

	return nil
}

func (h *OriginsGroupServiceHandler) getFolderIDByGroupID(ctx context.Context, groupID int64) (string, errors.ErrorResult) {
	val, ok := h.Cache.Get(groupID)
	if ok {
		return val.(string), nil
	}

	originsGroup, errorResult := h.OriginService.GetGroupWithoutOrigins(ctx, &model.GetOriginsGroupParams{
		OriginsGroupID: groupID,
	})
	if errorResult != nil {
		return "", errorResult
	}

	h.Cache.Add(groupID, originsGroup.FolderID)
	return originsGroup.FolderID, nil
}

func calculateNextPage(page *model.Pagination, entitiesLen int) uint32 {
	if entitiesLen < int(page.PageSize) {
		return page.NextPageToken()
	}

	return 0
}

func makeOriginParams(origins []*userapi.OriginParams) ([]*model.OriginParams, error) {
	result := make([]*model.OriginParams, 0, len(origins))
	for _, origin := range origins {
		originType, err := makeOriginType(origin.Type)
		if err != nil {
			return nil, err
		}
		result = append(result, &model.OriginParams{
			Source:  origin.Source,
			Enabled: origin.Enabled,
			Backup:  origin.Backup,
			Type:    originType,
		})
	}

	return result, nil
}
