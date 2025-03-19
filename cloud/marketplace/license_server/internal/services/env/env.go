package env

// NOTE: Env concept is deprecated as it doesn't reflect backends usage in
// NOTE: actions and should be replaced by proper designed adapters concepts.

import (
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/uuid"
	"a.yandex-team.ru/library/go/core/log"
)

type Env struct {
	backends         *Backends
	cloudIDGenerator uuid.CloudIDGenerator
	authBackend      auth.AuthBackend

	HandlersLogger log.Logger
}

func (env *Env) Backends() *Backends {
	return env.backends
}

func (env *Env) CloudIDGenerator() uuid.CloudIDGenerator {
	return env.cloudIDGenerator
}

type EnvBuilder struct {
	handlersLogger   log.Logger
	backendsFabric   []BackendsOption
	cloudIDGenerator uuid.CloudIDGenerator
}

func NewEnvBuilder() *EnvBuilder {
	return &EnvBuilder{}
}

func (b *EnvBuilder) Build() *Env {
	return &Env{
		HandlersLogger:   b.handlersLogger,
		backends:         NewBackendsSet(b.backendsFabric...),
		cloudIDGenerator: b.cloudIDGenerator,
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

func (b *EnvBuilder) WithCloudIDGeneratorPrefix(prefix string) *EnvBuilder {
	b.cloudIDGenerator = uuid.NewCloudIDGenerator(prefix)
	return b
}
