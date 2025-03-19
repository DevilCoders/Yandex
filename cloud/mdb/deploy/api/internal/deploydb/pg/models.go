package pg

import (
	"database/sql"
	"encoding/json"
	"time"

	"github.com/jackc/pgtype"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type groupModel struct {
	ID   int64  `db:"id"`
	Name string `db:"name"`
}

func groupFromDB(g groupModel) models.Group {
	return models.Group{
		ID:   models.GroupID(g.ID),
		Name: g.Name,
	}
}

type masterModel struct {
	ID           int64            `db:"id"`
	Aliases      pgtype.TextArray `db:"aliases"`
	AliveCheckAt time.Time        `db:"alive_check_at"`
	CreatedAt    time.Time        `db:"created_at"`
	FQDN         string           `db:"fqdn"`
	Group        string           `db:"deploy_group"`
	Description  sql.NullString   `db:"description"`
	IsOpen       bool             `db:"is_open"`
	Alive        bool             `db:"is_alive"`
}

func masterFromDB(m masterModel) models.Master {
	out := models.Master{
		ID:           models.MasterID(m.ID),
		FQDN:         m.FQDN,
		Group:        m.Group,
		IsOpen:       m.IsOpen,
		AliveCheckAt: m.AliveCheckAt,
		Alive:        m.Alive,
		CreatedAt:    m.CreatedAt,
	}

	if m.Description.Valid {
		out.Description = m.Description.String
	}

	for _, a := range m.Aliases.Elements {
		out.Aliases = append(out.Aliases, a.String)
	}

	return out
}

type minionModel struct {
	ID            int64          `db:"id"`
	FQDN          string         `db:"fqdn"`
	Group         string         `db:"deploy_group"`
	MasterFQDN    string         `db:"master_fqdn"`
	CreatedAt     time.Time      `db:"created_at"`
	UpdatedAt     time.Time      `db:"updated_at"`
	RegisterUntil sql.NullTime   `db:"register_until"`
	PublicKey     sql.NullString `db:"pub_key"`
	AutoReassign  bool           `db:"auto_reassign"`
	Registered    bool           `db:"registered"`
	Deleted       bool           `db:"deleted"`
}

func minionFromDB(m minionModel) models.Minion {
	return models.Minion{
		ID:           models.MinionID(m.ID),
		FQDN:         m.FQDN,
		Group:        m.Group,
		AutoReassign: m.AutoReassign,
		MasterFQDN:   m.MasterFQDN,
		PublicKey:    m.PublicKey.String,
		// TODO: this is not passed to REST API (do we even need it there? do we need this value at all?)
		// This can be deduced from public key
		Registered:    m.Registered,
		CreatedAt:     m.CreatedAt,
		UpdatedAt:     m.UpdatedAt,
		RegisterUntil: m.RegisterUntil.Time,
		Deleted:       m.Deleted,
	}
}

type commandDefModel struct {
	Type    string   `json:"type"`
	Args    []string `json:"arguments"`
	Timeout string   `json:"timeout"`
}

type shipmentModel struct {
	ID               int64            `db:"shipment_id"`
	Commands         pgtype.JSON      `db:"commands"`
	FQDNs            pgtype.TextArray `db:"fqdns"`
	Status           string           `db:"status"`
	Parallel         int64            `db:"parallel"`
	StopOnErrorCount int64            `db:"stop_on_error_count"`
	OtherCount       int64            `db:"other_count"`
	DoneCount        int64            `db:"done_count"`
	ErrorsCount      int64            `db:"errors_count"`
	TotalCount       int64            `db:"total_count"`
	CreatedAt        time.Time        `db:"created_at"`
	UpdatedAt        time.Time        `db:"updated_at"`
	Timeout          pgtype.Interval  `db:"timeout"`
	Tracing          sql.NullString   `db:"tracing"`
}

func asDuration(t pgtype.Interval) encodingutil.Duration {
	return encodingutil.FromDuration(time.Microsecond * time.Duration(t.Microseconds))
}

func commandDefFromDB(s shipmentModel) ([]models.CommandDef, error) {
	var dbCommands []commandDefModel
	var out []models.CommandDef
	if err := s.Commands.AssignTo(&dbCommands); err != nil {
		return nil, xerrors.Errorf("got shipment %d with broken commands: %w", s.ID, err)
	}
	for _, dc := range dbCommands {
		dbTimeout := pgtype.Interval{}
		if err := dbTimeout.DecodeText(nil, []byte(dc.Timeout)); err != nil && dc.Timeout != "" {
			return nil, xerrors.Errorf("got shipment %d with unexpected timeout %s in command %s: %w", s.ID, dc.Timeout, dc.Type, err)
		}
		out = append(out, models.CommandDef{
			Type:    dc.Type,
			Args:    dc.Args,
			Timeout: asDuration(dbTimeout),
		})
	}
	return out, nil
}

func commandDefToDB(commands []models.CommandDef) (string, error) {
	dbCommands := make([]commandDefModel, 0, len(commands))
	for _, cd := range commands {
		dbCommands = append(dbCommands, commandDefModel{
			Type:    cd.Type,
			Args:    cd.Args,
			Timeout: cd.Timeout.String(),
		})
	}

	jsStr, err := json.Marshal(dbCommands)
	if err != nil {
		return "", xerrors.Errorf("fail while try marshal command: %w", err)
	}
	return string(jsStr), nil
}

func shipmentFromDB(s shipmentModel) (models.Shipment, error) {
	commands, err := commandDefFromDB(s)
	if err != nil {
		return models.Shipment{}, err
	}

	out := models.Shipment{
		ID:               models.ShipmentID(s.ID),
		Commands:         commands,
		Status:           models.ParseShipmentStatus(s.Status),
		Parallel:         s.Parallel,
		StopOnErrorCount: s.StopOnErrorCount,
		OtherCount:       s.OtherCount,
		DoneCount:        s.DoneCount,
		ErrorsCount:      s.ErrorsCount,
		TotalCount:       s.TotalCount,
		CreatedAt:        s.CreatedAt,
		UpdatedAt:        s.UpdatedAt,
		Timeout:          asDuration(s.Timeout),
	}

	for _, t := range s.FQDNs.Elements {
		out.FQDNs = append(out.FQDNs, t.String)
	}

	// Tracing will be missing for old shipments
	tr, err := tracingFromDB(s.Tracing)
	if err != nil {
		return models.Shipment{}, err
	}
	out.Tracing = tr

	return out, nil
}

type commandModel struct {
	ID         int64            `db:"id"`
	ShipmentID int64            `db:"shipment_id"`
	Type       string           `db:"type"`
	Args       pgtype.TextArray `db:"arguments"`
	FQDN       string           `db:"fqdn"`
	Status     string           `db:"status"`
	CreatedAt  time.Time        `db:"created_at"`
	UpdatedAt  time.Time        `db:"updated_at"`
	Timeout    pgtype.Interval  `db:"timeout"`
}

func commandFromDB(c commandModel) models.Command {
	out := models.Command{
		CommandDef: models.CommandDef{
			Type:    c.Type,
			Timeout: asDuration(c.Timeout),
		},
		ID:         models.CommandID(c.ID),
		ShipmentID: models.ShipmentID(c.ShipmentID),
		FQDN:       c.FQDN,
		Status:     models.ParseCommandStatus(c.Status),
		CreatedAt:  c.CreatedAt,
		UpdatedAt:  c.UpdatedAt,
	}

	for _, t := range c.Args.Elements {
		out.Args = append(out.Args, t.String)
	}

	return out
}

type jobModel struct {
	ID        int64     `db:"id"`
	ExtID     string    `db:"ext_id"`
	CommandID int64     `db:"command_id"`
	Status    string    `db:"status"`
	CreatedAt time.Time `db:"created_at"`
	UpdatedAt time.Time `db:"updated_at"`
}

func jobFromDB(j jobModel) models.Job {
	return models.Job{
		ID:        models.JobID(j.ID),
		ExtID:     j.ExtID,
		CommandID: models.CommandID(j.CommandID),
		Status:    models.ParseJobStatus(j.Status),
		CreatedAt: j.CreatedAt,
		UpdatedAt: j.UpdatedAt,
	}
}

type jobResultModel struct {
	ID         int64          `db:"id"`
	ExtID      string         `db:"ext_id"`
	FQDN       string         `db:"fqdn"`
	OrderID    int            `db:"order_id"`
	Status     sql.NullString `db:"status"`
	Result     sql.NullString `db:"result"`
	RecordedAt time.Time      `db:"recorded_at"`
}

func jobResultFromDB(jr jobResultModel) models.JobResult {
	status := models.JobResultStatusUnknown
	if jr.Status.Valid {
		status = models.ParseJobResultStatus(jr.Status.String)
	}

	return models.JobResult{
		JobResultID: models.JobResultID(jr.ID),
		ExtID:       jr.ExtID,
		FQDN:        jr.FQDN,
		Order:       jr.OrderID,
		Status:      status,
		Result:      json.RawMessage(jr.Result.String),
		RecordedAt:  jr.RecordedAt,
	}
}

const (
	sortOrderAsc  = true
	sortOrderDesc = false
)

func sortOrderToDB(so models.SortOrder) bool {
	switch so {
	case models.SortOrderAsc:
		return sortOrderAsc
	case models.SortOrderDesc:
		return sortOrderDesc
	default:
		return sortOrderAsc
	}
}

type jobResultCoords struct {
	ShipmentID models.ShipmentID `db:"shipment_id"`
	CommandID  models.CommandID  `db:"command_id"`
	JobID      models.JobID      `db:"job_id"`
	Tracing    sql.NullString    `db:"tracing"`
}

func jobResultCoordsFromDB(jrc jobResultCoords) (models.JobResultCoords, error) {
	tr, err := tracingFromDB(jrc.Tracing)
	if err != nil {
		return models.JobResultCoords{}, err
	}

	return models.JobResultCoords{
		ShipmentID: jrc.ShipmentID,
		CommandID:  jrc.CommandID,
		JobID:      jrc.JobID,
		Tracing:    tr,
	}, nil
}

func tracingFromDB(t sql.NullString) (opentracing.TextMapCarrier, error) {
	if !t.Valid {
		return nil, nil
	}

	return tracing.UnmarshalTextMapCarrier([]byte(t.String))
}
