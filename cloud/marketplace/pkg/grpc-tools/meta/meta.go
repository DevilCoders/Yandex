package meta

import (
	"context"
	"net/textproto"
	"strings"

	"google.golang.org/grpc/metadata"
)

var asHdr = textproto.CanonicalMIMEHeaderKey

var (
	headerAuthorization = asHdr("authorization")

	headerNameOriginalRequestID = asHdr("X-Request-ID")
	headerNameForwardedAgent    = asHdr("X-Forwarded-Agent")
	headerNameForwardedFor      = asHdr("X-Forwarded-For")
)

type metaCtx struct {
	context.Context
}

func NewMeta(c context.Context) metaCtx {
	return metaCtx{c}
}

func (m metaCtx) AuthorizationToken() string {
	const tokenPrefix = "bearer "

	rawToken := m.header(headerAuthorization)

	if !strings.HasPrefix(strings.ToLower(rawToken), tokenPrefix) {
		return ""
	}

	return strings.TrimLeft(rawToken[len(tokenPrefix):], " ")
}

func (m metaCtx) UserAgent() string {
	return m.header(headerNameForwardedAgent)
}

func (m metaCtx) RemoteIP() string {
	return m.header(headerNameForwardedFor)
}

func (m metaCtx) OriginalRequestID() string {
	return m.header(headerNameOriginalRequestID)
}

func (m metaCtx) header(hdrName string) string {
	if md, ok := metadata.FromIncomingContext(m); ok {
		if v := md.Get(hdrName); len(v) > 0 {
			return v[0]
		}
	}

	return ""
}
