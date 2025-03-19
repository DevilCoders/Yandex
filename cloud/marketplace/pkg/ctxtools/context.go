package ctxtools

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/logging"

	"a.yandex-team.ru/mds/go/nullable"
)

type TagsMapping map[string][]string

type (
	ctxMarker uint8

	ctxStore struct {
		requestID         nullable.String
		originalRequestID nullable.String
		longRequestID     nullable.String

		authToken nullable.String

		userAgent string
		remoteIP  string

		// tags store arbitrary tags within context, could be used for context based logging.
		tags map[string][]string

		logger log.Structured
	}
)

const (
	ctxStoreKey ctxMarker = iota
)

func newCtxStore() *ctxStore {
	return &ctxStore{
		tags: make(TagsMapping),
	}
}

func getCtxStore(ctx context.Context) (*ctxStore, bool) {
	v := ctx.Value(ctxStoreKey)
	if v == nil {
		return newCtxStore(), false
	}

	return v.(*ctxStore), true
}

func withCtxStore(ctx context.Context) (context.Context, *ctxStore) {
	store, ok := getCtxStore(ctx)
	if !ok {
		ctx = context.WithValue(ctx, ctxStoreKey, store)
	}
	return ctx, store
}

func WithRequestID(ctx context.Context, requestID string) context.Context {
	resCtx, store := withCtxStore(ctx)
	store.requestID.SetValid(requestID)
	return resCtx
}

func GetRequestIDOrEmpty(ctx context.Context) string {
	store, _ := getCtxStore(ctx)
	return store.requestID.ValueOrZero()
}

func WithLongRequestID(ctx context.Context, requestID string) context.Context {
	resCtx, store := withCtxStore(ctx)
	store.longRequestID = nullable.StringFrom(requestID)
	return resCtx
}

func GetLongRequestIDOrEmpty(ctx context.Context) string {
	store, _ := getCtxStore(ctx)
	return store.longRequestID.ValueOrZero()
}

func WithOriginalRequestID(ctx context.Context, originalRequestID string) context.Context {
	resCtx, store := withCtxStore(ctx)
	store.originalRequestID = nullable.OptionalStringFrom(originalRequestID)
	return resCtx
}

func GetOriginalRequestIDOrEmpty(ctx context.Context) string {
	store, _ := getCtxStore(ctx)
	return store.originalRequestID.String()
}

func WithRequestData(ctx context.Context, userAgent, remoteIP string) context.Context {
	resCtx, store := withCtxStore(ctx)
	store.userAgent = userAgent
	store.remoteIP = remoteIP
	return resCtx
}

func WithTag(ctx context.Context, key string, values ...string) context.Context {
	resCtx, store := withCtxStore(ctx)
	store.tags[key] = append(store.tags[key], values...)
	return resCtx
}

func WithAuthToken(ctx context.Context, authToken string) context.Context {
	resCtx, store := withCtxStore(ctx)
	store.authToken = nullable.StringFrom(authToken)
	return resCtx
}

func GetAuthToken(ctx context.Context) string {
	store, _ := getCtxStore(ctx)
	return store.authToken.String()
}

func GetUserAgent(ctx context.Context) string {
	store, _ := getCtxStore(ctx)
	return store.userAgent
}

func GetRemoteIP(ctx context.Context) string {
	store, _ := getCtxStore(ctx)
	return store.remoteIP
}

func GetTag(ctx context.Context, key string) []string {
	store, _ := getCtxStore(ctx)
	return store.tags[key]
}

func GetTags(ctx context.Context) TagsMapping {
	store, _ := getCtxStore(ctx)
	return store.tags
}

func Logger(ctx context.Context) log.Structured {
	store, _ := getCtxStore(ctx)
	if store.logger != nil {
		return store.logger
	}

	return logging.Logger()
}

func LoggerWith(ctx context.Context, fields ...log.Field) log.Logger {
	return log.With(Logger(ctx).Logger(), fields...)
}

func WithStructuredLogger(ctx context.Context, logger log.Structured) context.Context {
	resCtx, store := withCtxStore(ctx)
	store.logger = logger

	return resCtx
}
