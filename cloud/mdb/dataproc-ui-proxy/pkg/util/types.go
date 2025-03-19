package util

import (
	"context"
	"net/http"
)

type HTTPMiddleware = func(handler http.Handler) http.Handler
type ctxKey string

const (
	clusterContextKey = ctxKey("clusterID")
)

func WithClusterID(ctx context.Context, clusterID string) context.Context {
	if ctx.Value(clusterContextKey) != nil {
		return ctx
	}

	return context.WithValue(ctx, clusterContextKey, clusterID)
}

func ClusterIDFromContext(ctx context.Context) string {
	if clusterID, ok := ctx.Value(clusterContextKey).(string); ok {
		return clusterID
	}

	return ""
}
