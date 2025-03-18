package consoleapi

import (
	"context"
	"fmt"

	"google.golang.org/protobuf/types/known/emptypb"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/grpcutil"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/originservice"
	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type OriginsGroupServiceHandler struct {
	Logger             log.Logger
	AuthClient         grpcutil.AuthClient
	OriginGroupService originservice.OriginsGroupService
	Cache              FolderIDCache
}

func (h *OriginsGroupServiceHandler) Get(ctx context.Context, request *cdnpb.GetOriginsGroupRequest) (*cdnpb.OriginsGroup, error) {
	ctxlog.Info(ctx, h.Logger, "get origins group", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.AuthClient.Authorize(ctx, auth.GetOriginPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	group, errorResult := h.OriginGroupService.GetOriginsGroup(ctx, &model.GetOriginsGroupParams{
		OriginsGroupID: request.OriginGroupId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get origins group", errorResult)
	}

	if group.FolderID != request.FolderId {
		return nil, grpcutil.ValidationError("invalid folder id in request")
	}

	return makePBOriginsGroup(group), nil
}

func (h *OriginsGroupServiceHandler) List(ctx context.Context, request *cdnpb.ListOriginsGroupsRequest) (*cdnpb.ListOriginsGroupsResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list origins group", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.AuthClient.Authorize(ctx, auth.ListOriginsPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	groups, errorResult := h.OriginGroupService.GetAllOriginsGroup(ctx, &model.GetAllOriginsGroupParams{
		FolderID: makeOptionalFolderID(request.FolderId),
		GroupIDs: nil,
		Page:     nil,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get all origins group", errorResult)
	}

	return &cdnpb.ListOriginsGroupsResponse{
		OriginsGroups: makePBOriginsGroups(groups),
	}, nil
}

func (h *OriginsGroupServiceHandler) Create(ctx context.Context, request *cdnpb.CreateOriginsGroupRequest) (*cdnpb.OriginsGroup, error) {
	ctxlog.Info(ctx, h.Logger, "create origins group", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.AuthClient.Authorize(ctx, auth.CreateOriginPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	originParams, err := makeOriginParams(request.Origins)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin params", err)
	}

	params := &model.CreateOriginsGroupParams{
		FolderID: request.FolderId,
		Name:     request.Name,
		UseNext:  extractOptionalBool(request.UseNext, true),
		Origins:  originParams,
	}

	group, errorResult := h.OriginGroupService.CreateOriginsGroup(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("create origins group", errorResult)
	}

	return makePBOriginsGroup(group), nil
}

func (h *OriginsGroupServiceHandler) Update(ctx context.Context, request *cdnpb.UpdateOriginsGroupRequest) (*cdnpb.OriginsGroup, error) {
	ctxlog.Info(ctx, h.Logger, "update origins group", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithGroupFolderID(ctx, auth.UpdateOriginPermission, request.OriginGroupId, request.FolderId)
	if err != nil {
		return nil, err
	}

	originParams, err := makeOriginParams(request.Origins)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin params", err)
	}

	params := &model.UpdateOriginsGroupParams{
		OriginsGroupID: request.OriginGroupId,
		Name:           request.GroupName.GetValue(),
		UseNext:        extractOptionalBool(request.UseNext, true),
		Origins:        originParams,
	}

	group, errorResult := h.OriginGroupService.UpdateOriginsGroup(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("update origins group", errorResult)
	}

	return makePBOriginsGroup(group), nil
}

func (h *OriginsGroupServiceHandler) Delete(ctx context.Context, request *cdnpb.DeleteOriginsGroupRequest) (*emptypb.Empty, error) {
	ctxlog.Info(ctx, h.Logger, "delete origins group", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithGroupFolderID(ctx, auth.DeleteOriginPermission, request.OriginGroupId, request.FolderId)
	if err != nil {
		return nil, err
	}

	errorResult := h.OriginGroupService.DeleteOriginsGroup(ctx, &model.DeleteOriginsGroupParams{
		OriginsGroupID: request.OriginGroupId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("delete origins group", errorResult)
	}

	return &emptypb.Empty{}, nil
}

func (h *OriginsGroupServiceHandler) authorizeWithGroupFolderID(
	ctx context.Context,
	permission auth.Permission,
	groupID int64,
	requestFolderID string,
) error {
	folderID, errorResult := h.getFolderIDByGroupID(ctx, groupID)
	if errorResult != nil {
		return grpcutil.ExtractErrorResult("get folderID", errorResult)
	}

	if folderID != requestFolderID {
		return grpcutil.ValidationError("invalid folder id in request")
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

	originsGroup, errorResult := h.OriginGroupService.GetGroupWithoutOrigins(ctx, &model.GetOriginsGroupParams{
		OriginsGroupID: groupID,
	})
	if errorResult != nil {
		return "", errorResult
	}

	h.Cache.Add(groupID, originsGroup.FolderID)
	return originsGroup.FolderID, nil
}

func makePBOriginsGroups(groups []*model.OriginsGroup) []*cdnpb.OriginsGroup {
	result := make([]*cdnpb.OriginsGroup, 0, len(groups))
	for _, group := range groups {
		result = append(result, makePBOriginsGroup(group))
	}

	return result
}

func makePBOriginsGroup(group *model.OriginsGroup) *cdnpb.OriginsGroup {
	return &cdnpb.OriginsGroup{
		Id:       group.ID,
		FolderId: group.FolderID,
		Name:     group.Name,
		UseNext:  group.UseNext,
		Origins:  makePBOrigins(group.Origins),
	}
}

func makeOriginParams(pb []*cdnpb.OriginParams) ([]*model.OriginParams, error) {
	result := make([]*model.OriginParams, 0, len(pb))
	for _, param := range pb {
		source, originType, err := makeOriginSource(param.Source, param.Type, param.Meta)
		if err != nil {
			return nil, fmt.Errorf("make origin source: %w", err)
		}

		result = append(result, &model.OriginParams{
			Source:  source,
			Enabled: param.Enabled,
			Backup:  param.Backup,
			Type:    originType,
		})
	}

	return result, nil
}
