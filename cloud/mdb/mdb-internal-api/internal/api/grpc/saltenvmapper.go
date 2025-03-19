package grpc

import (
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

type SaltEnvMapper struct {
	production  int64
	prestable   int64
	unspecified int64
	cfg         logic.SaltEnvsConfig
}

func NewSaltEnvMapper(production, prestable, unspecified int64, cfg logic.SaltEnvsConfig) SaltEnvMapper {
	return SaltEnvMapper{
		production:  production,
		prestable:   prestable,
		unspecified: unspecified,
		cfg:         cfg,
	}
}

func (m SaltEnvMapper) FromGRPC(env int64) (environment.SaltEnv, error) {
	switch env {
	case m.production:
		return m.cfg.Production, nil
	case m.prestable:
		return m.cfg.Prestable, nil
	default:
		return environment.SaltEnvInvalid, semerr.InvalidInputf("unknown environment type %d", env)
	}
}

func (m SaltEnvMapper) IsPrestableFromGRPC(env int64) bool {
	switch env {
	case m.prestable:
		return true
	default:
		return false
	}
}

func (m SaltEnvMapper) ToGRPC(env environment.SaltEnv) int64 {
	switch env {
	case m.cfg.Production:
		return m.production
	case m.cfg.Prestable:
		return m.prestable
	default:
		return m.unspecified
	}
}
