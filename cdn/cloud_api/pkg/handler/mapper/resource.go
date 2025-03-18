package mapper

import (
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	pbmodel "a.yandex-team.ru/cdn/cloud_api/proto/model"
)

func MakePBResources(resources []*model.Resource) []*pbmodel.Resource {
	result := make([]*pbmodel.Resource, 0, len(resources))
	for _, resource := range resources {
		result = append(result, MakePBResource(resource))
	}

	return result
}

func MakePBResource(resource *model.Resource) *pbmodel.Resource {
	if resource == nil {
		return nil
	}

	return &pbmodel.Resource{
		Id:                 resource.ID,
		FolderId:           resource.FolderID,
		OriginsGroupId:     resource.OriginsGroupID,
		Active:             resource.Active,
		Name:               resource.Name,
		Cname:              resource.Cname,
		SecondaryHostnames: resource.SecondaryHostnames,
		OriginProtocol:     makeOriginProtocol(resource.OriginProtocol),
		Options:            MakePBOptions(resource.Options),
		VersionMeta:        makeEntityMeta(resource.Meta),
	}
}

func MakePBOptions(options *model.ResourceOptions) *pbmodel.ResourceOptions {
	if options == nil {
		return nil
	}

	return &pbmodel.ResourceOptions{
		CustomHost:       options.CustomHost,
		CustomSni:        options.CustomSNI,
		RedirectToHttps:  options.RedirectToHTTPS,
		AllowedMethods:   makePBOptionsAllowedMethods(options.AllowedMethods),
		Cors:             makePBCors(options.CORS),
		BrowserCache:     makePBBrowserCache(options.BrowserCacheOptions),
		EdgeCache:        makePBEdgeCache(options.EdgeCacheOptions),
		ServeStale:       makePBServeStale(options.ServeStaleOptions),
		NormalizeRequest: makePBNormalizeRequest(options.NormalizeRequestOptions),
		Compression:      makePBCompression(options.CompressionOptions),
		StaticHeaders:    makePBStaticHeaders(options.StaticHeadersOptions),
		Rewrite:          makePBRewrite(options.RewriteOptions),
	}
}

func makePBOptionsAllowedMethods(methods []model.AllowedMethod) []pbmodel.ResourceOptions_AllowedMethod {
	result := make([]pbmodel.ResourceOptions_AllowedMethod, 0, len(methods))
	for _, method := range methods {
		result = append(result, makePBOptionsAllowedMethod(method))
	}

	return result
}

func makePBOptionsAllowedMethod(method model.AllowedMethod) pbmodel.ResourceOptions_AllowedMethod {
	switch method {
	case model.AllowedMethodGet:
		return pbmodel.ResourceOptions_GET
	case model.AllowedMethodHead:
		return pbmodel.ResourceOptions_HEAD
	case model.AllowedMethodPost:
		return pbmodel.ResourceOptions_POST
	case model.AllowedMethodPut:
		return pbmodel.ResourceOptions_PUT
	case model.AllowedMethodPatch:
		return pbmodel.ResourceOptions_PATCH
	case model.AllowedMethodDelete:
		return pbmodel.ResourceOptions_DELETE
	case model.AllowedMethodOptions:
		return pbmodel.ResourceOptions_OPTIONS
	default:
		return pbmodel.ResourceOptions_ALLOWED_METHOD_UNSPECIFIED
	}
}

func makePBCors(options *model.CORSOptions) *pbmodel.CORSOptions {
	if options == nil {
		return nil
	}

	return &pbmodel.CORSOptions{
		Enabled:        options.Enabled,
		EnableTiming:   options.EnableTiming,
		Mode:           makePBCorsMode(options.Mode),
		AllowedOrigins: options.AllowedOrigins,
		AllowedMethods: makePBCORSAllowedMethods(options.AllowedMethods),
		AllowedHeaders: options.AllowedHeaders,
		MaxAge:         options.MaxAge,
		ExposeHeaders:  options.ExposeHeaders,
	}
}

