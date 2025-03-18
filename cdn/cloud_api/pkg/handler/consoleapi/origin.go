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

type OriginServiceHandler struct {
	Logger        log.Logger
	AuthClient    grpcutil.AuthClient
	OriginService originservice.OriginService
	GroupService  originservice.OriginsGroupService
	Cache         FolderIDCache
}

func (h *OriginServiceHandler) Get(ctx context.Context, request *cdnpb.GetOriginRequest) (*cdnpb.Origin, error) {
	ctxlog.Info(ctx, h.Logger, "get origin", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
		valid.Value(&request.OriginId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithGroupFolderID(ctx, auth.GetOriginPermission, request.OriginGroupId, request.FolderId)
	if err != nil {
		return nil, err
	}

	origin, errorResult := h.OriginService.GetOrigin(ctx, &model.GetOriginParams{
		OriginsGroupID: request.OriginGroupId,
		OriginID:       request.OriginId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get origin", errorResult)
	}

	return makePBOrigin(origin), nil
}

func (h *OriginServiceHandler) List(ctx context.Context, request *cdnpb.ListOriginsRequest) (*cdnpb.ListOriginsResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list origins", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithGroupFolderID(ctx, auth.ListOriginsPermission, request.OriginGroupId, request.FolderId)
	if err != nil {
		return nil, err
	}

	originModels, errorResult := h.OriginService.GetAllOrigins(ctx, &model.GetAllOriginParams{
		OriginsGroupID: request.OriginGroupId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get all origins", errorResult)
	}

	return &cdnpb.ListOriginsResponse{
		Origins: makePBOrigins(originModels),
	}, nil
}

func (h *OriginServiceHandler) Create(ctx context.Context, request *cdnpb.CreateOriginRequest) (*cdnpb.Origin, error) {
	ctxlog.Info(ctx, h.Logger, "create origin", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.AuthClient.Authorize(ctx, auth.CreateOriginPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	source, originType, err := makeOriginSource(request.Source, request.Type, request.Meta)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin source", err)
	}

	params := &model.CreateOriginParams{
		FolderID:       request.FolderId,
		OriginsGroupID: request.OriginGroupId,
		OriginParams: model.OriginParams{
			Source:  source,
			Enabled: extractOptionalBool(request.Enabled, true),
			Backup:  extractOptionalBool(request.Backup, false),
			Type:    originType,
		},
	}

	origin, errorResult := h.OriginService.CreateOrigin(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("create origin", errorResult)
	}

	return makePBOrigin(origin), nil
}

func (h *OriginServiceHandler) Update(ctx context.Context, request *cdnpb.UpdateOriginRequest) (*cdnpb.Origin, error) {
	ctxlog.Info(ctx, h.Logger, "update origin", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
		valid.Value(&request.OriginId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithGroupFolderID(ctx, auth.UpdateOriginPermission, request.OriginGroupId, request.FolderId)
	if err != nil {
		return nil, err
	}

	source, originType, err := makeOriginSource(request.Source, request.Type, request.Meta)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin source", err)
	}

	params := &model.UpdateOriginParams{
		FolderID:       request.FolderId,
		OriginsGroupID: request.OriginGroupId,
		OriginID:       request.OriginId,
		OriginParams: model.OriginParams{
			Source:  source,
			Enabled: request.Enabled,
			Backup:  request.Backup,
			Type:    originType,
		},
	}

	origin, errorResult := h.OriginService.UpdateOrigin(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("update origin", errorResult)
	}

	return makePBOrigin(origin), nil
}

func (h *OriginServiceHandler) Delete(ctx context.Context, request *cdnpb.DeleteOriginRequest) (*emptypb.Empty, error) {
	ctxlog.Info(ctx, h.Logger, "delete origin", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
		valid.Value(&request.OriginGroupId, rule.IsPositive),
		valid.Value(&request.OriginId, rule.IsPositive),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithGroupFolderID(ctx, auth.DeleteOriginPermission, request.OriginGroupId, request.FolderId)
	if err != nil {
		return nil, err
	}

	errorResult := h.OriginService.DeleteOrigin(ctx, &model.DeleteOriginParams{
		OriginsGroupID: request.OriginGroupId,
		OriginID:       request.OriginId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("delete origin", errorResult)
	}

	return &emptypb.Empty{}, nil
}

func (h *OriginServiceHandler) authorizeWithGroupFolderID(ctx context.Context, permission auth.Permission, groupID int64, requestFolderID string) error {
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

func makeOriginSource(source string, originType cdnpb.OriginType, meta *cdnpb.OriginMeta) (string, model.OriginType, error) {
	switch originType {
	case cdnpb.OriginType_COMMON, cdnpb.OriginType_BUCKET, cdnpb.OriginType_WEBSITE:
		break
	default:
		return "", 0, fmt.Errorf("unsupported origin type: %s", originType.String())
	}

	switch variant := meta.GetOriginMetaVariant().(type) {
	case *cdnpb.OriginMeta_Bucket:
		return variant.Bucket.BucketName, model.OriginTypeBucket, nil
	case *cdnpb.OriginMeta_Website:
		return variant.Website.BucketName, model.OriginTypeWebsite, nil
	}

	return source, model.OriginTypeCommon, nil
}

func makePBOrigins(origins []*model.Origin) []*cdnpb.Origin {
	result := make([]*cdnpb.Origin, 0, len(origins))
	for _, origin := range origins {
		result = append(result, makePBOrigin(origin))
	}

	return result
}

func makePBOrigin(origin *model.Origin) *cdnpb.Origin {
	return &cdnpb.Origin{
		Id:      origin.ID,
		Source:  origin.Source,
		Enabled: origin.Enabled,
		Backup:  origin.Backup,
		Type:    makePBOriginType(origin.Type),
		Meta:    makePBOriginMeta(origin.Source, origin.Type),
	}
}

func makePBOriginType(originType model.OriginType) cdnpb.OriginType {
	switch originType {
	case model.OriginTypeCommon:
		return cdnpb.OriginType_COMMON
	case model.OriginTypeBucket:
		return cdnpb.OriginType_BUCKET
	case model.OriginTypeWebsite:
		return cdnpb.OriginType_WEBSITE
	default:
		return cdnpb.OriginType_ORIGIN_TYPE_UNSPECIFIED
	}
}

func makePBOriginMeta(source string, originType model.OriginType) *cdnpb.OriginMeta {
	switch originType {
	case model.OriginTypeBucket:
		return &cdnpb.OriginMeta{
			OriginMetaVariant: &cdnpb.OriginMeta_Bucket{
				Bucket: &cdnpb.OriginBucketMeta{
					BucketName: source,
				},
			},
		}
	case model.OriginTypeWebsite:
		return &cdnpb.OriginMeta{
			OriginMetaVariant: &cdnpb.OriginMeta_Website{
				Website: &cdnpb.OriginWebsiteMeta{
					BucketName: source,
				},
			},
		}
	default:
		return nil
	}
}
