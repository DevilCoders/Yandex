package auth

import "context"

type ctxKey string

func (c ctxKey) String() string {
	return "models context key " + string(c)
}

const (
	ctxAuthToken = ctxKey("authToken")
)

// WithAuthToken adds auth token to context
func WithAuthToken(ctx context.Context, token string) context.Context {
	return context.WithValue(ctx, ctxAuthToken, token)
}

// TokenFromContext retrieves auth token from context if any
func TokenFromContext(ctx context.Context) (string, bool) {
	token, ok := ctx.Value(ctxAuthToken).(string)
	return token, ok
}