func makePBCORSAllowedMethods(methods []model.AllowedMethod) []pbmodel.CORSOptions_AllowedMethod {
	result := make([]pbmodel.CORSOptions_AllowedMethod, 0, len(methods))
	for _, method := range methods {
		result = append(result, makePBCORSAllowedMethod(method))
	}

	return result
}

func makePBCORSAllowedMethod(method model.AllowedMethod) pbmodel.CORSOptions_AllowedMethod {
	switch method {
	case model.AllowedMethodGet:
		return pbmodel.CORSOptions_GET
	case model.AllowedMethodHead:
		return pbmodel.CORSOptions_HEAD
	case model.AllowedMethodPost:
		return pbmodel.CORSOptions_POST
	case model.AllowedMethodPut:
		return pbmodel.CORSOptions_PUT
	case model.AllowedMethodPatch:
		return pbmodel.CORSOptions_PATCH
	case model.AllowedMethodDelete:
		return pbmodel.CORSOptions_DELETE
	case model.AllowedMethodOptions:
		return pbmodel.CORSOptions_OPTIONS
	case model.AllowedMethodStar:
		return pbmodel.CORSOptions_STAR
	default:
		return pbmodel.CORSOptions_ALLOWED_METHOD_UNSPECIFIED
	}
}

func makePBCorsMode(mode *model.CORSMode) *pbmodel.CorsMode {
	if mode == nil {
		return nil
	}

	var pbmode pbmodel.CorsMode
	switch *mode {
	case model.CORSModeStar:
		pbmode = pbmodel.CorsMode_STAR
	case model.CORSModeOriginAny:
		pbmode = pbmodel.CorsMode_ORIGIN_ANY
	case model.CORSModeOriginFromList:
		pbmode = pbmodel.CorsMode_FROM_LIST
	default:

	}

	return &pbmode
}

func makePBBrowserCache(options *model.BrowserCacheOptions) *pbmodel.BrowserCacheOptions {
	if options == nil {
		return nil
	}

	return &pbmodel.BrowserCacheOptions{
		Enabled: options.Enabled,
		MaxAge:  options.MaxAge,
	}
}

func makePBEdgeCache(options *model.EdgeCacheOptions) *pbmodel.EdgeCacheOptions {
	if options == nil {
		return nil
	}

	return &pbmodel.EdgeCacheOptions{
		Enabled:          options.Enabled,
		UseRedirects:     options.UseRedirects,
		Ttl:              options.TTL,
		Override:         options.Override,
		OverrideTtlCodes: makePBOverrideTTLCodes(options.OverrideTTLCodes),
	}
}

func makePBOverrideTTLCodes(codes []model.OverrideTTLCode) []*pbmodel.OverrideTTLCode {
	result := make([]*pbmodel.OverrideTTLCode, 0, len(codes))
	for _, code := range codes {
		result = append(result, makePBOverrideTTLCode(code))
	}

	return result
}

func makePBOverrideTTLCode(code model.OverrideTTLCode) *pbmodel.OverrideTTLCode {
	return &pbmodel.OverrideTTLCode{
		Code: code.Code,
		Ttl:  code.TTL,
	}
}

func makePBServeStale(options *model.ServeStaleOptions) *pbmodel.ServeStaleOptions {
	if options == nil {
		return nil
	}

	return &pbmodel.ServeStaleOptions{
		Enabled: options.Enabled,
		Errors:  makePBServeStaleErrors(options.Errors),
	}
}

func makePBServeStaleErrors(errorTypes []model.ServeStaleErrorType) []pbmodel.ServeStaleError {
	result := make([]pbmodel.ServeStaleError, 0, len(errorTypes))
	for _, errorType := range errorTypes {
		result = append(result, makePBServeStaleError(errorType))
	}

	return result
}

