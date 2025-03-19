package scope

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"
)

type scopeKeyType int

const scopeKey scopeKeyType = iota

var (
	globalScopeInstance baseScope
)

type baseScope interface {
	Logger() log.Logger
}

type handlerScope interface {
	baseScope
	RequestID() string
}

func getCurrent(ctx context.Context) baseScope {
	if scope, ok := ctx.Value(scopeKey).(baseScope); ok {
		return scope
	}

	if globalScopeInstance != nil {
		return globalScopeInstance
	}

	panic("global scope must be initialized")
}

func newScopeContext(scope baseScope, ctx context.Context) context.Context {
	return context.WithValue(ctx, scopeKey, scope)
}
