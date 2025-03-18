package storage

import "net/http"

//go:generate easyjson types.go

type Pagination struct {
	Offset int
	Limit  int
}

//easyjson:json
type StringArray []string

//easyjson:json
type HeaderOptionArray []HeaderOption

const (
	Unspecified = "UNSPECIFIED"
	Common      = "COMMON"
	Bucket      = "BUCKET"
	Website     = "WEBSITE"
)

type OriginType int64

const (
	OriginTypeCommon OriginType = iota
	OriginTypeBucket
	OriginTypeWebsite
)

func (t OriginType) String() string {
	switch t {
	case OriginTypeCommon:
		return Common
	case OriginTypeBucket:
		return Bucket
	case OriginTypeWebsite:
		return Website
	}

	return Unspecified
}

const (
	HTTP  = "HTTP"
	HTTPS = "HTTPS"
	Same  = "SAME"
)

type OriginProtocol int64

const (
	OriginProtocolHTTP OriginProtocol = iota
	OriginProtocolHTTPS
	OriginProtocolSame
)

func (p OriginProtocol) String() string {
	switch p {
	case OriginProtocolHTTP:
		return HTTP
	case OriginProtocolHTTPS:
		return HTTPS
	case OriginProtocolSame:
		return Same
	}

	return Unspecified
}

const (
	Star           = "STAR"
	OriginAny      = "ORIGIN_ANY"
	OriginFromList = "ORIGIN_FROM_LIST"
)

type CORSMode int64

const (
	CORSModeStar CORSMode = iota
	CORSModeOriginAny
	CORSModeOriginFromList
)

func (m *CORSMode) String() string {
	if m == nil {
		return Unspecified
	}

	switch *m {
	case CORSModeStar:
		return Star
	case CORSModeOriginAny:
		return OriginAny
	case CORSModeOriginFromList:
		return OriginFromList
	}

	return Unspecified
}

const (
	Last      = "LAST"
	Break     = "BREAK"
	Redirect  = "REDIRECT"
	Permanent = "PERMANENT"
)

type RewriteFlag int64

const (
	RewriteFlagLast RewriteFlag = iota
	RewriteFlagBreak
	RewriteFlagRedirect
	RewriteFlagPermanent
)

func (f RewriteFlag) String() string {
	switch f {
	case RewriteFlagLast:
		return Last
	case RewriteFlagBreak:
		return Break
	case RewriteFlagRedirect:
		return Redirect
	case RewriteFlagPermanent:
		return Permanent
	}

	return Unspecified
}

const (
	Gzip   = "GZIP"
	Brotli = "BROTLI"
)

type CompressCodec int64

const (
	CompressCodecGzip CompressCodec = iota
	CompressCodecBrotli
)

func (c CompressCodec) String() string {
	switch c {
	case CompressCodecGzip:
		return Gzip
	case CompressCodecBrotli:
		return Brotli
	}

	return Unspecified
}

//easyjson:json
type CompressCodecArray []CompressCodec

//easyjson:json
type OverrideTTLCodeArray []OverrideTTLCode

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

//easyjson:json
type ServeStaleErrorArray []ServeStaleErrorType

const (
	Set    = "SET"
	Append = "APPEND"
	Remove = "REMOVE"
)

type HeaderAction int64

const (
	HeaderActionSet HeaderAction = iota
	HeaderActionAppend
	HeaderActionRemove
)

func (a HeaderAction) String() string {
	switch a {
	case HeaderActionSet:
		return Set
	case HeaderActionAppend:
		return Append
	case HeaderActionRemove:
		return Remove
	default:
		return Unspecified
	}
}

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

//easyjson:json
type AllowedMethodArray []AllowedMethod

type EntityVersion int64
