package helpers

import (
	"context"
	"time"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/federation"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/iam"
	"a.yandex-team.ru/library/go/core/log"
)

type Operation struct {
	ID                 string    `json:"id" yaml:"id"`
	InstanceID         string    `json:"instance_id" yaml:"instance_id"`
	CreatedAt          time.Time `json:"created_at" yaml:"created_at"`
	ModifiedAt         time.Time `json:"modified_at" yaml:"modified_at"`
	StatusMessage      string    `json:"status_message" yaml:"status_message"`
	Status             string    `json:"status" yaml:"status"`
	DeliveryEstimation time.Time `json:"delivery_estimation" yaml:"delivery_estimation"`
}

func NewCMSClient(ctx context.Context, env *cli.Env) instanceclient.InstanceClient {
	cfg := config.FromEnv(env)

	caPath := cfg.CAPath
	if cfg.DeploymentConfig().CAPath != "" {
		caPath = cfg.DeploymentConfig().CAPath
	}
	caPath, err := tilde.Expand(caPath)
	if err != nil {
		env.L().Fatalf("failed to get CA path: %s", err)
	}

	clientCfg := grpc.Config{
		Host: cfg.DeploymentConfig().CMSHost,
		Transport: grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile: caPath,
				},
			},
		},
	}

	iamCreds := federation.NewIamCredentials(env)
	if iamCreds == nil {
		iamCreds = iam.NewIamCredentials(env)
	}
	if iamCreds == nil {
		env.L().Fatal("can not get IAM token", log.Error(err))
	}

	client, err := grpc.NewFromConfig(ctx, clientCfg, "mdb-admin", env.L(), iamCreds)
	if err != nil {
		env.L().Fatal("can not create api", log.Error(err))
	}

	return client
}

func OutputOperation(env *cli.Env, op *api.InstanceOperation) {
	operation := Operation{
		ID:                 op.Id,
		InstanceID:         op.InstanceId,
		CreatedAt:          op.CreatedAt.AsTime(),
		ModifiedAt:         op.ModifiedAt.AsTime(),
		StatusMessage:      op.StatusMessage,
		Status:             op.Status.String(),
		DeliveryEstimation: op.DeliveryEstimation.AsTime(),
	}
	o, err := env.OutMarshaller.Marshal(operation)
	if err != nil {
		env.L().Error("Failed to marshal operation", log.Any("operation", op), log.Error(err))
	}

	env.L().Info(string(o))
}

func CtxWithNewIdempotence(ctx context.Context, env *cli.Env) context.Context {
	idem, err := idempotence.New()
	if err != nil {
		env.L().Fatal("can not create idempotence", log.Error(err))
	}
	newCtx := idempotence.WithOutgoing(ctx, idem)
	return newCtx
}
