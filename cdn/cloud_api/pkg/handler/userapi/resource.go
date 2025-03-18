package userapi

import (
	"context"
	"fmt"
	"sort"
	"strings"

	"a.yandex-team.ru/cdn/cloud_api/pkg/diff"
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
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type ResourceServiceHandler struct {
	Logger          log.Logger
	AuthClient      grpcutil.AuthClient
	ResourceService resourceservice.ResourceService
	RuleService     resourceservice.ResourceRuleService
	Cache           FolderIDCache
}

func (h *ResourceServiceHandler) CreateResource(ctx context.Context, request *userapi.CreateResourceRequest) (*pbmodel.Resource, error) {
	ctxlog.Info(ctx, h.Logger, "create resource", log.Sprintf("request", "%+v", request))

	err := h.AuthClient.Authorize(ctx, auth.CreateResourcePermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	originVariant, err := makeOriginVariant(request.OriginVariant)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin variant", err)
	}

	resourceOptions, err := makeResourceOptions(request.Options)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make resource options", err)
	}

	originProtocol, err := makeOriginProtocol(request.OriginProtocol)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin protocol", err)
	}

	params := &model.CreateResourceParams{
		FolderID:           request.FolderId,
		OriginVariant:      originVariant,
		Active:             request.Active,
		Name:               request.Name,
		SecondaryHostnames: request.SecondaryHostnames,
		OriginProtocol:     originProtocol,
		Options:            resourceOptions,
	}

	resource, errorResult := h.ResourceService.CreateResource(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("create resource", errorResult)
	}

	return mapper.MakePBResource(resource), nil
}

