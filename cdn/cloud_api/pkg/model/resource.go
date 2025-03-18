package model

import (
	"net/http"
	"time"

	"a.yandex-team.ru/library/go/ptr"
)

type Resource struct {
	ID             string
	FolderID       string
	OriginsGroupID int64

	Active             bool
	Name               string
	Cname              string
	SecondaryHostnames []string
	OriginProtocol     OriginProtocol

	Options *ResourceOptions

	CreateAt  time.Time
	UpdatedAt time.Time

	Meta *VersionMeta
}

type ResourceOptions struct {
	CustomHost      *string
	CustomSNI       *string
	RedirectToHTTPS *bool
	AllowedMethods  []AllowedMethod

	CORS                    *CORSOptions
	BrowserCacheOptions     *BrowserCacheOptions
	EdgeCacheOptions        *EdgeCacheOptions
	ServeStaleOptions       *ServeStaleOptions
	NormalizeRequestOptions *NormalizeRequestOptions
	CompressionOptions      *CompressionOptions
	StaticHeadersOptions    *StaticHeadersOptions
	RewriteOptions          *RewriteOptions
}

func SetResourceOptionsDefaults(optionsPtr *ResourceOptions) *ResourceOptions {
	var options ResourceOptions
	if optionsPtr != nil {
		options = *optionsPtr
	}

	if len(options.AllowedMethods) == 0 {
		options.AllowedMethods = []AllowedMethod{AllowedMethodGet, AllowedMethodHead, AllowedMethodOptions}
	}

	options.EdgeCacheOptions = setEdgeCacheOptionsDefaults(options.EdgeCacheOptions)
	options.NormalizeRequestOptions = setNormalizeRequestOptionsDefaults(options.NormalizeRequestOptions)
	options.CompressionOptions = setCompressionOptionsDefaults(options.CompressionOptions)

	return &options
}

const (
	Unspecified = "UNSPECIFIED"
)

type AllowedMethod int64

const (
	AllowedMethodGet AllowedMethod = iota
	AllowedMethodHead
	AllowedMethodPost
	AllowedMethodPut
	AllowedMethodPatch
	AllowedMethodDelete
	AllowedMethodOptions
	AllowedMethodStar
)

func (m AllowedMethod) String() string {
	switch m {
	case AllowedMethodGet:
		return http.MethodGet
	case AllowedMethodHead:
		return http.MethodHead
	case AllowedMethodPost:
		return http.MethodPost
	case AllowedMethodPut:
		return http.MethodPut
	case AllowedMethodPatch:
		return http.MethodPatch
	case AllowedMethodDelete:
		return http.MethodDelete
	case AllowedMethodOptions:
		return http.MethodOptions
	case AllowedMethodStar:
		return "*"
	default:
		return Unspecified
	}
}

type CORSOptions struct {
	Enabled        *bool
	EnableTiming   *bool
	Mode           *CORSMode
	AllowedOrigins []string
	AllowedMethods []AllowedMethod
	AllowedHeaders []string
	MaxAge         *int64
	ExposeHeaders  []string
}

type CORSMode int64

const (
	CORSModeStar CORSMode = iota
	CORSModeOriginAny
	CORSModeOriginFromList
)

type BrowserCacheOptions struct {
	Enabled *bool
	MaxAge  *int64
}

type EdgeCacheOptions struct {
	Enabled          *bool
	UseRedirects     *bool
	TTL              *int64
	Override         *bool
	OverrideTTLCodes []OverrideTTLCode
}

func setEdgeCacheOptionsDefaults(optionsPtr *EdgeCacheOptions) *EdgeCacheOptions {
	var options EdgeCacheOptions
	if optionsPtr != nil {
		options = *optionsPtr
	}

	if options.Enabled == nil {
		options.Enabled = ptr.Bool(true)
	}

	if options.UseRedirects == nil {
		options.UseRedirects = ptr.Bool(true)
	}

	if options.TTL == nil {
		options.TTL = ptr.Int64(int64((24 * time.Hour).Seconds()))
	}

	return &options
}

type OverrideTTLCode struct {
	Code int64
	TTL  int64
}

type ServeStaleOptions struct {
	Enabled *bool
	Errors  []ServeStaleErrorType
}

const (
	Error         = "ERROR"
	Timeout       = "TIMEOUT"
	InvalidHeader = "INVALID_HEADER"
	Updating      = "UPDATING"
	HTTP500       = "HTTP_500"
	HTTP502       = "HTTP_502"
	HTTP503       = "HTTP_503"
	HTTP504       = "HTTP_504"
	HTTP403       = "HTTP_403"
	HTTP404       = "HTTP_404"
	HTTP429       = "HTTP_429"
)

type ServeStaleErrorType int64

