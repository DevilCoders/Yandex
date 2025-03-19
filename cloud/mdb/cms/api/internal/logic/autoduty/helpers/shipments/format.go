package shipments

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type FormattedShipment map[string]BeautyMinion

type BeautyMinion struct {
	Commands []BeautyCommand `json:",omitempty" yaml:",omitempty"`
}

func GetOutputFromSimpleShipmentInfo(fs FormattedShipment, fqdn string) ([]json.RawMessage, error) {
	var res []json.RawMessage
	m, ok := fs[fqdn]
	if !ok {
		return nil, xerrors.Errorf("no %s in shipment", fqdn)
	}
	if len(m.Commands) == 0 {
		return nil, xerrors.Errorf("got 0 commands")
	}
	for _, cmd := range m.Commands {
		found := false
		for _, jr := range cmd.Job.Results {
			if jr.Status == models.JobResultStatusSuccess {
				res = append(res, jr.Output)
				found = true
				break
			}
		}
		if !found {
			return nil, xerrors.Errorf(
				"no successfull result in %d results for cmd %q",
				len(cmd.Job.Results),
				cmd.CommandDef.Type,
			)
		}
	}

	return res, nil
}

func GetExtendedShipmentInfo(ctx context.Context, dapi deployapi.Client, shipment models.Shipment) (FormattedShipment, error) {
	deepShipment := make(FormattedShipment, len(shipment.FQDNs))
	cmds := loadCommands(ctx, dapi, shipment.ID)
	jobs, err := loadJobs(ctx, dapi, shipment.ID)
	if err != nil {
		return nil, err
	}
	jobResults := loadJobResults(ctx, dapi, jobs, cmds)
	for _, fqdn := range shipment.FQDNs {
		minionCmds, ok := cmds[fqdn]
		if !ok {
			continue
		}
		var bMinion BeautyMinion
		for _, cmd := range minionCmds {

			bCmd := BeautyCommand{
				CommandDef: cmd.CommandDef,
				ID:         cmd.ID,
				Status:     cmd.Status,
			}

			job, ok := jobs[cmd.ID]
			if !ok {
				bMinion.Commands = append(bMinion.Commands, bCmd)
				continue
			}

			bCmd.Job = &BeautyJob{
				ID:     job.ID,
				Status: job.Status,
			}

			jrs, ok := jobResults[job.ID]
			if !ok {
				bMinion.Commands = append(bMinion.Commands, bCmd)
				continue
			}

			for _, jr := range jrs {
				rr, errs := saltapi.ParseReturnResult(jr.Result, time.Hour*3) // we parse in Msk
				if len(errs) > 0 {
					// return any
					return deepShipment, errs[0]
				}
				bCmd.Job.Results = append(
					bCmd.Job.Results,
					BeautyJobResult{
						ID:     jr.JobResultID,
						Status: jr.Status,
						Output: rr.ReturnRaw,
					},
				)
			}

			bMinion.Commands = append(bMinion.Commands, bCmd)
		}

		deepShipment[fqdn] = bMinion
	}
	return deepShipment, nil
}

func FormatShipment(deepShipment FormattedShipment, shipment models.Shipment) (string, error) {
	marshaller := pretty.YAMLMarshaller{}
	repr, err := marshaller.Marshal(deepShipment)
	if err != nil {
		return "", err
	}
	return fmt.Sprintf("%s shipment id=%s\n%s", shipment.Status, shipment.ID, repr), nil
}

func loadJobs(ctx context.Context, dapi deployapi.Client, shipmentID models.ShipmentID) (map[models.CommandID]models.Job, error) {
	jobsAttrs := deployapi.SelectJobsAttrs{}
	jobsAttrs.ShipmentID.Set(shipmentID.String())
	jobs, _, err := dapi.GetJobs(ctx, jobsAttrs, deployapi.Paging{Size: 10000})
	if err != nil {
		return nil, err
	}

	jobsMap := make(map[models.CommandID]models.Job, len(jobs))
	for _, job := range jobs {
		jobsMap[job.CommandID] = job
	}

	return jobsMap, nil
}

func loadCommands(ctx context.Context, dapi deployapi.Client, shipmentID models.ShipmentID) map[string][]models.Command {
	cmdsAttrs := deployapi.SelectCommandsAttrs{}
	cmdsAttrs.ShipmentID.Set(shipmentID.String())
	cmds, _, err := dapi.GetCommands(ctx, cmdsAttrs, deployapi.Paging{Size: 10000})
	if err != nil {
		return map[string][]models.Command{}
	}

	cmdsMap := make(map[string][]models.Command, len(cmds))
	for _, cmd := range cmds {
		cmdsMap[cmd.FQDN] = append(cmdsMap[cmd.FQDN], cmd)
	}

	return cmdsMap
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

func loadJobResults(ctx context.Context, dapi deployapi.Client, jobs map[models.CommandID]models.Job, cmdsPerFQDN map[string][]models.Command) map[models.JobID][]models.JobResult {
	jobsResultAttrs := deployapi.SelectJobResultsAttrs{}
	jobResultsMap := make(map[models.JobID][]models.JobResult)

	for _, job := range jobs {
		cmd, ok := commandByID(cmdsPerFQDN, job.CommandID)
		if !ok {
			continue
		}

		jobsResultAttrs.ExtJobID.Set(job.ExtID)
		jobsResultAttrs.FQDN.Set(cmd.FQDN)
		jobResults, _, err := dapi.GetJobResults(ctx, jobsResultAttrs, deployapi.Paging{Size: 10000})
		if err != nil {
			continue
		}

		if len(jobResults) > 0 {
			jobResultsMap[job.ID] = jobResults
		}
	}

	return jobResultsMap
}

type BeautyCommand struct {
	models.CommandDef

	ID     models.CommandID
	Status models.CommandStatus

	Job *BeautyJob `json:",omitempty" yaml:",omitempty"`
}

type BeautyJob struct {
	ID     models.JobID
	Status models.JobStatus

	Results []BeautyJobResult `json:",omitempty" yaml:",omitempty"`
}

type BeautyJobResult struct {
	ID     models.JobResultID
	Status models.JobResultStatus
	Output json.RawMessage
}
