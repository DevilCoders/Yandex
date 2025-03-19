package shipments

import (
	"context"
	"strings"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
)

var (
	cmdCreate = initCreate()

	flagFQDNs            []string
	flagCommands         []string
	flagParallel         int64
	flagStopOnErrorCount int64
	flagTimeout          time.Duration
)

const (
	flagNameFQDNs            = "fqdns"
	flagNameCommands         = "cmd"
	flagNameParallel         = "parallel"
	flagNameStopOnErrorCount = "errorcount"
	flagNameTimeout          = "timeout"
)

func initCreate() *cli.Command {
	cmd := &cobra.Command{
		Use:   "create <name>",
		Short: "Create deploy command",
		Long:  "Create deploy command.",
		Args:  cobra.MinimumNArgs(0),
	}

	cmd.Flags().StringSliceVar(
		&flagFQDNs,
		flagNameFQDNs,
		[]string{},
		"FQDNs of minions to run the command on",
	)
	if err := cmd.MarkFlagRequired(flagNameFQDNs); err != nil {
		panic(err)
	}

	cmd.Flags().StringArrayVar(
		&flagCommands,
		flagNameCommands,
		nil,
		"Command consisting of comma-separated list of timeout, command and arguments. "+
			"Multiple arguments are allowed, each describes a separate command. "+
			"Order of arguments determines order of execution. "+
			"Example: --cmd=1m,cmd.run,'uname -a' --cmd=1m,cmd.run,'salt-call test.ping' will "+
			"run 'uname -a' and then 'salt-call test.ping'). "+
			"WARNING: comma in arguments (not as separator) is NOT supported at this time.", // TODO
	)
	if err := cmd.MarkFlagRequired(flagNameCommands); err != nil {
		panic(err)
	}

	cmd.Flags().Int64Var(
		&flagParallel,
		flagNameParallel,
		1,
		"How many commands to run in parallel",
	)

	cmd.Flags().Int64Var(
		&flagStopOnErrorCount,
		flagNameStopOnErrorCount,
		1,
		"Stop after this number of failed commands",
	)

	cmd.Flags().DurationVar(
		&flagTimeout,
		flagNameTimeout,
		0,
		"Timeout for entire shipment (precision is in seconds); if none, sum of command timeouts will be used",
	)

	return &cli.Command{Cmd: cmd, Run: Create}
}

// Create shipment
func Create(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	cmdDefs := make([]models.CommandDef, 0, len(flagCommands))
	for _, cmd := range flagCommands {
		s := strings.Split(cmd, ",")
		if len(s) < 2 {
			env.Logger.Fatalf("Command %q must have at least two arguments (command type and timeout)", cmd)
		}

		d, err := time.ParseDuration(s[0])
		if err != nil {
			env.Logger.Fatalf("Command %q has invalid timeout %q: %s", cmd, s[0], err)
		}

		if s[1] == "" {
			env.Logger.Fatalf("Command %q has empty type", cmd)
		}

		cmdDef := models.CommandDef{
			Type:    s[1],
			Args:    s[2:],
			Timeout: encodingutil.FromDuration(d),
		}

		cmdDefs = append(cmdDefs, cmdDef)
	}

	if flagTimeout == 0 {
		for _, cmd := range cmdDefs {
			flagTimeout += cmd.Timeout.Duration
		}
	}

	shipment, err := dapi.CreateShipment(
		ctx,
		flagFQDNs,
		cmdDefs,
		flagParallel,
		flagStopOnErrorCount,
		flagTimeout,
	)
	if err != nil {
		env.Logger.Fatalf("Failed to create deploy shipment: %s", err)
	}

	g, err := env.OutMarshaller.Marshal(shipment)
	if err != nil {
		env.Logger.Errorf("Failed to marshal deploy shipment '%+v': %s", shipment, err)
	}

	env.Logger.Info(string(g))
}
