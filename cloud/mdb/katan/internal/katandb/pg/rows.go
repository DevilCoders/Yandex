package pg

import (
	"database/sql"
	"fmt"
	"time"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type hostRow struct {
	FQDN      string `db:"fqdn"`
	ClusterID string `db:"cluster_id"`
	Tags      string `db:"tags"`
}

func (r hostRow) render() katandb.Host {
	return katandb.Host{
		ClusterID: r.ClusterID,
		FQDN:      r.FQDN,
		Tags:      r.Tags,
	}
}

type hostsParser struct {
	ret []katandb.Host
}

func (hp *hostsParser) parse(rows *sqlx.Rows) error {
	var row hostRow
	if err := rows.StructScan(&row); err != nil {
		return err
	}
	hp.ret = append(hp.ret, row.render())
	return nil
}

type clusterRow struct {
	ID   string `db:"cluster_id"`
	Tags string `db:"tags"`
}

func (r clusterRow) render() katandb.Cluster {
	return katandb.Cluster{
		ID:   r.ID,
		Tags: r.Tags,
	}
}

type clustersParser struct {
	ret []katandb.Cluster
}

func (hp *clustersParser) parse(rows *sqlx.Rows) error {
	var row clusterRow
	if err := rows.StructScan(&row); err != nil {
		return err
	}
	hp.ret = append(hp.ret, row.render())
	return nil
}

type rolloutRow struct {
	ID         int64          `db:"rollout_id"`
	Commands   string         `db:"commands"`
	Options    string         `db:"options"`
	CreateAt   time.Time      `db:"created_at"`
	Parallel   int            `db:"parallel"`
	FinishedAt sql.NullTime   `db:"finished_at"`
	CreatedBy  sql.NullString `db:"created_by"`
	ScheduleID sql.NullInt64  `db:"schedule_id"`
	Comment    sql.NullString `db:"comment"`
}

func (r rolloutRow) render() katandb.Rollout {
	up := katandb.Rollout{
		ID:         r.ID,
		Commands:   r.Commands,
		Options:    r.Options,
		CreateAt:   r.CreateAt,
		Parallel:   r.Parallel,
		FinishedAt: optional.Time{},
		CreatedBy:  "",
		ScheduleID: optional.Int64{},
	}
	if r.ScheduleID.Valid {
		up.ScheduleID.Set(r.ScheduleID.Int64)
	}
	if r.CreatedBy.Valid {
		up.CreatedBy = r.CreatedBy.String
	}
	if r.FinishedAt.Valid {
		up.FinishedAt.Set(r.FinishedAt.Time)
	}
	if r.Comment.Valid {
		up.Comment.Set(r.Comment.String)
	}
	return up
}

type rolloutsParser struct {
	ret []katandb.Rollout
}

func (hp *rolloutsParser) parse(rows *sqlx.Rows) error {
	var row rolloutRow
	if err := rows.StructScan(&row); err != nil {
		return err
	}
	hp.ret = append(hp.ret, row.render())
	return nil
}

type rolloutDatesRow struct {
	ID                   int64          `db:"rollout_id"`
	CreatedAt            time.Time      `db:"created_at"`
	LastUpdatedAt        sql.NullTime   `db:"last_updated_at"`
	LastUpdatedAtCluster sql.NullString `db:"last_updated_at_cluster"`
}

func (r rolloutDatesRow) render() katandb.RolloutDates {
	model := katandb.RolloutDates{
		ID:        r.ID,
		CreatedAt: r.CreatedAt,
	}
	if r.LastUpdatedAt.Valid {
		model.LastUpdatedAt.Set(r.LastUpdatedAt.Time)
	}
	if r.LastUpdatedAtCluster.Valid {
		model.LastUpdatedAtCluster.Set(r.LastUpdatedAtCluster.String)
	}
	return model
}

func parseClusterRolloutState(s string) katandb.ClusterRolloutState {
	switch s {
	case "pending":
		return katandb.ClusterRolloutPending
	case "running":
		return katandb.ClusterRolloutRunning
	case "succeeded":
		return katandb.ClusterRolloutSucceeded
	case "cancelled":
		return katandb.ClusterRolloutCancelled
	case "skipped":
		return katandb.ClusterRolloutSkipped
	case "failed":
		return katandb.ClusterRolloutFailed
	}
	return katandb.ClusterRolloutUnknown
}

type clusterRolloutRow struct {
	ClusterID string         `db:"cluster_id"`
	RolloutID int64          `db:"rollout_id"`
	State     string         `db:"state"`
	Comment   sql.NullString `db:"comment"`
	UpdatedAt time.Time      `db:"updated_at"`
}

func (r clusterRolloutRow) render() katandb.ClusterRollout {
	model := katandb.ClusterRollout{
		ClusterID: r.ClusterID,
		RolloutID: r.RolloutID,
		State:     parseClusterRolloutState(r.State),
		UpdatedAt: r.UpdatedAt,
	}
	if r.Comment.Valid {
		model.Comment.Set(r.Comment.String)
	}
	return model
}

type clusterRolloutsParser struct {
	ret []katandb.ClusterRollout
}

func (hp *clusterRolloutsParser) parse(rows *sqlx.Rows) error {
	var row clusterRolloutRow
	if err := rows.StructScan(&row); err != nil {
		return err
	}
	hp.ret = append(hp.ret, row.render())
	return nil
}

type listOfStringParser struct {
	ret []string
}

func (p *listOfStringParser) parse(rows *sqlx.Rows) error {
	var s string
	if err := rows.Scan(&s); err != nil {
		return err
	}
	p.ret = append(p.ret, s)
	return nil
}

type scheduleRow struct {
	ID                int64            `db:"schedule_id"`
	MatchTags         string           `db:"match_tags"`
	Commands          string           `db:"commands"`
	State             string           `db:"state"`
	Age               pgtype.Interval  `db:"age"`
	StillAge          pgtype.Interval  `db:"still_age"`
	MaxSize           int64            `db:"max_size"`
	Parallel          int32            `db:"parallel"`
	Options           string           `db:"options"`
	DependsOn         pgtype.Int8Array `db:"depends_on"`
	EditedAt          time.Time        `db:"edited_at"`
	EditedBy          sql.NullString   `db:"edited_by"`
	ExaminedRolloutID sql.NullInt64    `db:"examined_rollout_id"`
	Name              string           `db:"name"`
	Namespace         string           `db:"namespace"`
}

func assignIntervalToDuration(val pgtype.Interval, dur *time.Duration) error {
	if val.Status != pgtype.Present {
		return fmt.Errorf("%v is null or not defined", val)
	}
	*dur = time.Duration(val.Microseconds)*time.Microsecond + time.Hour*24*time.Duration(val.Days) + time.Hour*24*30*time.Duration(val.Months)

	return nil
}

func (r scheduleRow) render() (katandb.Schedule, error) {
	model := katandb.Schedule{
		ID:        r.ID,
		MatchTags: r.MatchTags,
		Commands:  r.Commands,
		Options:   r.Options,
		State:     parseScheduleState(r.State),
		MaxSize:   r.MaxSize,
		Parallel:  r.Parallel,
		EditedAt:  r.EditedAt,
		Name:      r.Name,
		Namespace: r.Namespace,
	}

	if r.EditedBy.Valid {
		model.EditedBy.Set(r.EditedBy.String)
	}

	if r.ExaminedRolloutID.Valid {
		model.ExaminedRolloutID.Set(r.ExaminedRolloutID.Int64)
	}

	if err := assignIntervalToDuration(r.Age, &model.Age); err != nil {
		return katandb.Schedule{}, xerrors.Errorf("fail to convert Age: %w", err)
	}

	if err := assignIntervalToDuration(r.StillAge, &model.StillAge); err != nil {
		return katandb.Schedule{}, xerrors.Errorf("fail to convert StillAge: %w", err)
	}

	if err := r.DependsOn.AssignTo(&model.DependsOn); err != nil {
		return katandb.Schedule{}, xerrors.Errorf("fail to convert DependsOn: %w", err)
	}
	return model, nil
}

func parseScheduleState(s string) katandb.ScheduleState {
	switch s {
	case "active":
		return katandb.ScheduleStateActive
	case "stopped":
		return katandb.ScheduleStateStopped
	case "broken":
		return katandb.ScheduleStateBroken
	}
	return katandb.ScheduleStateUnknown
}

type schedulesParser struct {
	ret []katandb.Schedule
}

func (sp *schedulesParser) parse(rows *sqlx.Rows) error {
	var row scheduleRow
	if err := rows.StructScan(&row); err != nil {
		return err
	}

	model, err := row.render()
	if err != nil {
		return err
	}
	sp.ret = append(sp.ret, model)
	return nil
}
