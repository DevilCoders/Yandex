package idempotence

import "context"

var (
	ctxKeyIncoming = ctxKey("incoming idempotence")
)

// Incoming idempotence
type Incoming struct {
	ID   string
	Hash []byte
}

// WithServerSide adds server-side idempotence to context
func WithIncoming(ctx context.Context, idemp Incoming) context.Context {
	// Do not replace value already stored in ctx
	if ctx.Value(ctxKeyIncoming) != nil {
		return ctx
	}

	return context.WithValue(ctx, ctxKeyIncoming, idemp)
}

// IncomingFromContext retrieves server-side idempotence from context
func IncomingFromContext(ctx context.Context) (Incoming, bool) {
	idemp, ok := ctx.Value(ctxKeyIncoming).(Incoming)
	return idemp, ok
}

// MustIncomingFromContext retrieves server-side idempotence from context or panics if there is none
func MustIncomingFromContext(ctx context.Context) Incoming {
	idemp, ok := IncomingFromContext(ctx)
	if !ok {
		panic("incoming idempotence not found in context")
	}

	return idemp
}
