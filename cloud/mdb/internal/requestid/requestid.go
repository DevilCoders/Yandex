package requestid

import (
	"context"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type ctxKey string

func (c ctxKey) String() string {
	return "context key " + string(c)
}

var (
	ctxKeyRequestID = ctxKey("RequestID")
)

// New returns new random request ID
func New() string {
	// TODO: handle error gracefully
	return uuid.Must(uuid.NewV4()).String()
}

// WithRequestID adds request ID to context
func WithRequestID(ctx context.Context, rid string) context.Context {
	// Do not replace value already stored in ctx
	if ctx.Value(ctxKeyRequestID) != nil {
		return ctx
	}

	return context.WithValue(ctx, ctxKeyRequestID, rid)
}

// CheckedFromContext retrieves request ID from context
func CheckedFromContext(ctx context.Context) (string, bool) {
	rid, ok := ctx.Value(ctxKeyRequestID).(string)
	return rid, ok
}

// MustFromContext retrieves request ID from context or panics if there is none
func MustFromContext(ctx context.Context) string {
	rid, ok := CheckedFromContext(ctx)
	if !ok {
		panic("request id not found in context")
	}

	return rid
}

// FromContextOrNew retrieves request ID from context or generates a new one
func FromContextOrNew(ctx context.Context) string {
	rid, ok := CheckedFromContext(ctx)
	if !ok {
		return New()
	}

	return rid
}

const (
	logFieldName = "request_id"
)

// WithLogField adds request id log field to context
func WithLogField(ctx context.Context, rid string) context.Context {
	return ctxlog.WithFields(ctx, log.String(logFieldName, rid))
}
