package idempotence

import (
	"context"

	"github.com/gofrs/uuid"
)

var (
	ctxKeyOutgoing = ctxKey("outgoing idempotence")
)

// New returns new random idempotence id
func New() (string, error) {
	id, err := uuid.NewV4()
	if err != nil {
		return "", err
	}

	return id.String(), nil
}

// Must returns new random idempotence id, throws panic on error
func Must() string {
	id, err := New()
	if err != nil {
		panic(err)
	}

	return id
}

// WithOutgoing adds client-side idempotence to context
func WithOutgoing(ctx context.Context, id string) context.Context {
	// Do not replace value already stored in ctx
	if ctx.Value(ctxKeyOutgoing) != nil {
		return ctx
	}

	return context.WithValue(ctx, ctxKeyOutgoing, id)
}

// OutgoingFromContext retrieves client-side idempotence from context
func OutgoingFromContext(ctx context.Context) (string, bool) {
	id, ok := ctx.Value(ctxKeyOutgoing).(string)
	return id, ok
}

// MustOutgoingFromContext retrieves client-side idempotence from context or panics if there is none
func MustOutgoingFromContext(ctx context.Context) string {
	id, ok := OutgoingFromContext(ctx)
	if !ok {
		panic("outgoing idempotence not found in context")
	}

	return id
}
