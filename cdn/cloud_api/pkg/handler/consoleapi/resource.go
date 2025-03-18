package consoleapi

import (
	"context"
	"fmt"
	"net/http"
	"strconv"

	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/grpcutil"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/resourceservice"
	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
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
	Cache           FolderIDCache
}

func (h *ResourceServiceHandler) Get(ctx context.Context, request *cdnpb.GetResourceRequest) (*cdnpb.Resource, error) {
	ctxlog.Info(ctx, h.Logger, "get resource", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.ResourceId, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	resource, errorResult := h.ResourceService.GetResource(ctx, &model.GetResourceParams{
		ResourceID: request.ResourceId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get resource", errorResult)
	}

	err = h.AuthClient.Authorize(ctx, auth.GetResourcePermission, entityResource(resource.FolderID, resource.ID))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	return makePBResource(resource), nil
}

func (h *ResourceServiceHandler) List(ctx context.Context, request *cdnpb.ListResourcesRequest) (*cdnpb.ListResourcesResponse, error) {
	ctxlog.Info(ctx, h.Logger, "list resource", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.AuthClient.Authorize(ctx, auth.ListResourcesPermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	params := &model.GetAllResourceParams{
		FolderID: makeOptionalFolderID(request.FolderId),
		Page:     nil,
	}

	resources, errorResult := h.ResourceService.GetAllResources(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("get all resources", errorResult)
	}

	return &cdnpb.ListResourcesResponse{
		Resources: makePBResources(resources),
	}, nil
}

func (h *ResourceServiceHandler) Create(ctx context.Context, request *cdnpb.CreateResourceRequest) (*cdnpb.Resource, error) {
	ctxlog.Info(ctx, h.Logger, "create resource", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.FolderId, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.AuthClient.Authorize(ctx, auth.CreateResourcePermission, folder(request.FolderId))
	if err != nil {
		return nil, grpcutil.WrapAuthError(err)
	}

	variant, err := makeOriginVariant(request.Origin)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make origin variant", err)
	}

	protocol, err := makeOriginProtocol(request.OriginProtocol)
	if err != nil {
		return nil, err
	}

	resourceOptions, err := makeResourceOptions(request.Options)
	if err != nil {
		return nil, grpcutil.WrapMapperError("make resource options", err)
	}

	params := &model.CreateResourceParams{
		FolderID:           request.FolderId,
		OriginVariant:      variant,
		Active:             extractOptionalBool(request.Active, true),
		Name:               "",
		SecondaryHostnames: request.SecondaryHostnames,
		OriginProtocol:     protocol,
		Options:            resourceOptions,
	}

	resource, errorResult := h.ResourceService.CreateResource(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("create resource", errorResult)
	}

	return makePBResource(resource), nil
}

func (h *ResourceServiceHandler) Update(ctx context.Context, request *cdnpb.UpdateResourceRequest) (*cdnpb.Resource, error) {
	ctxlog.Info(ctx, h.Logger, "update resource", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.ResourceId, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithResource(ctx, auth.UpdateResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	resourceOptions, err := makeResourceOptions(request.Options)
	if err != nil {
		return nil, fmt.Errorf("make resource options: %w", err)
	}

	params := &model.UpdateResourceParams{
		ResourceID:         request.ResourceId,
		OriginsGroupID:     request.OriginGroupId.GetValue(),
		Active:             request.Active.GetValue(),
		SecondaryHostnames: makeNullableSecondaryHostnames(request.SecondaryHostnames),
		OriginProtocol:     makeNullableOriginProtocol(request.OriginProtocol),
		Options:            resourceOptions,
	}

	resource, errorResult := h.ResourceService.UpdateResource(ctx, params)
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("create resource", errorResult)
	}

	return makePBResource(resource), nil
}

func (h *ResourceServiceHandler) Delete(ctx context.Context, request *cdnpb.DeleteResourceRequest) (*operation.Operation, error) {
	ctxlog.Info(ctx, h.Logger, "delete resource", log.Sprintf("request", "%+v", request))

	err := valid.Struct(request,
		valid.Value(&request.ResourceId, rule.Required),
	)
	if err != nil {
		return nil, grpcutil.WrapValidateError(err)
	}

	err = h.authorizeWithResource(ctx, auth.DeleteResourcePermission, request.ResourceId)
	if err != nil {
		return nil, err
	}

	errorResult := h.ResourceService.DeleteResource(ctx, &model.DeleteResourceParams{
		ResourceID: request.ResourceId,
	})
	if errorResult != nil {
		return nil, grpcutil.ExtractErrorResult("delete resource", errorResult)
	}

	return &operation.Operation{}, nil // TODO: init?
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

// TODO: this is stub
func (h *ResourceServiceHandler) GetClientGCoreCName(ctx context.Context, request *cdnpb.GetGCoreCNameRequest) (*cdnpb.GetGCoreCNameResponse, error) {
	ctxlog.Info(ctx, h.Logger, "get client gcore cname request", log.String("folder_id", request.GetFolderId()))

	resp := &cdnpb.GetGCoreCNameResponse{
		Cname:    "dont.use.me",
		FolderId: request.FolderId,
	}
	return resp, nil
}

func makeNullableSecondaryHostnames(secondaryHostnames *cdnpb.UpdateResourceRequest_SecondaryHostnames) *[]string {
	if secondaryHostnames == nil {
		return nil
	}

	result := secondaryHostnames.Values
	return &result
}

func makeResourceOptions(options *cdnpb.ResourceOptions) (*model.ResourceOptions, error) {
	if options == nil {
		return nil, nil
	}

	if len(options.CacheHttpHeaders.GetValue()) != 0 {
		return nil, fmt.Errorf("cache http headers is not supported")
	}

	if options.Slice.GetValue() {
		return nil, fmt.Errorf("slice is not supported")
	}

	if options.ProxyCacheMethodsSet.GetValue() {
		return nil, fmt.Errorf("proxy cache methods set is not supported")
	}

	if options.DisableProxyForceRanges != nil && !options.DisableProxyForceRanges.Value {
		return nil, fmt.Errorf("disable proxy force ranges is not supported")
	}

	redirectToHTTPS, err := makeRedirectToHTTPS(options.RedirectOptions)
	if err != nil {
		return nil, err
	}

	allowedMethods, err := makeAllowedMethods(options.AllowedHttpMethods)
	if err != nil {
		return nil, err
	}

	edgeCacheOptions, err := makeEdgeCacheOptions(options.EdgeCacheSettings)
	if err != nil {
		return nil, err
	}

	serveStaleOptions, err := makeServeStaleOptions(options.Stale)
	if err != nil {
		return nil, err
	}

	rewriteOptions, err := makeRewriteOptions(options.Rewrite)
	if err != nil {
		return nil, err
	}

	return &model.ResourceOptions{
		CustomHost:              makeCustomHost(options.HostOptions),
		CustomSNI:               makeCustomHost(options.HostOptions),
		RedirectToHTTPS:         redirectToHTTPS,
		AllowedMethods:          allowedMethods,
		CORS:                    makeCORSOptions(options.Cors),
		BrowserCacheOptions:     makeBrowserCacheOptions(options.BrowserCacheSettings),
		EdgeCacheOptions:        edgeCacheOptions,
		ServeStaleOptions:       serveStaleOptions,
		NormalizeRequestOptions: makeNormalizeRequestOptions(options.IgnoreCookie, options.QueryParamsOptions),
		CompressionOptions:      makeCompressionOptions(options.CompressionOptions),
		StaticHeadersOptions:    makeStaticHeadersOptions(options.StaticHeaders, options.StaticRequestHeaders),
		RewriteOptions:          rewriteOptions,
	}, nil
}

func makeCustomHost(hostOptions *cdnpb.ResourceOptions_HostOptions) *string {
	if hostOptions == nil {
		return nil
	}

	switch variant := hostOptions.HostVariant.(type) {
	case *cdnpb.ResourceOptions_HostOptions_Host:
		return ptr.String(variant.Host.GetValue())
	}

	return nil
}

func makeRedirectToHTTPS(redirectOptions *cdnpb.ResourceOptions_RedirectOptions) (*bool, error) {
	if redirectOptions == nil {
		return nil, nil
	}

	switch redirectOptions.RedirectVariant.(type) {
	case *cdnpb.ResourceOptions_RedirectOptions_RedirectHttpToHttps:
		return ptr.Bool(true), nil
	case *cdnpb.ResourceOptions_RedirectOptions_RedirectHttpsToHttp:
		return nil, fmt.Errorf("redirect to http is not supported")
	}

	return ptr.Bool(false), nil
}

func makeAllowedMethods(allowedMethods *cdnpb.ResourceOptions_StringsListOption) ([]model.AllowedMethod, error) {
	if allowedMethods == nil {
		return nil, nil
	}

	methodStrings := allowedMethods.GetValue()
	result := make([]model.AllowedMethod, 0, len(methodStrings))
	for _, str := range methodStrings {
		var method model.AllowedMethod

		switch str {
		case http.MethodGet:
			method = model.AllowedMethodGet
		case http.MethodHead:
			method = model.AllowedMethodHead
		case http.MethodPost:
			method = model.AllowedMethodPost
		case http.MethodPut:
			method = model.AllowedMethodPut
		case http.MethodPatch:
			method = model.AllowedMethodPatch
		case http.MethodDelete:
			method = model.AllowedMethodDelete
		case http.MethodOptions:
			method = model.AllowedMethodOptions
		default:
			return nil, fmt.Errorf("unknown http method: %s", str)
		}

		result = append(result, method)
	}

	return result, nil
}

func makeOriginProtocol(protocol cdnpb.OriginProtocol) (model.OriginProtocol, error) {
	switch protocol {
	case cdnpb.OriginProtocol_HTTP:
		return model.OriginProtocolHTTP, nil
	case cdnpb.OriginProtocol_HTTPS:
		return model.OriginProtocolHTTPS, nil
	case cdnpb.OriginProtocol_MATCH:
		return model.OriginProtocolSame, nil
	default:
		return 0, fmt.Errorf("unknown origin protocol: %d", protocol)
	}
}

func makeNullableOriginProtocol(protocol cdnpb.OriginProtocol) *model.OriginProtocol {
	var modelProtocol model.OriginProtocol
	switch protocol {
	case cdnpb.OriginProtocol_HTTP:
		modelProtocol = model.OriginProtocolHTTP
	case cdnpb.OriginProtocol_HTTPS:
		modelProtocol = model.OriginProtocolHTTPS
	case cdnpb.OriginProtocol_MATCH:
		modelProtocol = model.OriginProtocolSame
	default:
		return nil
	}
	return &modelProtocol
}

func makeCORSOptions(corsOptions *cdnpb.ResourceOptions_StringsListOption) *model.CORSOptions {
	if corsOptions == nil {
		return nil
	}

	result := &model.CORSOptions{
		Enabled:        ptr.Bool(corsOptions.GetEnabled()),
		EnableTiming:   ptr.Bool(false),
		Mode:           nil,
		AllowedOrigins: nil,
		AllowedMethods: nil,
		AllowedHeaders: nil,
		MaxAge:         nil,
		ExposeHeaders:  nil,
	}

	originValues := corsOptions.GetValue()

	if len(originValues) == 1 {
		value := originValues[0]
		switch value {
		case "*":
			mode := model.CORSModeStar
			result.Mode = &mode
		case "$http_origin":
			mode := model.CORSModeOriginAny
			result.Mode = &mode
		default:
			mode := model.CORSModeOriginFromList
			result.Mode = &mode
			result.AllowedOrigins = originValues
		}
	} else {
		mode := model.CORSModeOriginFromList
		result.Mode = &mode
		result.AllowedOrigins = originValues
	}

	return result
}

func makeBrowserCacheOptions(options *cdnpb.ResourceOptions_Int64Option) *model.BrowserCacheOptions {
	if options == nil {
		return nil
	}

	return &model.BrowserCacheOptions{
		Enabled: ptr.Bool(options.Enabled),
		MaxAge:  ptr.Int64(options.Value),
	}
}

func makeEdgeCacheOptions(options *cdnpb.ResourceOptions_EdgeCacheSettings) (*model.EdgeCacheOptions, error) {
	if options == nil {
		return nil, nil
	}

	result := &model.EdgeCacheOptions{
		Enabled:          ptr.Bool(options.Enabled),
		UseRedirects:     nil,
		TTL:              nil,
		Override:         nil,
		OverrideTTLCodes: nil,
	}

	switch variant := options.ValuesVariant.(type) {
	case *cdnpb.ResourceOptions_EdgeCacheSettings_Value:
		result.TTL = ptr.Int64(variant.Value.SimpleValue)
		result.Override = ptr.Bool(true)
		codes, err := makeOverrideTTLCodes(variant.Value.CustomValues)
		if err != nil {
			return nil, err
		}
		result.OverrideTTLCodes = codes
	case *cdnpb.ResourceOptions_EdgeCacheSettings_DefaultValue:
		result.TTL = ptr.Int64(variant.DefaultValue)
	}

	return result, nil
}

func makeOverrideTTLCodes(customValues map[string]int64) ([]model.OverrideTTLCode, error) {
	result := make([]model.OverrideTTLCode, 0, len(customValues))
	for k, v := range customValues {
		httpCode, err := strconv.ParseInt(k, 10, 64)
		if err != nil {
			return nil, err
		}

		result = append(result, model.OverrideTTLCode{
			Code: httpCode,
			TTL:  v,
		})
	}

	return result, nil
}

func makeServeStaleOptions(staleOptions *cdnpb.ResourceOptions_StringsListOption) (*model.ServeStaleOptions, error) {
	if staleOptions == nil {
		return nil, nil
	}

	serveStaleErrors, err := makeServeStaleErrorArray(staleOptions.Value)
	if err != nil {
		return nil, err
	}

	return &model.ServeStaleOptions{
		Enabled: ptr.Bool(staleOptions.Enabled),
		Errors:  serveStaleErrors,
	}, nil
}

func makeServeStaleErrorArray(serveStaleErrors []string) ([]model.ServeStaleErrorType, error) {
	result := make([]model.ServeStaleErrorType, 0, len(serveStaleErrors))
	for _, serveStaleError := range serveStaleErrors {
		serveStaleErrorType, err := makeServeStaleError(serveStaleError)
		if err != nil {
			return nil, err
		}

		result = append(result, serveStaleErrorType)
	}

	return result, nil
}

func makeNormalizeRequestOptions(ignoreCookie *cdnpb.ResourceOptions_BoolOption, queryParams *cdnpb.ResourceOptions_QueryParamsOptions) *model.NormalizeRequestOptions {
	result := &model.NormalizeRequestOptions{
		Cookies: model.NormalizeRequestCookies{
			Ignore: nil,
		},
		QueryString: model.NormalizeRequestQueryString{
			Ignore:    nil,
			Whitelist: nil,
			Blacklist: nil,
		},
	}

	if ignoreCookie != nil {
		result.Cookies.Ignore = ptr.Bool(ignoreCookie.GetValue())
	}

	if queryParams == nil {
		return result
	}

	switch params := queryParams.QueryParamsVariant.(type) {
	case *cdnpb.ResourceOptions_QueryParamsOptions_IgnoreQueryString:
		result.QueryString.Ignore = ptr.Bool(params.IgnoreQueryString.GetValue())
	case *cdnpb.ResourceOptions_QueryParamsOptions_QueryParamsWhitelist:
		result.QueryString.Whitelist = params.QueryParamsWhitelist.GetValue()
	case *cdnpb.ResourceOptions_QueryParamsOptions_QueryParamsBlacklist:
		result.QueryString.Blacklist = params.QueryParamsBlacklist.GetValue()
	}

	return result
}

func makeCompressionOptions(options *cdnpb.ResourceOptions_CompressionOptions) *model.CompressionOptions {
	if options == nil {
		return nil
	}

	result := &model.CompressionOptions{
		Variant: model.CompressionVariant{
			FetchCompressed: nil,
			Compress:        nil,
		},
	}

	switch v := options.CompressionVariant.(type) {
	case *cdnpb.ResourceOptions_CompressionOptions_FetchCompressed:
		result.Variant.FetchCompressed = ptr.Bool(v.FetchCompressed.GetValue())
	case *cdnpb.ResourceOptions_CompressionOptions_GzipOn:
		result.Variant.Compress = &model.Compress{
			Compress: v.GzipOn.GetValue(),
			Codecs:   []model.CompressCodec{model.CompressCodecGzip},
			Types:    nil,
		}
	case *cdnpb.ResourceOptions_CompressionOptions_BrotliCompression:
		result.Variant.Compress = &model.Compress{
			Compress: v.BrotliCompression.GetEnabled(),
			Codecs:   []model.CompressCodec{model.CompressCodecBrotli},
			Types:    v.BrotliCompression.GetValue(),
		}
	}

	return result
}

func makeStaticHeadersOptions(staticHeaders *cdnpb.ResourceOptions_StringsMapOption, requestHeaders *cdnpb.ResourceOptions_StringsMapOption) *model.StaticHeadersOptions {
	if staticHeaders == nil {
		return nil
	}

	var request []model.HeaderOption
	for k, v := range requestHeaders.GetValue() {
		request = append(request, model.HeaderOption{
			Name:   k,
			Action: model.HeaderActionSet,
			Value:  v,
		})
	}

	var response []model.HeaderOption
	for k, v := range staticHeaders.GetValue() {
		response = append(response, model.HeaderOption{
			Name:   k,
			Action: model.HeaderActionSet,
			Value:  v,
		})
	}

	return &model.StaticHeadersOptions{
		Request:  request,
		Response: response,
	}
}

func makeRewriteOptions(options *cdnpb.ResourceOptions_RewriteOption) (*model.RewriteOptions, error) {
	if options == nil {
		return nil, nil
	}

	flag, err := makeRewriteFlag(options.Flag)
	if err != nil {
		return nil, err
	}

	return &model.RewriteOptions{
		Enabled:     ptr.Bool(options.Enabled),
		Regex:       ptr.String(options.Body),
		Replacement: nil,
		Flag:        flag,
	}, nil
}

func makeRewriteFlag(flag cdnpb.RewriteFlag) (*model.RewriteFlag, error) {
	var modelFlag model.RewriteFlag
	switch flag {
	case cdnpb.RewriteFlag_REWRITE_FLAG_LAST:
		modelFlag = model.RewriteFlagLast
	case cdnpb.RewriteFlag_REWRITE_FLAG_BREAK:
		modelFlag = model.RewriteFlagBreak
	case cdnpb.RewriteFlag_REWRITE_FLAG_REDIRECT:
		modelFlag = model.RewriteFlagRedirect
	case cdnpb.RewriteFlag_REWRITE_FLAG_PERMANENT:
		modelFlag = model.RewriteFlagPermanent
	case cdnpb.RewriteFlag_REWRITE_FLAG_UNSPECIFIED:
		return nil, nil
	default:
		return nil, fmt.Errorf("unknown rewrite flag: %d", flag)
	}

	return &modelFlag, nil
}

func makeOriginVariant(variant *cdnpb.CreateResourceRequest_Origin) (model.OriginVariant, error) {
	result := model.OriginVariant{
		GroupID: 0,
		Source:  nil,
	}

	if variant == nil {
		return result, fmt.Errorf("origin variant is nil")
	}

	switch ov := variant.OriginVariant.(type) {
	case *cdnpb.CreateResourceRequest_Origin_OriginGroupId:
		result.GroupID = ov.OriginGroupId
	case *cdnpb.CreateResourceRequest_Origin_OriginSource:
		result.Source = &model.OriginVariantSource{
			Source: ov.OriginSource,
			Type:   model.OriginTypeCommon,
		}
	case *cdnpb.CreateResourceRequest_Origin_OriginSourceParams:
		source, originType, err := makeOriginSource(ov.OriginSourceParams.Source, ov.OriginSourceParams.Type, ov.OriginSourceParams.Meta)
		if err != nil {
			return result, fmt.Errorf("make origin source: %w", err)
		}
		result.Source = &model.OriginVariantSource{
			Source: source,
			Type:   originType,
		}
	}

	return result, nil
}

func makePBResources(resources []*model.Resource) []*cdnpb.Resource {
	result := make([]*cdnpb.Resource, 0, len(resources))
	for _, resource := range resources {
		result = append(result, makePBResource(resource))
	}

	return result
}

func makePBResource(resource *model.Resource) *cdnpb.Resource {
	return &cdnpb.Resource{
		Id:                 resource.ID,
		FolderId:           resource.FolderID,
		Cname:              resource.Cname,
		CreatedAt:          timestamppb.New(resource.CreateAt),
		UpdatedAt:          timestamppb.New(resource.UpdatedAt),
		Active:             resource.Active,
		Options:            makePBOptions(resource.Options),
		SecondaryHostnames: resource.SecondaryHostnames,
		OriginsGroupId:     resource.OriginsGroupID,
		OriginGroupName:    "", // TODO: make an additional request to get this field?
		OriginProtocol:     makePBOriginProtocol(resource.OriginProtocol),
		SystemStatus:       "",
		SslCert: &cdnpb.SSLCert{
			Type:   cdnpb.SSLCertType_SSL_CERT_TYPE_UNSPECIFIED,
			Status: cdnpb.SSLCertStatus_SSL_CERT_STATUS_UNSPECIFIED,
			Data:   nil,
		},
	}
}

func makePBOptions(options *model.ResourceOptions) *cdnpb.ResourceOptions {
	if options == nil {
		return nil
	}

	return &cdnpb.ResourceOptions{
		DisableCache:         nil,
		EdgeCacheSettings:    makeEdgeCacheSettings(options.EdgeCacheOptions),
		BrowserCacheSettings: makePBBrowserCacheSettings(options.BrowserCacheOptions),
		CacheHttpHeaders:     nil,
		QueryParamsOptions:   makePBQueryParamsOptions(options.NormalizeRequestOptions),
		Slice: &cdnpb.ResourceOptions_BoolOption{
			Enabled: true,
			Value:   false,
		},
		CompressionOptions: makePBCompressionOptions(options.CompressionOptions),
		RedirectOptions:    makePBRedirectOptions(options.RedirectToHTTPS),
		HostOptions:        makePBHostOptions(options.CustomHost),
		StaticHeaders:      makePBStaticHeaders(options.StaticHeadersOptions),
		Cors:               makePBCors(options.CORS),
		Stale:              makePBStale(options.ServeStaleOptions),
		AllowedHttpMethods: makePBAllowedHTTPMethods(options.AllowedMethods),
		ProxyCacheMethodsSet: &cdnpb.ResourceOptions_BoolOption{
			Enabled: true,
			Value:   false,
		},
		DisableProxyForceRanges: &cdnpb.ResourceOptions_BoolOption{
			Enabled: true,
			Value:   true,
		},
		StaticRequestHeaders: makePBStaticRequestHeaders(options.StaticHeadersOptions),
		CustomServerName:     &cdnpb.ResourceOptions_StringOption{},
		IgnoreCookie:         makePBIgnoreCookie(options.NormalizeRequestOptions),
		Rewrite:              makePBRewrite(options.RewriteOptions),
	}
}

func makeEdgeCacheSettings(options *model.EdgeCacheOptions) *cdnpb.ResourceOptions_EdgeCacheSettings {
	if options == nil {
		return nil
	}

	result := &cdnpb.ResourceOptions_EdgeCacheSettings{
		Enabled:       extractBool(options.Enabled),
		ValuesVariant: nil,
	}

	if options.TTL == nil {
		return result
	}

	if options.Override != nil && *options.Override {
		result.ValuesVariant = &cdnpb.ResourceOptions_EdgeCacheSettings_Value{
			Value: &cdnpb.ResourceOptions_CachingTimes{
				SimpleValue:  *options.TTL,
				CustomValues: makePBOverrideTTLCodes(options.OverrideTTLCodes),
			},
		}
	} else {
		result.ValuesVariant = &cdnpb.ResourceOptions_EdgeCacheSettings_DefaultValue{
			DefaultValue: *options.TTL,
		}
	}

	return result
}

func makePBOverrideTTLCodes(codes []model.OverrideTTLCode) map[string]int64 {
	result := make(map[string]int64, len(codes))
	for _, code := range codes {
		strCode := strconv.FormatInt(code.Code, 10)
		result[strCode] = code.TTL
	}

	return result
}

func makePBBrowserCacheSettings(options *model.BrowserCacheOptions) *cdnpb.ResourceOptions_Int64Option {
	if options == nil {
		return nil
	}

	return &cdnpb.ResourceOptions_Int64Option{
		Enabled: extractBool(options.Enabled),
		Value:   extractInt64(options.MaxAge),
	}
}

func makePBQueryParamsOptions(options *model.NormalizeRequestOptions) *cdnpb.ResourceOptions_QueryParamsOptions {
	if options == nil {
		return nil
	}

	result := &cdnpb.ResourceOptions_QueryParamsOptions{
		QueryParamsVariant: nil,
	}

	switch {
	case options.QueryString.Ignore != nil:
		result.QueryParamsVariant = &cdnpb.ResourceOptions_QueryParamsOptions_IgnoreQueryString{
			IgnoreQueryString: &cdnpb.ResourceOptions_BoolOption{
				Enabled: true,
				Value:   *options.QueryString.Ignore,
			},
		}
	case len(options.QueryString.Whitelist) != 0:
		result.QueryParamsVariant = &cdnpb.ResourceOptions_QueryParamsOptions_QueryParamsWhitelist{
			QueryParamsWhitelist: &cdnpb.ResourceOptions_StringsListOption{
				Enabled: true,
				Value:   options.QueryString.Whitelist,
			},
		}
	case len(options.QueryString.Blacklist) != 0:
		result.QueryParamsVariant = &cdnpb.ResourceOptions_QueryParamsOptions_QueryParamsBlacklist{
			QueryParamsBlacklist: &cdnpb.ResourceOptions_StringsListOption{
				Enabled: true,
				Value:   options.QueryString.Blacklist,
			},
		}
	}

	return result
}

func makePBCompressionOptions(options *model.CompressionOptions) *cdnpb.ResourceOptions_CompressionOptions {
	if options == nil {
		return nil
	}

	result := &cdnpb.ResourceOptions_CompressionOptions{
		CompressionVariant: nil,
	}

	switch {
	case options.Variant.FetchCompressed != nil:
		result.CompressionVariant = &cdnpb.ResourceOptions_CompressionOptions_FetchCompressed{
			FetchCompressed: &cdnpb.ResourceOptions_BoolOption{
				Enabled: true,
				Value:   *options.Variant.FetchCompressed,
			},
		}
	case options.Variant.Compress != nil:
		compress := options.Variant.Compress
		if len(compress.Codecs) == 0 {
			return nil
		}

		codec := compress.Codecs[0]
		switch codec {
		case model.CompressCodecGzip:
			result.CompressionVariant = &cdnpb.ResourceOptions_CompressionOptions_GzipOn{
				GzipOn: &cdnpb.ResourceOptions_BoolOption{
					Enabled: true,
					Value:   compress.Compress,
				},
			}
		case model.CompressCodecBrotli:
			result.CompressionVariant = &cdnpb.ResourceOptions_CompressionOptions_BrotliCompression{
				BrotliCompression: &cdnpb.ResourceOptions_StringsListOption{
					Enabled: compress.Compress,
					Value:   compress.Types,
				},
			}
		}
	}

	return result
}

func makePBRedirectOptions(redirectToHTTPS *bool) *cdnpb.ResourceOptions_RedirectOptions {
	if redirectToHTTPS == nil || !*redirectToHTTPS {
		return nil
	}

	return &cdnpb.ResourceOptions_RedirectOptions{
		RedirectVariant: &cdnpb.ResourceOptions_RedirectOptions_RedirectHttpToHttps{
			RedirectHttpToHttps: &cdnpb.ResourceOptions_BoolOption{
				Enabled: true,
				Value:   *redirectToHTTPS,
			},
		},
	}
}

func makePBHostOptions(customHost *string) *cdnpb.ResourceOptions_HostOptions {
	if customHost == nil {
		return nil
	}

	return &cdnpb.ResourceOptions_HostOptions{
		HostVariant: &cdnpb.ResourceOptions_HostOptions_Host{
			Host: &cdnpb.ResourceOptions_StringOption{
				Enabled: true,
				Value:   *customHost,
			},
		},
	}
}

func makePBStaticHeaders(options *model.StaticHeadersOptions) *cdnpb.ResourceOptions_StringsMapOption {
	if options == nil {
		return nil
	}

	result := make(map[string]string, len(options.Response))
	for _, headerOption := range options.Response {
		result[headerOption.Name] = headerOption.Value
	}

	return &cdnpb.ResourceOptions_StringsMapOption{
		Enabled: true,
		Value:   result,
	}
}

func makePBCors(options *model.CORSOptions) *cdnpb.ResourceOptions_StringsListOption {
	if options == nil || options.Mode == nil {
		return nil
	}

	var value []string
	switch *options.Mode {
	case model.CORSModeStar:
		value = append(value, "*")
	case model.CORSModeOriginAny:
		value = append(value, "$http_origin")
	case model.CORSModeOriginFromList:
		value = append(value, options.AllowedOrigins...)
	}

	return &cdnpb.ResourceOptions_StringsListOption{
		Enabled: extractBool(options.Enabled),
		Value:   value,
	}
}

func makePBStale(options *model.ServeStaleOptions) *cdnpb.ResourceOptions_StringsListOption {
	if options == nil {
		return nil
	}

	return &cdnpb.ResourceOptions_StringsListOption{
		Enabled: extractBool(options.Enabled),
		Value:   makePBServeStaleErrors(options.Errors),
	}
}

func makePBServeStaleErrors(serveStaleErrors []model.ServeStaleErrorType) []string {
	result := make([]string, 0, len(serveStaleErrors))
	for _, serveStaleError := range serveStaleErrors {
		result = append(result, serveStaleError.String())
	}

	return result
}

func makePBAllowedHTTPMethods(allowedMethods []model.AllowedMethod) *cdnpb.ResourceOptions_StringsListOption {
	result := make([]string, 0, len(allowedMethods))
	for _, method := range allowedMethods {
		result = append(result, method.String())
	}

	return &cdnpb.ResourceOptions_StringsListOption{
		Enabled: true,
		Value:   result,
	}
}

func makePBStaticRequestHeaders(options *model.StaticHeadersOptions) *cdnpb.ResourceOptions_StringsMapOption {
	if options == nil {
		return nil
	}

	result := make(map[string]string, len(options.Request))
	for _, headerOption := range options.Request {
		result[headerOption.Name] = headerOption.Value
	}

	return &cdnpb.ResourceOptions_StringsMapOption{
		Enabled: true,
		Value:   result,
	}
}

func makePBIgnoreCookie(options *model.NormalizeRequestOptions) *cdnpb.ResourceOptions_BoolOption {
	if options == nil {
		return nil
	}

	return &cdnpb.ResourceOptions_BoolOption{
		Enabled: true,
		Value:   extractBool(options.Cookies.Ignore),
	}
}

func makePBRewrite(options *model.RewriteOptions) *cdnpb.ResourceOptions_RewriteOption {
	if options == nil {
		return nil
	}

	return &cdnpb.ResourceOptions_RewriteOption{
		Enabled: extractBool(options.Enabled),
		Body:    extractStringValue(options.Regex),
		Flag:    makePBFlag(options.Flag),
	}
}

func extractStringValue(str *string) string {
	if str != nil {
		return *str
	}
	return ""
}

func makePBFlag(flag *model.RewriteFlag) cdnpb.RewriteFlag {
	if flag == nil {
		return cdnpb.RewriteFlag_REWRITE_FLAG_UNSPECIFIED
	}

	switch *flag {
	case model.RewriteFlagLast:
		return cdnpb.RewriteFlag_REWRITE_FLAG_LAST
	case model.RewriteFlagBreak:
		return cdnpb.RewriteFlag_REWRITE_FLAG_BREAK
	case model.RewriteFlagRedirect:
		return cdnpb.RewriteFlag_REWRITE_FLAG_REDIRECT
	case model.RewriteFlagPermanent:
		return cdnpb.RewriteFlag_REWRITE_FLAG_PERMANENT
	default:
		return cdnpb.RewriteFlag_REWRITE_FLAG_UNSPECIFIED
	}
}

func makePBOriginProtocol(protocol model.OriginProtocol) cdnpb.OriginProtocol {
	switch protocol {
	case model.OriginProtocolHTTP:
		return cdnpb.OriginProtocol_HTTP
	case model.OriginProtocolHTTPS:
		return cdnpb.OriginProtocol_HTTPS
	case model.OriginProtocolSame:
		return cdnpb.OriginProtocol_MATCH
	default:
		return cdnpb.OriginProtocol_ORIGIN_PROTOCOL_UNSPECIFIED
	}
}

func extractBool(b *bool) bool {
	if b == nil {
		return false
	}
	return *b
}

func extractInt64(i *int64) int64 {
	if i == nil {
		return 0
	}
	return *i
}