func (h *ResourceServiceHandler) GetResource(ctx context.Context, request *userapi.GetResourceRequest) (*pbmodel.Resource, error) {
	ctxlog.Info(ctx, h.Logger, "get resource", log.Sprintf("request", "%+v", request))

	resource, errorResult := h.ResourceService.GetResource(ctx, &model.GetResourceParams{
		ResourceID: request.ResourceId,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("get resource", errorResult)
	}

	err := h.AuthClient.Authorize(ctx, auth.GetResourcePermission, entityResource(resource.FolderID, resource.ID))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	return mapper.MakePBResource(resource), nil
}

func (h *ResourceServiceHandler) ListResources(ctx context.Context, request *userapi.ListResourcesRequest) (*userapi.ListResourcesResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list resources", log.Sprintf("request", "%+v", request))

	err := h.AuthClient.Authorize(ctx, auth.ListResourcesPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	page := makePage(request.PageSize, request.PageToken)

	resources, errorResult := h.ResourceService.GetAllResources(ctx, &model.GetAllResourceParams{
		FolderID: &request.FolderId,
		Page:     page,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("get all resources", errorResult)
	}

	return &userapi.ListResourcesResponse{
		Resources:     mapper.MakePBResources(resources),
		NextPageToken: calculateNextPage(page, len(resources)),
	}, nil
}

func (h *ResourceServiceHandler) UpdateResource(ctx context.Context, request *userapi.UpdateResourceRequest) (*pbmodel.Resource, error) {
	ctxlog.Info(ctx, h.Logger, "update resource", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.UpdateResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceOptions, err := makeResourceOptions(request.Options)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make resource options", err)
	}

	originProtocol, err := makeNullableOriginProtocol(request.OriginProtocol)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin protocol", err)
	}

	params := &model.UpdateResourceParams{
		ResourceID:         request.ResourceId,
		OriginsGroupID:     extractInt64(request.OriginsGroupId),
		Active:             request.Active,
		SecondaryHostnames: makeNullableSecondaryHostnames(request.SecondaryHostnames),
		OriginProtocol:     originProtocol,
		Options:            resourceOptions,
	}

	resource, errorResult := h.ResourceService.UpdateResource(ctx, params)
	if errorResult != nil {
		return nil, mapper.MakePBError("update resource", errorResult)
	}

	return mapper.MakePBResource(resource), nil
}

func (h *ResourceServiceHandler) DeleteResource(ctx context.Context, request *userapi.DeleteResourceRequest) (*userapi.DeleteResourceResponse, error) {
	ctxlog.Info(ctx, h.Logger, "delete resource", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.DeleteResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	errorResult := h.ResourceService.DeleteResource(ctx, &model.DeleteResourceParams{
		ResourceID: request.ResourceId,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("delete resource", errorResult)
	}

	return &userapi.DeleteResourceResponse{}, nil
}

func (h *ResourceServiceHandler) ActivateResource(ctx context.Context, request *userapi.ActivateResourceRequest) (*userapi.ActivateResourceResponse, error) {
	ctxlog.Info(ctx, h.Logger, "activate resource", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.UpdateResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	errorResult := h.ResourceService.ActivateResource(ctx, &model.ActivateResourceParams{
		ResourceID: request.ResourceId,
		Version:    request.Version,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("activate resource", errorResult)
	}

	return &userapi.ActivateResourceResponse{}, nil
}

func (h *ResourceServiceHandler) ListResourceVersions(ctx context.Context, request *userapi.ListResourceVersionsRequest) (*userapi.ListResourceVersionsResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list resource versions", log.Sprintf("request", "%+v", request))

	err := h.authorizeWithResource(ctx, auth.GetResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	versions, errorResult := h.ResourceService.ListResourceVersions(ctx, &model.ListResourceVersionsParams{
		ResourceID: request.ResourceId,
		Versions:   nil,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("list resource versions", errorResult)
	}

	return &userapi.ListResourceVersionsResponse{
		Resources: mapper.MakePBResources(versions),
	}, nil
}

func (h *ResourceServiceHandler) CalculateResourceDiff(ctx context.Context, request *userapi.CalculateResourceDiffRequest) (*userapi.CalculateResourceDiffResponse, error) {
	ctxlog.Info(ctx, h.Logger, "calculate resource diff", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.ResourceId, rule.Required),
		valid.Value(&request.VersionLeft, rule.Required),
		valid.Value(&request.VersionRight, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}
	if request.VersionLeft >= request.VersionRight {
		return nil, grpcutil.ValidationError("version right must be greater than version left")
	}

	err = h.authorizeWithResource(ctx, auth.GetResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resources, errorResult := h.ResourceService.ListResourceVersions(ctx, &model.ListResourceVersionsParams{
		ResourceID: request.ResourceId,
		Versions:   []int64{request.VersionLeft, request.VersionRight},
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("list resource versions", errorResult)
	}
	if len(resources) != 2 {
		return nil, grpcutil.NotFoundError("resource not found")
	}
	sort.Slice(resources, func(i, j int) bool {
		return resources[i].Meta.Version < resources[j].Meta.Version
	})

	resourceDiff, err := diff.CalculateDiff(
		fmt.Sprintf("resource version: %d", resources[0].Meta.Version), mapper.MakePBResource(resources[0]),
		fmt.Sprintf("resource version: %d", resources[1].Meta.Version), mapper.MakePBResource(resources[1]),
	)
	if err != nil {
		return nil, grpcutil.WrapInternalError("calculate resource diff", err)
	}

	leftRules, errorResult := h.RuleService.GetAllRulesByResource(ctx, &model.GetAllRulesByResourceParams{
		ResourceID:      request.ResourceId,
		ResourceVersion: &request.VersionLeft,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("left resource rules", errorResult)
	}

	rightRules, errorResult := h.RuleService.GetAllRulesByResource(ctx, &model.GetAllRulesByResourceParams{
		ResourceID:      request.ResourceId,
		ResourceVersion: &request.VersionRight,
	})
	if errorResult != nil {
		return nil, mapper.MakePBError("right resource rules", errorResult)
	}

	rulesDiff, err := diff.CalculateDiff(
		fmt.Sprintf("resource version: %d rules", resources[0].Meta.Version), mapper.MakePBRules(leftRules),
		fmt.Sprintf("resource version: %d rules", resources[1].Meta.Version), mapper.MakePBRules(rightRules),
	)
	if err != nil {
		return nil, grpcutil.WrapInternalError("calculate rules diff", err)
	}

	builder := strings.Builder{}
	builder.WriteString("Resource: ")
	builder.WriteString(request.ResourceId)
	builder.WriteRune('\n')
	builder.WriteString(resourceDiff)

	builder.WriteString("Rules: ")
	builder.WriteRune('\n')
	builder.WriteString(rulesDiff)

	return &userapi.CalculateResourceDiffResponse{
		Diff: builder.String(),
	}, nil
}

func (h *ResourceServiceHandler) authorizeWithResource(ctx context.Context, permission auth.Permission, resourceID string) error {
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

func (h *ResourceServiceHandler) getFolderIDByResourceID(ctx context.Context, resourceID string) (string, errors.ErrorResult) {
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

func makeNullableSecondaryHostnames(hostnames *userapi.OptionalStringList) *[]string {
	if hostnames == nil {
		return nil
	}

	result := hostnames.Values
	return &result
}

func makeOriginVariant(variant *userapi.CreateResourceRequest_OriginVariant) (model.OriginVariant, error) {
	result := model.OriginVariant{
		GroupID: 0,
		Source:  nil,
	}

	if variant == nil {
		return result, fmt.Errorf("origin variant is nil")
	}

	switch v := variant.Variant.(type) {
	case *userapi.CreateResourceRequest_OriginVariant_OriginGroupId:
		result.GroupID = v.OriginGroupId
	case *userapi.CreateResourceRequest_OriginVariant_OriginSourceParams:
		originType, err := makeOriginType(v.OriginSourceParams.Type)
		if err != nil {
			return result, err
		}
		result.Source = &model.OriginVariantSource{
			Source: v.OriginSourceParams.Source,
			Type:   originType,
		}
	}

	return result, nil
}

func makeOriginType(originType pbmodel.OriginType) (model.OriginType, error) {
	switch originType {
	case pbmodel.OriginType_COMMON:
		return model.OriginTypeCommon, nil
	case pbmodel.OriginType_BUCKET:
		return model.OriginTypeBucket, nil
	case pbmodel.OriginType_WEBSITE:
		return model.OriginTypeWebsite, nil
	default:
		return 0, fmt.Errorf("unknown origin type: %d", originType)
	}
}

func makeResourceOptions(options *pbmodel.ResourceOptions) (*model.ResourceOptions, error) {
	if options == nil {
		return nil, nil
	}

	allowedMethods, err := makeOptionsAllowedMethods(options.AllowedMethods)
	if err != nil {
		return nil, err
	}

	cors, err := makeCORS(options.Cors)
	if err != nil {
		return nil, err
	}

	staleOptions, err := makeServeStaleOptions(options.ServeStale)
	if err != nil {
		return nil, err
	}

	compressionOptions, err := makeCompressionOptions(options.Compression)
	if err != nil {
		return nil, err
	}

	headersOptions, err := makeStaticHeadersOptions(options.StaticHeaders)
	if err != nil {
		return nil, err
	}

	rewriteOptions, err := makeRewriteOptions(options.Rewrite)
	if err != nil {
		return nil, err
	}

	return &model.ResourceOptions{
		CustomHost:              options.CustomHost,
		CustomSNI:               options.CustomSni,
		RedirectToHTTPS:         options.RedirectToHttps,
		AllowedMethods:          allowedMethods,
		CORS:                    cors,
		BrowserCacheOptions:     makeBrowserCacheOptions(options.BrowserCache),
		EdgeCacheOptions:        makeEdgeCacheOptions(options.EdgeCache),
		ServeStaleOptions:       staleOptions,
		NormalizeRequestOptions: makeNormalizeRequestOptions(options.NormalizeRequest),
		CompressionOptions:      compressionOptions,
		StaticHeadersOptions:    headersOptions,
		RewriteOptions:          rewriteOptions,
	}, nil
}

func makeOriginProtocol(protocol pbmodel.OriginProtocol) (model.OriginProtocol, error) {
	switch protocol {
	case pbmodel.OriginProtocol_HTTP:
		return model.OriginProtocolHTTP, nil
	case pbmodel.OriginProtocol_HTTPS:
		return model.OriginProtocolHTTPS, nil
	case pbmodel.OriginProtocol_SAME:
		return model.OriginProtocolSame, nil
	default:
		return 0, fmt.Errorf("unknown origin protocol: %d", protocol)
	}
}

func makeNullableOriginProtocol(protocol *pbmodel.OriginProtocol) (*model.OriginProtocol, error) {
	if protocol == nil {
		return nil, nil
	}

	var originProtocol model.OriginProtocol
	switch *protocol {
	case pbmodel.OriginProtocol_HTTP:
		originProtocol = model.OriginProtocolHTTP
	case pbmodel.OriginProtocol_HTTPS:
		originProtocol = model.OriginProtocolHTTPS
	case pbmodel.OriginProtocol_SAME:
		originProtocol = model.OriginProtocolSame
	default:
		return nil, fmt.Errorf("unknown origin protocol: %d", *protocol)
	}

	return &originProtocol, nil
}

func makeOptionsAllowedMethods(methods []pbmodel.ResourceOptions_AllowedMethod) ([]model.AllowedMethod, error) {
	result := make([]model.AllowedMethod, 0, len(methods))
	for _, method := range methods {
		modelMethod, err := makeOptionsAllowedMethod(method)
		if err != nil {
			return nil, err
		}
		result = append(result, modelMethod)
	}

	return result, nil
}

func makeOptionsAllowedMethod(method pbmodel.ResourceOptions_AllowedMethod) (model.AllowedMethod, error) {
	switch method {
	case pbmodel.ResourceOptions_GET:
		return model.AllowedMethodGet, nil
	case pbmodel.ResourceOptions_HEAD:
		return model.AllowedMethodHead, nil
	case pbmodel.ResourceOptions_POST:
		return model.AllowedMethodPost, nil
	case pbmodel.ResourceOptions_PUT:
		return model.AllowedMethodPut, nil
	case pbmodel.ResourceOptions_PATCH:
		return model.AllowedMethodPatch, nil
	case pbmodel.ResourceOptions_DELETE:
		return model.AllowedMethodDelete, nil
	case pbmodel.ResourceOptions_OPTIONS:
		return model.AllowedMethodOptions, nil
	default:
		return 0, fmt.Errorf("unknown allowed method: %d", method)
	}
}

func makeCORS(options *pbmodel.CORSOptions) (*model.CORSOptions, error) {
	if options == nil {
		return nil, nil
	}

	mode, err := makeCORSMode(options.Mode)
	if err != nil {
		return nil, err
	}

	allowedMethods, err := makeCORSAllowedMethods(options.AllowedMethods)
	if err != nil {
		return nil, err
	}

	return &model.CORSOptions{
		Enabled:        options.Enabled,
		EnableTiming:   options.EnableTiming,
		Mode:           mode,
		AllowedOrigins: options.AllowedOrigins,
		AllowedMethods: allowedMethods,
		AllowedHeaders: options.AllowedHeaders,
		MaxAge:         options.MaxAge,
		ExposeHeaders:  options.ExposeHeaders,
	}, nil
}

func makeCORSAllowedMethods(methods []pbmodel.CORSOptions_AllowedMethod) ([]model.AllowedMethod, error) {
	result := make([]model.AllowedMethod, 0, len(methods))
	for _, method := range methods {
		modelMethod, err := makeCORSAllowedMethod(method)
		if err != nil {
			return nil, err
		}
		result = append(result, modelMethod)
	}

	return result, nil
}

func makeCORSAllowedMethod(method pbmodel.CORSOptions_AllowedMethod) (model.AllowedMethod, error) {
	switch method {
	case pbmodel.CORSOptions_GET:
		return model.AllowedMethodGet, nil
	case pbmodel.CORSOptions_HEAD:
		return model.AllowedMethodHead, nil
	case pbmodel.CORSOptions_POST:
		return model.AllowedMethodPost, nil
	case pbmodel.CORSOptions_PUT:
		return model.AllowedMethodPut, nil
	case pbmodel.CORSOptions_PATCH:
		return model.AllowedMethodPatch, nil
	case pbmodel.CORSOptions_DELETE:
		return model.AllowedMethodDelete, nil
	case pbmodel.CORSOptions_OPTIONS:
		return model.AllowedMethodOptions, nil
	case pbmodel.CORSOptions_STAR:
		return model.AllowedMethodStar, nil
	default:
		return 0, fmt.Errorf("unknown allowed method: %d", method)
	}
}

func makeCORSMode(mode *pbmodel.CorsMode) (*model.CORSMode, error) {
	if mode == nil {
		return nil, nil
	}

	var modelMode model.CORSMode
	switch *mode {
	case pbmodel.CorsMode_STAR:
		modelMode = model.CORSModeStar
	case pbmodel.CorsMode_ORIGIN_ANY:
		modelMode = model.CORSModeOriginAny
	case pbmodel.CorsMode_FROM_LIST:
		modelMode = model.CORSModeOriginFromList
	default:
		return nil, fmt.Errorf("unknown cors mode: %d", *mode)
	}

	return &modelMode, nil
}

func makeBrowserCacheOptions(options *pbmodel.BrowserCacheOptions) *model.BrowserCacheOptions {
	if options == nil {
		return nil
	}

	return &model.BrowserCacheOptions{
		Enabled: options.Enabled,
		MaxAge:  options.MaxAge,
	}
}

func makeEdgeCacheOptions(options *pbmodel.EdgeCacheOptions) *model.EdgeCacheOptions {
	if options == nil {
		return nil
	}

	return &model.EdgeCacheOptions{
		Enabled:          options.Enabled,
		UseRedirects:     options.UseRedirects,
		TTL:              options.Ttl,
		Override:         options.Override,
		OverrideTTLCodes: makeOverrideTTLCodes(options.OverrideTtlCodes),
	}
}

func makeOverrideTTLCodes(codes []*pbmodel.OverrideTTLCode) []model.OverrideTTLCode {
	result := make([]model.OverrideTTLCode, 0, len(codes))
	for _, code := range codes {
		result = append(result, model.OverrideTTLCode{
			Code: code.Code,
			TTL:  code.Ttl,
		})
	}

	return result
}

func makeServeStaleOptions(options *pbmodel.ServeStaleOptions) (*model.ServeStaleOptions, error) {
	if options == nil {
		return nil, nil
	}

	staleErrors, err := makeServeStaleErrors(options.Errors)
	if err != nil {
		return nil, err
	}

	return &model.ServeStaleOptions{
		Enabled: options.Enabled,
		Errors:  staleErrors,
	}, nil
}

func makeServeStaleErrors(staleErrors []pbmodel.ServeStaleError) ([]model.ServeStaleErrorType, error) {
	result := make([]model.ServeStaleErrorType, 0, len(staleErrors))
	for _, staleError := range staleErrors {
		serveStaleError, err := makeServeStaleError(staleError)
		if err != nil {
			return nil, err
		}
		result = append(result, serveStaleError)
	}

	return result, nil
}

func makeServeStaleError(staleError pbmodel.ServeStaleError) (model.ServeStaleErrorType, error) {
	switch staleError {
	case pbmodel.ServeStaleError_ERROR:
		return model.ServeStaleError, nil
	case pbmodel.ServeStaleError_TIMEOUT:
		return model.ServeStaleTimeout, nil
	case pbmodel.ServeStaleError_INVALID_HEADER:
		return model.ServeStaleInvalidHeader, nil
	case pbmodel.ServeStaleError_UPDATING:
		return model.ServeStaleUpdating, nil
	case pbmodel.ServeStaleError_HTTP500:
		return model.ServeStaleHTTP500, nil
	case pbmodel.ServeStaleError_HTTP502:
		return model.ServeStaleHTTP502, nil
	case pbmodel.ServeStaleError_HTTP503:
		return model.ServeStaleHTTP503, nil
	case pbmodel.ServeStaleError_HTTP504:
		return model.ServeStaleHTTP504, nil
	case pbmodel.ServeStaleError_HTTP403:
		return model.ServeStaleHTTP403, nil
	case pbmodel.ServeStaleError_HTTP404:
		return model.ServeStaleHTTP404, nil
	case pbmodel.ServeStaleError_HTTP429:
		return model.ServeStaleHTTP429, nil
	default:
		return 0, fmt.Errorf("unknown serve stale error: %d", staleError)
	}
}

func makeNormalizeRequestOptions(options *pbmodel.NormalizeRequestOptions) *model.NormalizeRequestOptions {
	if options == nil {
		return nil
	}

	return &model.NormalizeRequestOptions{
		Cookies:     makeCookies(options.Cookies),
		QueryString: makeQueryString(options.QueryString),
	}
}

func makeCookies(cookies *pbmodel.NormalizeRequestCookies) model.NormalizeRequestCookies {
	result := model.NormalizeRequestCookies{
		Ignore: nil,
	}

	if cookies == nil {
		return result
	}

	result.Ignore = cookies.Ignore

	return result
}

func makeQueryString(queryString *pbmodel.NormalizeRequestQueryString) model.NormalizeRequestQueryString {
	result := model.NormalizeRequestQueryString{
		Ignore:    nil,
		Whitelist: nil,
		Blacklist: nil,
	}

	if queryString == nil {
		return result
	}

	switch v := queryString.Variant.(type) {
	case *pbmodel.NormalizeRequestQueryString_Ignore:
		result.Ignore = ptr.Bool(v.Ignore)
	case *pbmodel.NormalizeRequestQueryString_Whitelist:
		result.Whitelist = v.Whitelist.GetValues()
	case *pbmodel.NormalizeRequestQueryString_Blacklist:
		result.Blacklist = v.Blacklist.GetValues()
	}

	return result
}

func makeCompressionOptions(options *pbmodel.CompressionOptions) (*model.CompressionOptions, error) {
	if options == nil {
		return nil, nil
	}

	switch v := options.Variant.(type) {
	case *pbmodel.CompressionOptions_FetchCompressed:
		return &model.CompressionOptions{
			Variant: model.CompressionVariant{
				FetchCompressed: ptr.Bool(v.FetchCompressed),
				Compress:        nil,
			},
		}, nil
	case *pbmodel.CompressionOptions_Compress:
		codecs, err := makeCompressCodecs(v.Compress.Codecs)
		if err != nil {
			return nil, err
		}

		return &model.CompressionOptions{
			Variant: model.CompressionVariant{
				FetchCompressed: nil,
				Compress: &model.Compress{
					Compress: v.Compress.Compress,
					Codecs:   codecs,
					Types:    v.Compress.Types,
				},
			},
		}, nil
	default:
		return nil, fmt.Errorf("unknown compression options variant")
	}
}

func makeCompressCodecs(codecs []pbmodel.CompressCodec) ([]model.CompressCodec, error) {
	result := make([]model.CompressCodec, 0, len(codecs))
	for _, codec := range codecs {
		compressCodec, err := makeCompressCodec(codec)
		if err != nil {
			return nil, err
		}
		result = append(result, compressCodec)
	}

	return result, nil
}

func makeCompressCodec(codec pbmodel.CompressCodec) (model.CompressCodec, error) {
	switch codec {
	case pbmodel.CompressCodec_GZIP:
		return model.CompressCodecGzip, nil
	case pbmodel.CompressCodec_BROTLI:
		return model.CompressCodecBrotli, nil
	default:
		return 0, fmt.Errorf("unknown compress codec: %d", codec)
	}
}

func makeStaticHeadersOptions(options *pbmodel.StaticHeadersOptions) (*model.StaticHeadersOptions, error) {
	if options == nil {
		return nil, nil
	}

	request, err := makeHeaderOptions(options.Request)
	if err != nil {
		return nil, err
	}

	response, err := makeHeaderOptions(options.Response)
	if err != nil {
		return nil, err
	}

	return &model.StaticHeadersOptions{
		Request:  request,
		Response: response,
	}, nil
}

func makeHeaderOptions(headers []*pbmodel.HeaderOption) ([]model.HeaderOption, error) {
	result := make([]model.HeaderOption, 0, len(headers))
	for _, header := range headers {
		action, err := makeHeaderAction(header.Action)
		if err != nil {
			return nil, err
		}
		result = append(result, model.HeaderOption{
			Name:   header.Name,
			Action: action,
			Value:  header.Value,
		})
	}

	return result, nil
}

func makeHeaderAction(action pbmodel.HeaderAction) (model.HeaderAction, error) {
	switch action {
	case pbmodel.HeaderAction_SET:
		return model.HeaderActionSet, nil
	case pbmodel.HeaderAction_APPEND:
		return model.HeaderActionAppend, nil
	case pbmodel.HeaderAction_REMOVE:
		return model.HeaderActionRemove, nil
	default:
		return 0, fmt.Errorf("unknown header action: %d", action)
	}
}

func makeRewriteOptions(options *pbmodel.RewriteHeaders) (*model.RewriteOptions, error) {
	if options == nil {
		return nil, nil
	}

	flag, err := makeRewriteFlag(options.Flag)
	if err != nil {
		return nil, err
	}

	return &model.RewriteOptions{
		Enabled:     options.Enabled,
		Regex:       options.Regex,
		Replacement: options.Replacement,
		Flag:        flag,
	}, nil
}

func makeRewriteFlag(flag *pbmodel.RewriteFlag) (*model.RewriteFlag, error) {
	if flag == nil {
		return nil, nil
	}

	var modelFlag model.RewriteFlag
	switch *flag {
	case pbmodel.RewriteFlag_LAST:
		modelFlag = model.RewriteFlagLast
	case pbmodel.RewriteFlag_BREAK:
		modelFlag = model.RewriteFlagBreak
	case pbmodel.RewriteFlag_REDIRECT:
		modelFlag = model.RewriteFlagRedirect
	case pbmodel.RewriteFlag_PERMANENT:
		modelFlag = model.RewriteFlagPermanent
	default:
		return nil, fmt.Errorf("unknown rewrite flag: %d", flag)
	}

	return &modelFlag, nil
}

func makePage(pageSize, pageToken uint32) *model.Pagination {
	return &model.Pagination{
		PageToken: pageToken,
		PageSize:  pageSize,
	}
}
