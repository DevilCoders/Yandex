package imports

import (
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
)

type Option func(*formatter)

// WithOptimizeImports enabled fast imports optimization (deletion unused imports and so on).
// NB! You must provide imports resolver (WithImportsResolver) to use this.
func WithOptimizeImports(optimize bool) Option {
	return func(fmt *formatter) {
		fmt.removeUnused = optimize
	}
}

// WithImportsResolver setups imports resolver to use during imports optimization.
func WithImportsResolver(resolver resolver.Resolver) Option {
	return func(fmt *formatter) {
		fmt.resolver = resolver
	}
}

// WithLocalPrefix specifies 'local' import prefix for grouping.
func WithLocalPrefix(prefix string) Option {
	return func(fmt *formatter) {
		fmt.localPrefix = prefix
	}
}

// WithGenerated enables or disables processing of the generated code.
func WithGenerated(process bool) Option {
	return func(fmt *formatter) {
		fmt.processGen = process
	}
}
