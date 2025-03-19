package tooling

import (
	"context"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
)

type ctxKeyType string

const (
	ctxStoreKey ctxKeyType = "tooling_store_ctx"
)

type ctxStore struct {
	ctxStoreCommon
	ctxStoreFeatures
	ctxStoreDB
	ctxStoreInterconnect
	ctxStoreRetry
	ctxStoreTracing

	ctxStoreTesting
}

type ctxCallback func()

func getStoreFromCtx(ctx context.Context) *ctxStore {
	if value := ctx.Value(ctxStoreKey); value != nil {
		return value.(*ctxStore)
	}
	return nil
}

func getNotEmptyStoreFromCtx(ctx context.Context) *ctxStore {
	if store := getStoreFromCtx(ctx); store != nil {
		return store
	}
	return &ctxStore{}
}

func getOrEmbedStore(ctx context.Context) (context.Context, *ctxStore) {
	if store := getStoreFromCtx(ctx); store != nil {
		return ctx, store
	}
	return embedStore(ctx)
}

func embedStore(ctx context.Context) (context.Context, *ctxStore) {
	store := &ctxStore{}
	return context.WithValue(ctx, ctxStoreKey, store), store
}

func deriveStore(ctx context.Context) (context.Context, *ctxStore) {
	store := &ctxStore{}
	if fromCtx := getStoreFromCtx(ctx); fromCtx != nil {
		*store = *fromCtx
	}
	return context.WithValue(ctx, ctxStoreKey, store), store
}

type ctxStoreCommon struct {
	service         string
	sourceFull      string
	sourceShort     string
	sourcePartition string
	handler         string
	action          string

	requestID string
}

type ctxStoreDB struct {
	dbQueryName      string
	dbQueryRowsCount int
	dbQueryCallTime  time.Time
}

type ctxStoreInterconnect struct {
	icSystem          string
	icRequest         string
	icRequestCallTime time.Time
}

type ctxStoreRetry struct {
	retryStarted bool
	retryAttempt int
}

type ctxStoreTracing struct {
	span opentracing.Span
}

type ctxStoreFeatures struct {
	flags        features.Flags
	flagsUpdated bool
}

var realClock = clockwork.NewRealClock()

type ctxStoreTesting struct {
	clockOverride clockwork.Clock
}

func (s *ctxStoreTesting) getClock() clockwork.Clock {
	if s.clockOverride != nil {
		return s.clockOverride
	}
	return realClock
}
