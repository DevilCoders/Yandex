package jobresults

import (
	"context"
	"strings"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
)

var (
	cmdGet = initGet()
)

const (
	flagNameFailed                 = "failed"
	flagNameAsIs                   = "asis"
	flagNameExcludeRequisiteFailed = "exclude-requisite-failed"
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:   "get <name>",
		Short: "Get job result",
		Long:  "Get job result.",
		Args:  cobra.MinimumNArgs(1),
	}

	cmd.Flags().Bool(
		flagNameFailed,
		false,
		"Show only failed states",
	)

	cmd.Flags().Bool(
		flagNameAsIs,
		false,
		"Show job result as is, without any beautifying or other handling",
	)

	cmd.Flags().Bool(
		flagNameExcludeRequisiteFailed,
		false,
		"Exclude state with One or more requisite fails. Works only with --failed flag",
	)

	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get job result
func Get(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	id, err := models.ParseJobResultID(args[0])
	if err != nil {
		env.App.L().Fatalf("Invalid job result id type: %s", err)
	}

	jr, err := dapi.GetJobResult(ctx, id)
	if err != nil {
		env.App.L().Fatalf("Failed to load job result: %s", err)
	}

	if cmd.Flag(flagNameAsIs).Changed {
		var g []byte
		g, err = env.OutMarshaller.Marshal(jr)
		if err != nil {
			env.App.L().Errorf("Failed to marshal job result '%+v': %s", jr, err)
		}

		env.App.L().Info(string(g))
		return
	}

	res, errs := saltapi.ParseReturnResult(jr.Result, time.Hour*3) // Always parse in Moscow TZ
	if len(errs) != 0 {
		env.Logger.Fatalf("Failed to parse job result value (use %q flag to see full output): %s", flagNameAsIs, errs)
	}

	// Remove successful states
	if cmd.Flag(flagNameFailed).Changed {
		for k, v := range res.ReturnStates {
			if v.Result {
				delete(res.ReturnStates, k)
			}
		}
		if cmd.Flag(flagNameExcludeRequisiteFailed).Changed {
			for k, v := range res.ReturnStates {
				if strings.HasPrefix(string(v.Comment), "One or more requisite failed") {
					delete(res.ReturnStates, k)
				}
			}
		}
	}

	bjr := beautify(jr, res)
	g, err := env.OutMarshaller.Marshal(bjr)
	if err != nil {
		env.App.L().Errorf("Failed to marshal job result '%+v': %s", bjr, err)
	}

	env.App.L().Info(string(g))
}

func beautify(jr models.JobResult, res saltapi.ReturnResult) beautyJobResult {
	return beautyJobResult{
		ID:         jr.JobResultID,
		ExtJobID:   jr.ExtID,
		FQDN:       jr.FQDN,
		Order:      jr.Order,
		Status:     jr.Status,
		RecordedAt: jr.RecordedAt,
		Result: beautyResultValue{
			JID:          res.JID,
			FQDN:         res.FQDN,
			Func:         res.Func,
			FuncArgs:     res.FuncArgs,
			StartTS:      res.StartTS,
			FinishTS:     res.FinishTS,
			ReturnCode:   res.ReturnCode,
			Success:      res.Success,
			ReturnStates: res.ReturnStates,
			ReturnValue:  res.ReturnValue,
		},
	}
}

type beautyJobResult struct {
	ID         models.JobResultID
	ExtJobID   string
	FQDN       string
	Order      int
	Status     models.JobResultStatus
	Result     beautyResultValue
	RecordedAt time.Time
}

type beautyResultValue struct {
	JID          string
	FQDN         string
	Func         string
	FuncArgs     []string
	StartTS      time.Time
	FinishTS     time.Time
	ReturnCode   int
	Success      bool
	ReturnStates interface{}
	ReturnValue  interface{}
}