func makePBServeStaleError(errorType model.ServeStaleErrorType) pbmodel.ServeStaleError {
	switch errorType {
	case model.ServeStaleError:
		return pbmodel.ServeStaleError_ERROR
	case model.ServeStaleTimeout:
		return pbmodel.ServeStaleError_TIMEOUT
	case model.ServeStaleInvalidHeader:
		return pbmodel.ServeStaleError_INVALID_HEADER
	case model.ServeStaleUpdating:
		return pbmodel.ServeStaleError_UPDATING
	case model.ServeStaleHTTP500:
		return pbmodel.ServeStaleError_HTTP500
	case model.ServeStaleHTTP502:
		return pbmodel.ServeStaleError_HTTP502
	case model.ServeStaleHTTP503:
		return pbmodel.ServeStaleError_HTTP503
	case model.ServeStaleHTTP504:
		return pbmodel.ServeStaleError_HTTP504
	case model.ServeStaleHTTP403:
		return pbmodel.ServeStaleError_HTTP403
	case model.ServeStaleHTTP404:
		return pbmodel.ServeStaleError_HTTP404
	case model.ServeStaleHTTP429:
		return pbmodel.ServeStaleError_HTTP429
	default:
		return pbmodel.ServeStaleError_SERVE_STALE_ERROR_UNSPECIFIED
	}
}

func makePBNormalizeRequest(options *model.NormalizeRequestOptions) *pbmodel.NormalizeRequestOptions {
	if options == nil {
		return nil
	}

	return &pbmodel.NormalizeRequestOptions{
		Cookies:     makePBNormalizeRequestCookies(options.Cookies),
		QueryString: makePBNormalizeRequestQueryString(options.QueryString),
	}
}

func makePBNormalizeRequestCookies(cookies model.NormalizeRequestCookies) *pbmodel.NormalizeRequestCookies {
	return &pbmodel.NormalizeRequestCookies{
		Ignore: cookies.Ignore,
	}
}

func makePBNormalizeRequestQueryString(queryString model.NormalizeRequestQueryString) *pbmodel.NormalizeRequestQueryString {
	switch {
	case queryString.Ignore != nil:
		return &pbmodel.NormalizeRequestQueryString{
			Variant: &pbmodel.NormalizeRequestQueryString_Ignore{
				Ignore: *queryString.Ignore,
			},
		}
	case len(queryString.Whitelist) != 0:
		return &pbmodel.NormalizeRequestQueryString{
			Variant: &pbmodel.NormalizeRequestQueryString_Whitelist{
				Whitelist: &pbmodel.NormalizeRequestQueryStringWhiteBlackList{
					Values: queryString.Whitelist,
				},
			},
		}
	case len(queryString.Blacklist) != 0:
		return &pbmodel.NormalizeRequestQueryString{
			Variant: &pbmodel.NormalizeRequestQueryString_Blacklist{
				Blacklist: &pbmodel.NormalizeRequestQueryStringWhiteBlackList{
					Values: queryString.Blacklist,
				},
			},
		}
	default:
		return nil
	}
}

func makePBCompression(options *model.CompressionOptions) *pbmodel.CompressionOptions {
	if options == nil {
		return nil
	}

	result := &pbmodel.CompressionOptions{
		Variant: nil,
	}

	variant := options.Variant

	switch {
	case variant.FetchCompressed != nil:
		result.Variant = &pbmodel.CompressionOptions_FetchCompressed{
			FetchCompressed: *variant.FetchCompressed,
		}
	case variant.Compress != nil:
		result.Variant = &pbmodel.CompressionOptions_Compress{
			Compress: &pbmodel.Compress{
				Compress: variant.Compress.Compress,
				Codecs:   makeCompressCodecs(variant.Compress.Codecs),
				Types:    variant.Compress.Types,
			},
		}
	}

	return result
}

func makeCompressCodecs(codecs []model.CompressCodec) []pbmodel.CompressCodec {
	result := make([]pbmodel.CompressCodec, 0, len(codecs))
	for _, codec := range codecs {
		result = append(result, makeCompressCodec(codec))
	}

	return result
}

func makeCompressCodec(codec model.CompressCodec) pbmodel.CompressCodec {
	switch codec {
	case model.CompressCodecGzip:
		return pbmodel.CompressCodec_GZIP
	case model.CompressCodecBrotli:
		return pbmodel.CompressCodec_BROTLI
	default:
		return pbmodel.CompressCodec_COMPRESS_CODEC_UNSPECIFIED
	}
}

