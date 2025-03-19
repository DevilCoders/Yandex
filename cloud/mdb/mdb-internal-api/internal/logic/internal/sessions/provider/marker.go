package provider

import "context"

type ctxKey string

var (
	ctxKeySessionMarker = ctxKey("session marker")
)

func withSessionMarker(ctx context.Context) context.Context {
	return context.WithValue(ctx, ctxKeySessionMarker, true)
}

func sessionMarkerFrom(ctx context.Context) bool {
	b := ctx.Value(ctxKeySessionMarker)
	return b != nil
}
