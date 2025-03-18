package resourceservice

import (
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func makeEntityAllowedMethods(methods []model.AllowedMethod) []storage.AllowedMethod {
	result := make([]storage.AllowedMethod, 0, len(methods))
	for _, method := range methods {
		result = append(result, storage.AllowedMethod(method))
	}

	return result
}

func makeEntityCORSOptions(options *model.CORSOptions) *storage.CORSOptions {
	if options == nil {
		return nil
	}

	return &storage.CORSOptions{
		Enabled:      options.Enabled,
		EnableTiming: options.EnableTiming,
		Mode: func() *storage.CORSMode {
			if options.Mode == nil {
				return nil
			}
			result := storage.CORSMode(*options.Mode)
			return &result
		}(),
		AllowedOrigins: options.AllowedOrigins,
		AllowedMethods: makeEntityAllowedMethods(options.AllowedMethods),
		AllowedHeaders: options.AllowedHeaders,
		MaxAge:         options.MaxAge,
		ExposeHeaders:  options.ExposeHeaders,
	}
}

func makeEntityBrowserCacheOptions(options *model.BrowserCacheOptions) *storage.BrowserCacheOptions {
	if options == nil {
		return nil
	}

	return &storage.BrowserCacheOptions{
		Enabled: options.Enabled,
		MaxAge:  options.MaxAge,
	}
}

func makeEntityEdgeCacheOptions(options *model.EdgeCacheOptions) *storage.EdgeCacheOptions {
	if options == nil {
		return nil
	}

	return &storage.EdgeCacheOptions{
		Enabled:          options.Enabled,
		UseRedirects:     options.UseRedirects,
		TTL:              options.TTL,
		Override:         options.Override,
		OverrideTTLCodes: makeEntityOverrideTTLCodes(options.OverrideTTLCodes),
	}
}

func makeEntityOverrideTTLCodes(codes []model.OverrideTTLCode) []storage.OverrideTTLCode {
	result := make([]storage.OverrideTTLCode, 0, len(codes))
	for _, code := range codes {
		result = append(result, storage.OverrideTTLCode{
			Code: code.Code,
			TTL:  code.TTL,
		})
	}

	return result
}

func makeEntityServeStaleOptions(options *model.ServeStaleOptions) *storage.ServeStaleOptions {
	if options == nil {
		return nil
	}

	return &storage.ServeStaleOptions{
		Enabled: options.Enabled,
		Errors:  makeEntityServeStaleErrorArray(options.Errors),
	}
}

func makeEntityServeStaleErrorArray(serveStaleErrors []model.ServeStaleErrorType) []storage.ServeStaleErrorType {
	result := make([]storage.ServeStaleErrorType, 0, len(serveStaleErrors))
	for _, serverStaleError := range serveStaleErrors {
		result = append(result, storage.ServeStaleErrorType(serverStaleError))
	}

	return result
}

func makeEntityNormalizeRequestOptions(options *model.NormalizeRequestOptions) *storage.NormalizeRequestOptions {
	if options == nil {
		return nil
	}

	return &storage.NormalizeRequestOptions{
		Cookies: storage.NormalizeRequestCookies{
			Ignore: options.Cookies.Ignore,
		},
		QueryString: storage.NormalizeRequestQueryString{
			Ignore:    options.QueryString.Ignore,
			Whitelist: options.QueryString.Whitelist,
			Blacklist: options.QueryString.Blacklist,
		},
	}
}

func makeEntityCompressionOptions(options *model.CompressionOptions) *storage.CompressionOptions {
	if options == nil {
		return nil
	}

	return &storage.CompressionOptions{
		Variant: storage.CompressionVariant{
			FetchCompressed: options.Variant.FetchCompressed,
			Compress:        makeEntityCompress(options.Variant.Compress),
		},
	}
}

func makeEntityCompress(compress *model.Compress) *storage.Compress {
	if compress == nil {
		return nil
	}

	return &storage.Compress{
		Compress: compress.Compress,
		Codecs:   makeEntityCompressCodecArray(compress.Codecs),
		Types:    compress.Types,
	}
}

func makeEntityCompressCodecArray(codecs []model.CompressCodec) []storage.CompressCodec {
	result := make([]storage.CompressCodec, 0, len(codecs))
	for _, codec := range codecs {
		result = append(result, storage.CompressCodec(codec))
	}

	return result
}

func makeEntityStaticHeadersOptions(options *model.StaticHeadersOptions) *storage.StaticHeadersOptions {
	if options == nil {
		return nil
	}

	return &storage.StaticHeadersOptions{
		Request:  makeEntityHeaderOptionArray(options.Request),
		Response: makeEntityHeaderOptionArray(options.Response),
	}
}

func makeEntityHeaderOptionArray(options []model.HeaderOption) []storage.HeaderOption {
	result := make([]storage.HeaderOption, 0, len(options))
	for _, option := range options {
		result = append(result, storage.HeaderOption{
			Name:   option.Name,
			Action: storage.HeaderAction(option.Action),
			Value:  option.Value,
		})
	}

	return result
}

func makeEntityRewriteOptions(options *model.RewriteOptions) *storage.RewriteOptions {
	if options == nil {
		return nil
	}

	return &storage.RewriteOptions{
		Enabled:     options.Enabled,
		Regex:       options.Regex,
		Replacement: options.Replacement,
		Flag: func() *storage.RewriteFlag {
			if options.Flag == nil {
				return nil
			}
			flag := storage.RewriteFlag(*options.Flag)
			return &flag
		}(),
	}
}
