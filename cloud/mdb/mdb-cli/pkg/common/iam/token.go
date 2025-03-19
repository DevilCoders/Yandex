package iam

import (
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/token"
	"a.yandex-team.ru/library/go/core/log"
)

const iamTokenURL = `https://oauth.yandex-team.ru/authorize?response_type=token&client_id=8cdb2f6a0dca48398c6880312ee2f78d`

func getToken(env *cli.Env) (iam.Token, error) {
	cfg := config.FromEnv(env)
	logger := env.L()

	oldToken := cfg.DeploymentConfig().Token
	err := oldToken.Validate()
	if err == nil {
		return iam.Token{
			Value:     oldToken.IAMToken,
			ExpiresAt: oldToken.ExpiresAt,
		}, nil
	}

	if cfg.IamOauthToken == "" {
		t, err := token.Get(env, token.Description{
			URL:  iamTokenURL,
			Name: "IAM token",
		})
		if err != nil {
			logger.Error("can not get oauth token", log.Error(err))
			return iam.Token{}, err
		}

		cfg.IamOauthToken = t
	}

	tokenService, err := grpc.NewTokenServiceClient(
		env.ShutdownContext(),
		cfg.DeploymentConfig().TokenServiceHost,
		"mdb-admin",
		grpcutil.DefaultClientConfig(),
		&grpcutil.PerRPCCredentialsStatic{},
		logger,
	)
	if err != nil {
		logger.Error("can not create token service client", log.Error(err))
		return iam.Token{}, err
	}

	newToken, err := tokenService.TokenFromOauth(env.ShutdownContext(), cfg.IamOauthToken)
	if err != nil {
		logger.Error("can not get token from oauth", log.Error(err))
		return iam.Token{}, err
	}

	logger.Infof("Save token to deployment %s", cfg.SelectedDeployment)
	deploymentCfg := cfg.DeploymentConfig()
	deploymentCfg.Token = config.DeploymentToken{
		IAMToken:  newToken.Value,
		ExpiresAt: newToken.ExpiresAt,
	}
	cfg.Deployments[cfg.DeploymentName()] = deploymentCfg

	logger.Infof("Write updated config to %s", env.GetConfigPath())
	if err = config.WriteConfig(cfg, env.GetConfigPath()); err != nil {
		logger.Fatalf("failed to write config: %s", err)
	}
	env.RootCmd.Cmd.Printf("Wrote updated config to %s\n", env.GetConfigPath())

	logger.Infof("Token Service successfully finished, token will expire at %s", newToken.ExpiresAt)

	return newToken, nil
}
