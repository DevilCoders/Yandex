package services

import (
	"context"
	"time"

	"github.com/olekukonko/tablewriter"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
)

var (
	cmdList = initList()
)

func initList() *cli.Command {
	cmd := &cobra.Command{
		Use:   "list",
		Short: "Get service information",
		Long:  "Get service information about hosts and last jobs.",
		Args:  cobra.ExactArgs(0),
	}
	return &cli.Command{Cmd: cmd, Run: List}
}

// List services
func List(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	dapi := helpers.NewDeployAPI(env)
	environment, ok := topology[cfg.SelectedDeployment]
	if !ok {
		env.L().Fatalf("can't find topology for deployment %q", cfg.SelectedDeployment)
	}
	table := tablewriter.NewWriter(cmd.OutOrStdout())
	table.SetHeader([]string{"Name", "Group", "Host", "Last Job", "Recorded"})

	for name, service := range environment {
		hosts := helpers.ParseMultiTargets(ctx, []string{service.Group}, env.L())
		for _, host := range hosts {
			results, _, err := dapi.GetJobResults(ctx, deployapi.SelectJobResultsAttrs{FQDN: optional.NewString(host)}, deployapi.Paging{Size: 1, SortOrder: models.SortOrderDesc})
			if err != nil {
				env.L().Fatalf("load job result for hosts: %s", err)
			}

			if len(results) == 0 {
				data := []string{string(name), service.Group, host, "", ""}
				colors := []tablewriter.Colors{{}, {}, {}}
				table.Rich(data, colors)
				continue
			}

			result := results[0]

			rr, errs := saltapi.ParseReturnResult(result.Result, time.Hour*3)
			if errs != nil {
				env.L().Warnf("parse job result: %s", errs)
			}

			extID := result.ExtID
			if rr.IsTestRun() {
				extID += "(t)"
			}

			data := []string{string(name), service.Group, host, extID, result.RecordedAt.Format(time.Stamp)}
			colors := []tablewriter.Colors{{}, {}, {}}
			if result.Status == models.JobResultStatusSuccess {
				colors = append(colors, tablewriter.Colors{tablewriter.FgGreenColor})
			} else if result.Status == models.JobResultStatusFailure {
				colors = append(colors, tablewriter.Colors{tablewriter.FgRedColor})
			} else {
				colors = append(colors, tablewriter.Colors{tablewriter.FgYellowColor})
			}
			if result.RecordedAt.Before(time.Now().Add(-10 * time.Minute)) {
				colors = append(colors, tablewriter.Colors{tablewriter.FgRedColor})
			} else {
				colors = append(colors, tablewriter.Colors{tablewriter.FgGreenColor})
			}
			table.Rich(data, colors)
		}
	}
	table.SetAutoMergeCells(true)
	table.SetRowLine(true)
	table.Render()
}
