package auth

import "context"

type ctxKey string

func (c ctxKey) String() string {
	return "models context key " + string(c)
}

const (
	ctxAccessID     = ctxKey("AccessId")
	ctxAccessSecret = ctxKey("AccessSecret")
)

// WithAuth adds data to context
func WithAuth(ctx context.Context, id, secret string) context.Context {
	return context.WithValue(context.WithValue(ctx, ctxAccessID, id), ctxAccessSecret, secret)
}

// AuthDataFromContext retrieves auth data from context if any
func AuthDataFromContext(ctx context.Context) (string, string, bool) {
	id, okID := ctx.Value(ctxAccessID).(string)
	secret, okSecret := ctx.Value(ctxAccessSecret).(string)
	return id, secret, okID && okSecret
}
