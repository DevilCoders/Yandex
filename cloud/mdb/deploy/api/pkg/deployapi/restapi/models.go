package restapi

import (
	"encoding/json"
	"time"

	swagmodels "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/api/swagapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

// TODO: unify conversions with server-side?

func groupFromREST(gr *swagmodels.GroupResp) models.Group {
	return models.Group{
		ID:   models.GroupID(gr.ID),
		Name: gr.Name,
	}
}

func masterFromREST(mr *swagmodels.MasterResp) models.Master {
	m := models.Master{
		FQDN:         mr.Fqdn,
		Aliases:      mr.Aliases,
		AliveCheckAt: time.Unix(mr.AliveCheckAt, 0),
		Alive:        mr.IsAlive,
		CreatedAt:    time.Unix(mr.CreatedAt, 0),
	}

	if mr.Group != nil {
		m.Group = *mr.Group
	}
	if mr.IsOpen != nil {
		m.IsOpen = *mr.IsOpen
	}
	if mr.Description != nil {
		m.Description = *mr.Description
	}

	return m
}

func minionFromREST(mr *swagmodels.MinionResp) models.Minion {
	m := models.Minion{
		FQDN:          mr.Fqdn,
		CreatedAt:     time.Unix(mr.CreatedAt, 0),
		UpdatedAt:     time.Unix(mr.UpdatedAt, 0),
		RegisterUntil: time.Unix(mr.RegisterUntil, 0),
		PublicKey:     mr.PublicKey,
		Registered:    mr.Registered,
		Deleted:       mr.Deleted,
	}

	if mr.Group != nil {
		m.Group = *mr.Group
	}
	if mr.AutoReassign != nil {
		m.AutoReassign = *mr.AutoReassign
	}
	if mr.Master != nil {
		m.MasterFQDN = *mr.Master
	}

	return m
}

func shipmentFromREST(mr *swagmodels.ShipmentResp) (models.Shipment, error) {
	id, err := models.ParseShipmentID(mr.ID)
	if err != nil {
		return models.Shipment{}, err
	}

	return models.Shipment{
		ID:               id,
		Commands:         commandsDefFromREST(mr.Commands),
		FQDNs:            mr.Fqdns,
		Status:           swagapi.ShipmentStatusFromREST(mr.Status),
		Parallel:         mr.Parallel,
		StopOnErrorCount: mr.StopOnErrorCount,
		OtherCount:       mr.OtherCount,
		DoneCount:        mr.DoneCount,
		ErrorsCount:      mr.ErrorsCount,
		TotalCount:       mr.TotalCount,
		CreatedAt:        time.Unix(mr.CreatedAt, 0),
		UpdatedAt:        time.Unix(mr.UpdatedAt, 0),
		Timeout:          encodingutil.FromDuration(time.Second * time.Duration(mr.Timeout)),
	}, nil
}

func commandsDefFromREST(cmds []*swagmodels.CommandDef) []models.CommandDef {
	var out []models.CommandDef
	for _, c := range cmds {
		out = append(out, commandDefFromREST(*c))
	}
	return out
}

func commandDefFromREST(c swagmodels.CommandDef) models.CommandDef {
	return models.CommandDef{Type: c.Type, Args: c.Arguments, Timeout: encodingutil.FromDuration(time.Second * time.Duration(c.Timeout))}
}

func commandsDefToREST(cmds []models.CommandDef) []*swagmodels.CommandDef {
	var out []*swagmodels.CommandDef
	for _, c := range cmds {
		out = append(out, &swagmodels.CommandDef{Type: c.Type, Arguments: c.Args, Timeout: int64(c.Timeout.Seconds())})
	}
	return out
}

func commandFromREST(mr *swagmodels.CommandResp) (models.Command, error) {
	id, err := models.ParseCommandID(mr.ID)
	if err != nil {
		return models.Command{}, err
	}

	shipmentID, err := models.ParseShipmentID(mr.ShipmentID)
	if err != nil {
		return models.Command{}, err
	}

	return models.Command{
		CommandDef: commandDefFromREST(mr.CommandDef),
		ID:         id,
		ShipmentID: shipmentID,
		FQDN:       mr.Fqdn,
		Status:     swagapi.CommandStatusFromREST(mr.Status),
		CreatedAt:  time.Unix(mr.CreatedAt, 0),
		UpdatedAt:  time.Unix(mr.UpdatedAt, 0),
	}, nil
}

func jobFromREST(j *swagmodels.JobResp) (models.Job, error) {
	id, err := models.ParseJobID(j.ID)
	if err != nil {
		return models.Job{}, err
	}

	cmdID, err := models.ParseCommandID(j.CommandID)
	if err != nil {
		return models.Job{}, err
	}

	return models.Job{
		ID:        id,
		ExtID:     j.ExtID,
		CommandID: cmdID,
		Status:    swagapi.JobStatusFromREST(j.Status),
		CreatedAt: time.Unix(j.CreatedAt, 0),
		UpdatedAt: time.Unix(j.UpdatedAt, 0),
	}, nil
}

func jobResultFromREST(jr *swagmodels.JobResultResp) models.JobResult {
	return models.JobResult{
		JobResultID: models.JobResultID(jr.ID),
		ExtID:       jr.ExtID,
		FQDN:        jr.Fqdn,
		Order:       int(jr.Order),
		Status:      swagapi.JobResultStatusFromREST(jr.Status),
		Result:      json.RawMessage(jr.Result),
		RecordedAt:  time.Unix(jr.RecordedAt, 0),
	}
}