const (
	ServeStaleError ServeStaleErrorType = iota
	ServeStaleTimeout
	ServeStaleInvalidHeader
	ServeStaleUpdating
	ServeStaleHTTP500
	ServeStaleHTTP502
	ServeStaleHTTP503
	ServeStaleHTTP504
	ServeStaleHTTP403
	ServeStaleHTTP404
	ServeStaleHTTP429
)

func (t ServeStaleErrorType) String() string {
	switch t {
	case ServeStaleError:
		return Error
	case ServeStaleTimeout:
		return Timeout
	case ServeStaleInvalidHeader:
		return InvalidHeader
	case ServeStaleUpdating:
		return Updating
	case ServeStaleHTTP500:
		return HTTP500
	case ServeStaleHTTP502:
		return HTTP502
	case ServeStaleHTTP503:
		return HTTP503
	case ServeStaleHTTP504:
		return HTTP504
	case ServeStaleHTTP403:
		return HTTP403
	case ServeStaleHTTP404:
		return HTTP404
	case ServeStaleHTTP429:
		return HTTP429
	default:
		return Unspecified
	}
}

type NormalizeRequestOptions struct {
	Cookies     NormalizeRequestCookies
	QueryString NormalizeRequestQueryString
}

func setNormalizeRequestOptionsDefaults(optionsPtr *NormalizeRequestOptions) *NormalizeRequestOptions {
	var options NormalizeRequestOptions
	if optionsPtr != nil {
		options = *optionsPtr
	}

	if options.Cookies.Ignore == nil {
		options.Cookies.Ignore = ptr.Bool(true)
	}

	if options.QueryString.Ignore == nil && len(options.QueryString.Whitelist) == 0 && len(options.QueryString.Blacklist) == 0 {
		options.QueryString.Ignore = ptr.Bool(true)
	}

	return &options
}

type NormalizeRequestCookies struct {
	Ignore *bool
}

type NormalizeRequestQueryString struct { // one of
	Ignore    *bool
	Whitelist []string
	Blacklist []string
}

type CompressionOptions struct {
	Variant CompressionVariant
}

func setCompressionOptionsDefaults(optionsPtr *CompressionOptions) *CompressionOptions {
	var options CompressionOptions
	if optionsPtr != nil {
		options = *optionsPtr
	}

	if options.Variant.FetchCompressed == nil && options.Variant.Compress == nil {
		options.Variant.Compress = &Compress{
			Compress: true,
			Codecs:   []CompressCodec{CompressCodecGzip, CompressCodecBrotli},
			Types:    nil,
		}
	}

	return &options
}

type CompressionVariant struct {
	FetchCompressed *bool
	Compress        *Compress
}

type Compress struct {
	Compress bool
	Codecs   []CompressCodec
	Types    []string
}

type CompressCodec int64

const (
	CompressCodecGzip CompressCodec = iota
	CompressCodecBrotli
)

type StaticHeadersOptions struct {
	Request  []HeaderOption
	Response []HeaderOption
}

type HeaderOption struct {
	Name   string
	Action HeaderAction
	Value  string
}

type HeaderAction int64

const (
	HeaderActionSet HeaderAction = iota
	HeaderActionAppend
	HeaderActionRemove
)

type RewriteOptions struct {
	Enabled     *bool
	Regex       *string
	Replacement *string
	Flag        *RewriteFlag
}

type RewriteFlag int64

const (
	RewriteFlagLast RewriteFlag = iota
	RewriteFlagBreak
	RewriteFlagRedirect
	RewriteFlagPermanent
)

type OriginProtocol int64

const (
	OriginProtocolHTTP OriginProtocol = iota
	OriginProtocolHTTPS
	OriginProtocolSame
)

type CreateResourceParams struct {
	FolderID      string
	OriginVariant OriginVariant

	Active             bool
	Name               string
	SecondaryHostnames []string
	OriginProtocol     OriginProtocol

	Options *ResourceOptions
}

type OriginVariant struct {
	GroupID int64
	Source  *OriginVariantSource
}

type OriginVariantSource struct {
	Source string
	Type   OriginType
}

type GetResourceParams struct {
	ResourceID string
}

type GetAllResourceParams struct {
	FolderID *string
	Page     *Pagination
}

type UpdateResourceParams struct {
	ResourceID     string
	OriginsGroupID int64

	Active             bool
	SecondaryHostnames *[]string
	OriginProtocol     *OriginProtocol

	Options *ResourceOptions
}

type DeleteResourceParams struct {
	ResourceID string
}

type ActivateResourceParams struct {
	ResourceID string
	Version    int64
}

type ListResourceVersionsParams struct {
	ResourceID string
	Versions   []int64
}

type CountActiveResourcesParams struct {
	FolderID string
}
