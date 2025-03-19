package iam

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
)

func NewIamCredentials(env *cli.Env) iam.CredentialsService {
	token, err := getToken(env)
	if err != nil {
		env.L().Fatalf("failed get iam token: %s", err)
	}

	return grpc.NewTokenCredentials(func(ctx context.Context) (iam.Token, error) {
		return token, nil
	})
}
