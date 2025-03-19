package abc

import (
	"context"
	"fmt"
	"strings"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/federation"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/iam"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	// Cmd ...
	Cmd *cli.Command
)

func init() {
	cmd := &cobra.Command{
		Use:   "abc <abc-slug>",
		Short: "Resolve Cloud for specified abc project",
		Long:  "Resolve Cloud for specified abc project",
		Args:  cobra.ExactArgs(1),
	}
	Cmd = &cli.Command{Cmd: cmd, Run: Resolve}
}

// Resolve Cloud for specified ABC project
func Resolve(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	abcSlug := strings.ToLower(args[0])

	caPath := cfg.CAPath
	if cfg.DeploymentConfig().CAPath != "" {
		caPath = cfg.DeploymentConfig().CAPath
	}
	caPath, err := tilde.Expand(caPath)
	if err != nil {
		env.L().Fatalf("failed to get CA path: %s", err)
	}

	grpcConfig := grpcutil.DefaultClientConfig()
	grpcConfig.Security.TLS.CAFile = caPath

	iamCreds := federation.NewIamCredentials(env)
	if iamCreds == nil {
		iamCreds = iam.NewIamCredentials(env)
	}
	if iamCreds == nil {
		env.L().Fatal("can not get IAM token", log.Error(err))
	}

	abcClient, err := iamgrpc.NewAbcServiceClient(
		ctx,
		"ti.cloud.yandex-team.ru:443",
		"mdb-cli",
		grpcConfig,
		iamCreds,
		env.L(),
	)
	if err != nil {
		env.L().Fatalf("new abc client: %s", err)
	}

	abc, err := abcClient.ResolveByABCSlug(ctx, abcSlug)
	if err != nil {
		env.L().Fatalf("resolve ABC: %s", err)
	}

	pretty, err := env.OutMarshaller.Marshal(abc)
	if err != nil {
		env.L().Fatalf("Failed to marshal abc %s", err)
	}
	fmt.Print(string(pretty))
}
