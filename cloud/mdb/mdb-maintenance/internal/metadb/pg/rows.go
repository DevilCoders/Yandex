package pg

import (
	"database/sql"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type maintenanceTaskRow struct {
	ClusterID string         `db:"cid"`
	ConfigID  string         `db:"config_id"`
	TaskID    sql.NullString `db:"task_id"`
	CreateTS  time.Time      `db:"create_ts"`
	PlanTS    sql.NullTime   `db:"plan_ts"`
	Status    string         `db:"status"`
	MaxDelay  time.Time      `db:"max_delay"`
}

func convertMWTaskStatus(db string) (models.MaintenanceTaskStatus, error) {
	var s models.MaintenanceTaskStatus
	switch db {
	case "PLANNED":
		s = models.MaintenanceTaskPlanned
	case "CANCELED":
		s = models.MaintenanceTaskCanceled
	case "COMPLETED":
		s = models.MaintenanceTaskCompleted
	case "REJECTED":
		s = models.MaintenanceTaskRejected
	case "FAILED":
		s = models.MaintenanceTaskFailed
	default:
		return s, fmt.Errorf("unknown maintenance task status: %q", db)
	}
	return s, nil
}

func (r maintenanceTaskRow) render() (models.MaintenanceTask, error) {
	m := models.MaintenanceTask{
		ClusterID: r.ClusterID,
		ConfigID:  r.ConfigID,
		CreateTS:  r.CreateTS,
		MaxDelay:  r.MaxDelay,
	}

	status, err := convertMWTaskStatus(r.Status)
	if err != nil {
		return models.MaintenanceTask{}, err
	}
	m.Status = status

	if r.TaskID.Valid {
		m.TaskID = optional.NewString(r.TaskID.String)
	}
	if r.PlanTS.Valid {
		m.PlanTS = optional.NewTime(r.PlanTS.Time)
	}
	return m, nil
}

func ConvertMWDay(rowDay string) (time.Weekday, error) {
	var mwDay time.Weekday
	switch rowDay {
	case "MON":
		mwDay = time.Monday
	case "TUE":
		mwDay = time.Tuesday
	case "WED":
		mwDay = time.Wednesday
	case "THU":
		mwDay = time.Thursday
	case "FRI":
		mwDay = time.Friday
	case "SAT":
		mwDay = time.Saturday
	case "SUN":
		mwDay = time.Saturday
	default:
		return mwDay, xerrors.Errorf("got unsupported maintenance window day: %q", rowDay)
	}
	return mwDay, nil
}

type clusterRow struct {
	CID              string         `db:"cid"`
	FolderID         int64          `db:"folder_id"`
	FolderExtID      string         `db:"folder_ext_id"`
	ClusterType      string         `db:"type"`
	ClusterName      string         `db:"name"`
	CloudExtID       string         `db:"cloud_ext_id"`
	Day              sql.NullString `db:"day"`
	UpperBoundMWHour sql.NullInt32  `db:"hour"`
	Env              string         `db:"env"`
	HasTasks         bool           `db:"has_tasks"`
}

func convertEnv(dbEnv string) (models.Env, error) {
	switch dbEnv {
	case "dev":
		return models.EnvDev, nil
	case "qa":
		return models.EnvQA, nil
	case "prod":
		return models.EnvProd, nil
	case "compute-prod":
		return models.EnvComputeProd, nil
	default:
		return 0, xerrors.Errorf("unexpected cluster env %q", dbEnv)
	}
}

func (r clusterRow) render() (models.Cluster, error) {
	var mwSettings models.MaintenanceSettings
	if r.Day.Valid && r.UpperBoundMWHour.Valid {
		mwDay, err := ConvertMWDay(r.Day.String)
		if err != nil {
			return models.Cluster{}, err
		}
		mwSettings = models.NewMaintenanceSettings(mwDay, int(r.UpperBoundMWHour.Int32))
	}

	env, err := convertEnv(r.Env)
	if err != nil {
		return models.Cluster{}, err
	}

	return models.Cluster{
		ID:             r.CID,
		ClusterType:    r.ClusterType,
		ClusterName:    r.ClusterName,
		FolderID:       r.FolderID,
		FolderExtID:    r.FolderExtID,
		CloudExtID:     r.CloudExtID,
		Settings:       mwSettings,
		Env:            env,
		HasActiveTasks: r.HasTasks,
	}, nil
}