func makePBStaticHeaders(options *model.StaticHeadersOptions) *pbmodel.StaticHeadersOptions {
	if options == nil {
		return nil
	}

	return &pbmodel.StaticHeadersOptions{
		Request:  makePBHeaderOptions(options.Request),
		Response: makePBHeaderOptions(options.Response),
	}
}

func makePBHeaderOptions(options []model.HeaderOption) []*pbmodel.HeaderOption {
	result := make([]*pbmodel.HeaderOption, 0, len(options))
	for _, option := range options {
		result = append(result, &pbmodel.HeaderOption{
			Name:   option.Name,
			Action: makePBHeaderAction(option.Action),
			Value:  option.Value,
		})
	}

	return result
}

func makePBHeaderAction(action model.HeaderAction) pbmodel.HeaderAction {
	switch action {
	case model.HeaderActionSet:
		return pbmodel.HeaderAction_SET
	case model.HeaderActionAppend:
		return pbmodel.HeaderAction_APPEND
	case model.HeaderActionRemove:
		return pbmodel.HeaderAction_REMOVE
	default:
		return pbmodel.HeaderAction_HEADER_ACTION_UNSPECIFIED
	}
}

func makePBRewrite(options *model.RewriteOptions) *pbmodel.RewriteHeaders {
	if options == nil {
		return nil
	}

	return &pbmodel.RewriteHeaders{
		Enabled:     options.Enabled,
		Regex:       options.Regex,
		Replacement: options.Replacement,
		Flag:        makePBRewriteFlag(options.Flag),
	}
}

func makePBRewriteFlag(flag *model.RewriteFlag) *pbmodel.RewriteFlag {
	if flag == nil {
		return nil
	}

	var pbflag pbmodel.RewriteFlag
	switch *flag {
	case model.RewriteFlagLast:
		pbflag = pbmodel.RewriteFlag_LAST
	case model.RewriteFlagBreak:
		pbflag = pbmodel.RewriteFlag_BREAK
	case model.RewriteFlagRedirect:
		pbflag = pbmodel.RewriteFlag_REDIRECT
	case model.RewriteFlagPermanent:
		pbflag = pbmodel.RewriteFlag_PERMANENT
	default:
		pbflag = pbmodel.RewriteFlag_REWRITE_FLAG_UNSPECIFIED
	}

	return &pbflag
}

func makeOriginProtocol(protocol model.OriginProtocol) pbmodel.OriginProtocol {
	switch protocol {
	case model.OriginProtocolHTTP:
		return pbmodel.OriginProtocol_HTTP
	case model.OriginProtocolHTTPS:
		return pbmodel.OriginProtocol_HTTPS
	case model.OriginProtocolSame:
		return pbmodel.OriginProtocol_SAME
	default:
		return pbmodel.OriginProtocol_ORIGIN_PROTOCOL_UNSPECIFIED
	}
}

func MakePBRules(rules []*model.ResourceRule) []*pbmodel.ResourceRule {
	result := make([]*pbmodel.ResourceRule, 0, len(rules))
	for _, rule := range rules {
		result = append(result, MakePBRule(rule))
	}

	return result
}

func MakePBRule(rule *model.ResourceRule) *pbmodel.ResourceRule {
	if rule == nil {
		return nil
	}

	return &pbmodel.ResourceRule{
		Id:              rule.ID,
		ResourceId:      rule.ResourceID,
		Name:            rule.Name,
		Pattern:         rule.Pattern,
		OriginsGroupId:  rule.OriginsGroupEntityID,
		OriginProtocol:  makeNullableOriginProtocol(rule.OriginProtocol),
		ResourceOptions: MakePBOptions(rule.Options),
	}
}

func makeNullableOriginProtocol(protocol *model.OriginProtocol) *pbmodel.OriginProtocol {
	if protocol == nil {
		return nil
	}

	result := makeOriginProtocol(*protocol)
	return &result
}
