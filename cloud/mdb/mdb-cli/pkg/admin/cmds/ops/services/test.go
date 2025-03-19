package services

import (
	"context"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

var (
	cmdTest = initTest()

	flagTimeout time.Duration
)

const (
	flagNameTimeout = "timeout"
)

func initTest() *cli.Command {
	cmd := &cobra.Command{
		Use:               "test <service-name> <service-name>",
		Short:             "Update service information",
		Long:              "Update service information about salt states, run 'hs test'",
		Args:              cobra.MinimumNArgs(1),
		ValidArgsFunction: compServices,
	}
	cmd.Flags().DurationVar(
		&flagTimeout,
		flagNameTimeout,
		20*time.Minute,
		"Timeout for high state command and entire shipment",
	)
	return &cli.Command{Cmd: cmd, Run: Test}
}

// Test service status and information
func Test(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	environment, ok := topology[cfg.SelectedDeployment]
	if !ok {
		env.L().Fatalf("can't find topology for deployment %q", cfg.SelectedDeployment)
	}

	var targets []string
	for _, sn := range args {
		service, ok := environment[serviceName(sn)]
		if !ok {
			env.L().Fatalf("can't find service with name %q for deployment %q", sn, cfg.SelectedDeployment)
		}
		targets = append(targets, service.Group)
	}

	dapi := helpers.NewDeployAPI(env)
	hosts := helpers.ParseMultiTargets(ctx, targets, env.L())

	shipment, err := dapi.CreateShipment(ctx,
		hosts,
		[]models.CommandDef{
			{
				Type:    "state.highstate",
				Args:    []string{"test=True", "queue=True"},
				Timeout: encodingutil.Duration{Duration: flagTimeout},
			},
		},
		int64(len(hosts)),
		int64(len(hosts)),
		flagTimeout+5*time.Minute, // Minimum timeout for shipment infra.
	)

	if err != nil {
		env.L().Fatalf("create test shipment failed: %s", err)
	}

	shipment, err = waitForShipment(ctx, shipment, dapi)
	if err != nil {
		env.L().Fatalf("wait for test shipment: %s", err)
	}
	env.L().Infof("wait for test shipment completed with status %q", shipment.Status)
	if len(args) == 1 {
		Get(ctx, env, cmd, args)
	} else {
		List(ctx, env, cmd, args)
	}
}
