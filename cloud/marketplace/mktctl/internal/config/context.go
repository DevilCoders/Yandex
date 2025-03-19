package config

import "context"

type key int

var profileKey key

func FromContext(ctx context.Context) *Config {
	if ctx == nil {
		return nil
	}

	v := ctx.Value(profileKey)
	if v == nil {
		return nil
	}

	return v.(*Config)
}

func NewContext(ctx context.Context, c *Config) context.Context {
	return context.WithValue(ctx, profileKey, c)
}
