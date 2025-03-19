package shipments

import (
	"context"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy/helpers"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdGet = initGet()
)

const (
	flagNameDeep = "deep"
)

func initGet() *cli.Command {
	cmd := &cobra.Command{
		Use:   "get <id>",
		Short: "Get shipment",
		Long:  "Get shipment.",
		Args:  cobra.MinimumNArgs(1),
	}

	cmd.Flags().Bool(
		flagNameDeep,
		false,
		"Get deep info about this shipment",
	)

	return &cli.Command{Cmd: cmd, Run: Get}
}

// Get shipment
// nolint: gocyclo
func Get(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	dapi := helpers.NewDeployAPI(env)

	id, err := models.ParseShipmentID(args[0])
	if err != nil {
		env.Logger.Fatalf("Invalid shipment id type: %s", err)
	}

	shipment, err := dapi.GetShipment(ctx, id)
	if err != nil {
		env.Logger.Fatalf("Failed to load deploy shipment: %s", err)
	}

	deepShipment := make(map[string]beautyMinion, len(shipment.FQDNs))

	if cmd.Flag(flagNameDeep).Changed {
		cmds := loadCommands(ctx, dapi, id, env.Logger)
		jobs := loadJobs(ctx, dapi, id, env.Logger)
		jobResults := loadJobResults(ctx, dapi, jobs, cmds, env.Logger)

		for _, fqdn := range shipment.FQDNs {
			minionCmds, ok := cmds[fqdn]
			if !ok {
				continue
			}

			env.Logger.Debugf("Found %d commands for fqdn %s", len(cmds), fqdn)

			var bMinion beautyMinion
			for _, cmd := range minionCmds {
				env.Logger.Debugf("Found command %d for fqdn %s", cmd.ID, fqdn)

				bCmd := beautyCommand{
					CommandDef: cmd.CommandDef,
					ID:         cmd.ID,
					Status:     cmd.Status,
					CreatedAt:  cmd.CreatedAt,
					UpdatedAt:  cmd.UpdatedAt,
				}

				job, ok := jobs[cmd.ID]
				if !ok {
					bMinion.Commands = append(bMinion.Commands, bCmd)
					continue
				}

				env.Logger.Debugf("Found job %d for fqdn %s", job.ID, fqdn)

				bCmd.Job = &beautyJob{
					ID:        job.ID,
					ExtID:     job.ExtID,
					Status:    job.Status,
					CreatedAt: job.CreatedAt,
					UpdatedAt: job.UpdatedAt,
				}

				jrs, ok := jobResults[job.ID]
				if !ok {
					bMinion.Commands = append(bMinion.Commands, bCmd)
					continue
				}

				env.Logger.Debugf("Found %d job results for fqdn %s", len(jrs), fqdn)

				for _, jr := range jrs {
					bCmd.Job.Results = append(
						bCmd.Job.Results,
						beautyJobResult{
							ID:         jr.JobResultID,
							Order:      jr.Order,
							Status:     jr.Status,
							RecordedAt: jr.RecordedAt,
						},
					)
				}

				bMinion.Commands = append(bMinion.Commands, bCmd)
			}

			deepShipment[fqdn] = bMinion
		}
	}

	s, err := env.OutMarshaller.Marshal(shipment)
	if err != nil {
		env.Logger.Errorf("Failed to marshal deploy shipment '%+v': %s", shipment, err)
	}

	env.Logger.Info(string(s))

	// We output deep info here because we don't want to pollute output with debug info from retrieving all the deep info
	if cmd.Flag(flagNameDeep).Changed {
		s, err = env.OutMarshaller.Marshal(deepShipment)
		if err != nil {
			env.Logger.Errorf("Failed to marshal beauty '%+v': %s", deepShipment, err)
		}

		env.Logger.Info(string(s))
	}
}

func loadCommands(ctx context.Context, dapi deployapi.Client, shipmentID models.ShipmentID, lg log.Logger) map[string][]models.Command {
	cmdsAttrs := deployapi.SelectCommandsAttrs{}
	cmdsAttrs.ShipmentID.Set(shipmentID.String())
	cmds, _, err := dapi.GetCommands(ctx, cmdsAttrs, deployapi.Paging{Size: 10000})
	if err != nil {
		lg.Errorf("Failed to load commands: %s", err)
		return map[string][]models.Command{}
	}

	cmdsMap := make(map[string][]models.Command, len(cmds))
	for _, cmd := range cmds {
		cmdsMap[cmd.FQDN] = append(cmdsMap[cmd.FQDN], cmd)
	}

	lg.Debugf("Loaded %d commands", len(cmdsMap))
	return cmdsMap
}

func loadJobs(ctx context.Context, dapi deployapi.Client, shipmentID models.ShipmentID, lg log.Logger) map[models.CommandID]models.Job {
	jobsAttrs := deployapi.SelectJobsAttrs{}
	jobsAttrs.ShipmentID.Set(shipmentID.String())
	jobs, _, err := dapi.GetJobs(ctx, jobsAttrs, deployapi.Paging{Size: 10000})
	if err != nil {
		lg.Errorf("Failed to load jobs: %s", err)
		return map[models.CommandID]models.Job{}
	}

	jobsMap := make(map[models.CommandID]models.Job, len(jobs))
	for _, job := range jobs {
		jobsMap[job.CommandID] = job
	}

	lg.Debugf("Loaded %d jobs", len(jobsMap))
	return jobsMap
}

func commandByID(cmdsPerFQDN map[string][]models.Command, cmdID models.CommandID) (models.Command, bool) {
	for _, cmds := range cmdsPerFQDN {
		for _, cmd := range cmds {
			if cmdID == cmd.ID {
				return cmd, true
			}
		}
	}

	return models.Command{}, false
}

func loadJobResults(ctx context.Context, dapi deployapi.Client, jobs map[models.CommandID]models.Job, cmdsPerFQDN map[string][]models.Command, lg log.Logger) map[models.JobID][]models.JobResult {
	jobsResultAttrs := deployapi.SelectJobResultsAttrs{}
	jobResultsMap := make(map[models.JobID][]models.JobResult)

	for _, job := range jobs {
		cmd, ok := commandByID(cmdsPerFQDN, job.CommandID)
		if !ok {
			lg.Errorf("Failed to find command for job %+v", job)
			continue
		}

		jobsResultAttrs.ExtJobID.Set(job.ExtID)
		jobsResultAttrs.FQDN.Set(cmd.FQDN)
		jobResults, _, err := dapi.GetJobResults(ctx, jobsResultAttrs, deployapi.Paging{Size: 10000})
		if err != nil {
			lg.Errorf("Failed to load job results: %s", err)
			continue
		}

		if len(jobResults) > 0 {
			jobResultsMap[job.ID] = jobResults
		}
	}

	lg.Debugf("Loaded %d job results", len(jobResultsMap))
	return jobResultsMap
}

type beautyMinion struct {
	Commands []beautyCommand `json:",omitempty" yaml:",omitempty"`
}

type beautyCommand struct {
	models.CommandDef

	ID        models.CommandID
	Status    models.CommandStatus
	CreatedAt time.Time
	UpdatedAt time.Time

	Job *beautyJob `json:",omitempty" yaml:",omitempty"`
}

type beautyJob struct {
	ID        models.JobID
	ExtID     string
	Status    models.JobStatus
	CreatedAt time.Time
	UpdatedAt time.Time

	Results []beautyJobResult `json:",omitempty" yaml:",omitempty"`
}

type beautyJobResult struct {
	ID         models.JobResultID
	Order      int
	Status     models.JobResultStatus
	RecordedAt time.Time
}
