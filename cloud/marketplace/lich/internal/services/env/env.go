package env

// NOTE: Env concept is deprecated as it doesn't reflect backends usage in
// NOTE: actions and should be replaced by proper designed adapters concepts.

import (
	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/metrics"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
)

type Env struct {
	backends *Backends

	authBackend auth.AuthBackend

	HandlersLogger log.Logger

	metricsHub *metrics.Hub
}

func (env *Env) Auth() auth.AuthBackend {
	return env.authBackend
}

func (env *Env) Backends() *Backends {
	return env.backends
}

func (env *Env) Metrics() *metrics.Hub {
	return env.metricsHub
}

type EnvBuilder struct {
	handlersLogger log.Logger
	backendsFabric []BackendsOption

	authBackend auth.AuthBackend

	metricsHub *metrics.Hub
}

func NewEnvBuilder() *EnvBuilder {
	return &EnvBuilder{}
}

func (b *EnvBuilder) Build() *Env {
	return &Env{
		HandlersLogger: b.handlersLogger,
		backends:       NewBackendsSet(b.backendsFabric...),

		authBackend: b.authBackend,

		metricsHub: b.metricsHub,
	}
}

func (b *EnvBuilder) WithHandlersLogger(logger log.Logger) *EnvBuilder {
	b.handlersLogger = logger
	return b
}

func (b *EnvBuilder) WithBackendsFabrics(factory ...BackendsOption) *EnvBuilder {
	b.backendsFabric = append(b.backendsFabric, factory...)
	return b
}

func (b *EnvBuilder) WithAuthBackend(authBackend auth.AuthBackend) *EnvBuilder {
	b.authBackend = authBackend
	return b
}

func (b *EnvBuilder) WithMetricsHub(metricsHub *metrics.Hub) *EnvBuilder {
	b.metricsHub = metricsHub
	return b
}
