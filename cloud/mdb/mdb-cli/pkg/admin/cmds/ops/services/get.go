package services

import (
	"context"
	"fmt"
	"strconv"
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
	cmdGet = initGet()
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:               "get <service-name>",
		Short:             "Get service information",
		Long:              "Get service information about hosts and last jobs.",
		Args:              cobra.ExactArgs(1),
		ValidArgsFunction: compServices,
	}
	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get service status and information
func Get(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	cfg := config.FromEnv(env)
	environment, ok := topology[cfg.SelectedDeployment]
	if !ok {
		env.L().Fatalf("can't find topology for deployment %q", cfg.SelectedDeployment)
	}

	serviceName := serviceName(args[0])
	service, ok := environment[serviceName]
	if !ok {
		env.L().Fatalf("can't find service with name %q for deployment %q", serviceName, cfg.SelectedDeployment)
	}

	hosts := helpers.ParseMultiTargets(ctx, []string{service.Group}, env.L())
	dapi := helpers.NewDeployAPI(env)
	table := tablewriter.NewWriter(cmd.OutOrStdout())
	table.SetHeader([]string{"FQDN", "Func", "Status", "Changes", "JID", "Finished"})

	var result models.JobResult
	var changes []*saltapi.StateReturn
	for _, h := range hosts {
		_, err := dapi.GetMinion(ctx, h)
		if err != nil {
			env.L().Fatalf("get minion %q in deployment %q: %s", h, cfg.SelectedDeployment, err)
		}

		results, _, err := dapi.GetJobResults(ctx, deployapi.SelectJobResultsAttrs{FQDN: optional.NewString(h)}, deployapi.Paging{Size: 1, SortOrder: models.SortOrderDesc})
		if err != nil {
			env.L().Fatalf("load job result for hosts: %s", err)
		}
		if len(results) == 0 {
			break
		}
		result = results[0]
		rr, errs := saltapi.ParseReturnResult(result.Result, time.Hour*3)
		if len(errs) != 0 {
			env.L().Warnf("parse job result result for hosts: %s", errs)
		}
		// It's normal the changes is rewrite, because after this loop we use it to print a last job result.
		changes = make([]*saltapi.StateReturn, 0)
		if rr.ParseLevel == saltapi.ParseLevelFull {
			for _, state := range rr.ReturnStates {
				if rr.IsTestRun() {
					if !state.Result {
						changes = append(changes, state)
					}
				} else {
					if state.Result && string(state.Changes) != "{}" {
						changes = append(changes, state)
					}
				}
			}
		}

		rrFunc := rr.Func
		if rr.IsTestRun() {
			rrFunc += "(t)"
		}
		data := []string{
			h,
			rrFunc,
			string(result.Status),
			strconv.Itoa(len(changes)),
			rr.JID,
			rr.FinishTS.Local().Format(time.Stamp),
		}

		var colors []tablewriter.Colors

		colors = append(colors, tablewriter.Colors{}, tablewriter.Colors{})

		if result.Status == models.JobResultStatusSuccess {
			colors = append(colors, tablewriter.Colors{tablewriter.FgGreenColor})
		} else {
			colors = append(colors, tablewriter.Colors{tablewriter.FgRedColor})
		}

		colors = append(colors, tablewriter.Colors{}, tablewriter.Colors{})

		if rr.FinishTS.Before(time.Now().Add(-10 * time.Minute)) {
			colors = append(colors, tablewriter.Colors{tablewriter.FgRedColor})
		} else {
			colors = append(colors, tablewriter.Colors{tablewriter.FgGreenColor})
		}

		table.Rich(data, colors)
	}
	table.SetCaption(true, "Last job results")
	table.Render()

	if len(changes) == 0 {
		return
	}
	changesTable := tablewriter.NewWriter(cmd.OutOrStdout())
	changesTable.SetHeader([]string{"ID", "Comment", "Changes"})
	for _, change := range changes {
		changesTable.Append([]string{change.ID, string(change.Comment), string(change.Changes)})
	}
	changesTable.SetRowLine(true)
	changesTable.SetCaption(true, fmt.Sprintf("Changes for Job %q", result.ExtID))
	changesTable.Render()
}
