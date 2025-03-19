package jobresults

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/paging"
)

var (
	cmdList = initList()

	flagJobID           string
	flagFQDN            string
	flagJobResultStatus string
)

const (
	flagNameFQDN            = "fqdn"
	flagNameJobID           = "jobid"
	flagNameJobResultStatus = "status"
	//flagNameAsIs            = "asis"
)

func initList() *cli.Command {
	cmd := &cobra.Command{
		Use:   "list",
		Short: "List job results",
		Long:  "List all job results.",
		Args:  cobra.MinimumNArgs(0),
	}

	cmd.Flags().StringVar(
		&flagFQDN,
		flagNameFQDN,
		"",
		"Minion fqdn",
	)

	cmd.Flags().StringVar(
		&flagJobID,
		flagNameJobID,
		"",
		"Salt job id",
	)

	cmd.Flags().StringVar(
		&flagJobResultStatus,
		flagNameJobResultStatus,
		"",
		fmt.Sprintf(
			"Job result status (valid values are '%s')",
			strings.ToLower(strings.Join(models.JobResultStatusList(), ",")),
		),
	)

	cmd.Flags().Bool(
		flagNameAsIs,
		false,
		"Show job result as is, without any beautifying or other handling",
	)

	paging.Register(cmd.Flags())
	return &cli.Command{Cmd: cmd, Run: List}
}

// List job results
func List(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	attrs := deployapi.SelectJobResultsAttrs{}
	if cmd.Flag(flagNameJobID).Changed {
		attrs.ExtJobID.Set(flagJobID)
	}
	if cmd.Flag(flagNameFQDN).Changed {
		attrs.FQDN.Set(flagFQDN)
	}
	if cmd.Flag(flagNameJobResultStatus).Changed {
		jrs := models.ParseJobResultStatus(flagJobResultStatus)
		if jrs == models.JobResultStatusUnknown {
			env.App.L().Fatalf("invalid job result status: %s", flagJobResultStatus)
		}

		attrs.Status.Set(string(jrs))
	}

	jobresults, page, err := dapi.GetJobResults(ctx, attrs, paging.Paging())
	if err != nil {
		env.App.L().Fatalf("Failed to load job results: %s", err)
	}

	if cmd.Flag(flagNameAsIs).Changed {
		for _, jr := range jobresults {
			s, err := env.OutMarshaller.Marshal(jr)
			if err != nil {
				env.App.L().Errorf("Failed to marshal job result '%+v': %s", jr, err)
			}
			env.App.L().Info(string(s))
		}
	} else {
		for _, jr := range jobresults {
			res, errs := saltapi.ParseReturnResult(jr.Result, time.Hour*3) // Always parse in Moscow TZ
			if len(errs) != 0 {
				// fallback to 'as is' format, most likely it is not 'hs' results
				s, err := env.OutMarshaller.Marshal(jr)
				if err != nil {
					env.App.L().Errorf("Failed to marshal job result '%+v': %s", jr, err)
				}
				env.App.L().Info(string(s))
				continue
			}

			bjr := beautify(jr, res)
			s, err := env.OutMarshaller.Marshal(bjr)
			if err != nil {
				env.App.L().Errorf("Failed to marshal job result '%+v': %s", jr, err)
			}

			env.App.L().Info(string(s))
		}
	}

	p, err := env.OutMarshaller.Marshal(page)
	if err != nil {
		env.App.L().Fatalf("Failed to marshal paging '%+v': %s", page, err)
	}

	env.App.L().Info(string(p))
}
