package xmiddleware

import "context"

type nameCtxKey uint8

const (
	nameKey nameCtxKey = iota

	Unknown = "Unknown"
)

func SetOperationName(ctx context.Context, name string) context.Context {
	return context.WithValue(ctx, nameKey, name)
}

func GetOperationName(ctx context.Context) string {
	value := ctx.Value(nameKey)
	if name, ok := value.(string); ok {
		return name
	}

	return Unknown
}
