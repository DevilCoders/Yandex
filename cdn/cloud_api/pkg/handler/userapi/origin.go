package userapi

import (
	"context"

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
)

type OriginServiceHandler struct {
	Logger        log.Logger
	AuthClient    grpcutil.AuthClient
	OriginService originservice.OriginService
	GroupService  originservice.OriginsGroupService
	Cache         FolderIDCache
}

func (h *OriginServiceHandler) CreateOrigin(ctx context.Context, request *userapi.CreateOriginRequest) (*pbmodel.Origin, error) {
	ctxlog.Info(ctx, h.Logger, "create origin", log.Sprintf("request", "%+v", request))

	originType, err := makeOriginType(request.Type)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin type", err)
	}

	err = h.AuthClient.Authorize(ctx, auth.CreateOriginPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	params := &model.CreateOriginParams{
		FolderID:       request.FolderId,
		OriginsGroupID: request.GroupId,
		OriginParams: model.OriginParams{
			Source:  request.Source,
			Enabled: request.Enabled,
			Backup:  request.Backup,
			Type:    originType,
		},
	}

	origin, errorResult := h.OriginService.CreateOrigin(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("create origin", errorResult)
	}

	return mapper.MakePBOrigin(origin), nil
}

func (h *OriginServiceHandler) GetOrigin(ctx context.Context, request *userapi.GetOriginRequest) (*pbmodel.Origin, error) {
	ctxlog.Info(ctx, h.Logger, "get origin", log.Sprintf("request", "%+v", request))

	origin, errorResult := h.OriginService.GetOrigin(ctx, &model.GetOriginParams{
		OriginsGroupID: request.GroupId,
		OriginID:       request.OriginId,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("get origin", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, auth.GetOriginPermission, folder(origin.FolderID))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	return mapper.MakePBOrigin(origin), nil
}

func (h *OriginServiceHandler) ListOrigins(ctx context.Context, request *userapi.ListOriginsRequest) (*userapi.ListOriginsResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list origins", log.Sprintf("request", "%+v", request))

	origins, errorResult := h.OriginService.GetAllOrigins(ctx, &model.GetAllOriginParams{
		OriginsGroupID: request.GroupId,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("get all origins", errorResult)
	}

	if len(origins) > 0 {
		err := h.AuthClient.Authorize(ctx, auth.ListOriginsPermission, folder(origins[0].FolderID))
		if err != nil {
			return nil, grpcutil.WrapAuthError(err)
		}
	}

	return &userapi.ListOriginsResponse{
		Origins: mapper.MakePBOrigins(origins),
	}, nil
}

func (h *OriginServiceHandler) UpdateOrigin(ctx context.Context, request *userapi.UpdateOriginRequest) (*pbmodel.Origin, error) {
	ctxlog.Info(ctx, h.Logger, "update origin", log.Sprintf("request", "%+v", request))

	folderID, errorResult := h.getFolderIDByGroupID(ctx, request.GroupId)
	if errorResult != nil {
		return nil, mapper.MakePBError("get folderID", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, auth.UpdateOriginPermission, folder(folderID))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	originType, err := makeOriginType(request.Type)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin type", err)
	}

	params := &model.UpdateOriginParams{
		FolderID:       folderID,
		OriginsGroupID: request.GroupId,
		OriginID:       request.OriginId,
		OriginParams: model.OriginParams{
			Source:  request.Source,
			Enabled: request.Enabled,
			Backup:  request.Backup,
			Type:    originType,
		},
	}

	origin, errorResult := h.OriginService.UpdateOrigin(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("update origin", errorResult)
	}

	return mapper.MakePBOrigin(origin), nil
}

func (h *OriginServiceHandler) DeleteOrigin(ctx context.Context, request *userapi.DeleteOriginRequest) (*userapi.DeleteOriginResponse, error) {
	ctxlog.Info(ctx, h.Logger, "delete origin", log.Sprintf("request", "%+v", request))

	folderID, errorResult := h.getFolderIDByGroupID(ctx, request.GroupId)
	if errorResult != nil {
		return nil, mapper.MakePBError("get folderID", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, auth.DeleteOriginPermission, folder(folderID))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	errorResult = h.OriginService.DeleteOrigin(ctx, &model.DeleteOriginParams{
		OriginsGroupID: request.GroupId,
		OriginID:       request.OriginId,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("delete origin", errorResult)
	}

	return &userapi.DeleteOriginResponse{}, nil
}

func (h *OriginServiceHandler) getFolderIDByGroupID(ctx context.Context, groupID int64) (string, errors.ErrorResult) {
	val, ok := h.Cache.Get(groupID)
	if ok {
		return val.(string), nil
	}

	originsGroup, errorResult := h.GroupService.GetGroupWithoutOrigins(ctx, &model.GetOriginsGroupParams{
		OriginsGroupID: groupID,
	})
	if errorResult != nil {
		return "", errorResult
	}

	h.Cache.Add(groupID, originsGroup.FolderID)
	return originsGroup.FolderID, nil
}
