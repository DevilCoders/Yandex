package federation

import (
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

func NewIamCredentials(env *cli.Env) iam.CredentialsService {
	cfg := config.FromEnv(env)

	if cfg.DeploymentConfig().Federation.ID != "" {
		tokenGetter := NewTokenGetter(&cfg, env.GetConfigPath(), env.RootCmd.Cmd.InOrStdin(), env.Logger)
		return grpc.NewTokenCredentials(tokenGetter)
	}

	return nil
}
