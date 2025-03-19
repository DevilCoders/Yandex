package environment

import "a.yandex-team.ru/library/go/core/xerrors"

type SaltEnv string

// TODO: change to int enum like everything else
const (
	SaltEnvInvalid     SaltEnv = "invalid"
	SaltEnvDev         SaltEnv = "dev"
	SaltEnvLoad        SaltEnv = "load"
	SaltEnvQA          SaltEnv = "qa"
	SaltEnvProd        SaltEnv = "prod"
	SaltEnvComputeProd SaltEnv = "compute-prod"
)

var saltEnvMapping = map[SaltEnv]struct{}{
	SaltEnvDev:         {},
	SaltEnvLoad:        {},
	SaltEnvQA:          {},
	SaltEnvProd:        {},
	SaltEnvComputeProd: {},
}

func ParseSaltEnv(str string) (SaltEnv, error) {
	env := SaltEnv(str)
	_, ok := saltEnvMapping[env]
	if !ok {
		return "", xerrors.Errorf("invalid environment type: %s", str)
	}
	return env, nil
}
