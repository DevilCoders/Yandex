package clickhouse

import (
	"database/sql"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
)

type SessionStats struct {
	Time          sql.NullInt64  `db:"time"`
	SessionCount  int64          `db:"session_count"`
	Host          sql.NullString `db:"host"`
	Database      sql.NullString `db:"database"`
	User          sql.NullString `db:"user"`
	WaitEventType sql.NullString `db:"wait_event_type"`
	WaitEvent     sql.NullString `db:"wait_event"`
	State         sql.NullString `db:"state"`
	Query         sql.NullString `db:"query"`
}

func (ss SessionStats) makeDimensionsMap() map[string]string {
	m := make(map[string]string)
	if ss.Host.Valid {
		m[string(pgmodels.PGStatActivityHost)] = ss.Host.String
	}
	if ss.Database.Valid {
		m[string(pgmodels.PGStatActivityDatabase)] = ss.Database.String
	}
	if ss.User.Valid {
		m[string(pgmodels.PGStatActivityUser)] = ss.User.String
	}
	if ss.WaitEventType.Valid {
		m[string(pgmodels.PGStatActivityWaitEventType)] = ss.WaitEventType.String
	}
	if ss.WaitEvent.Valid {
		m[string(pgmodels.PGStatActivityWaitEvent)] = ss.WaitEvent.String
	}
	if ss.State.Valid {
		m[string(pgmodels.PGStatActivityState)] = ss.State.String
	}
	if ss.Query.Valid {
		m[string(pgmodels.PGStatActivityQuery)] = ss.Query.String
	}
	return m
}

type SessionState struct {
	Timestamp       time.Time    `db:"collect_time"`
	Host            string       `db:"host"`
	Pid             int64        `db:"pid"`
	Database        string       `db:"database"`
	User            string       `db:"user"`
	ApplicationName string       `db:"application_name"`
	BackendStart    sql.NullTime `db:"backend_start"`
	XactStart       sql.NullTime `db:"xact_start"`
	QueryStart      sql.NullTime `db:"query_start"`
	StateChange     sql.NullTime `db:"state_change"`
	WaitEventType   string       `db:"wait_event_type"`
	WaitEvent       string       `db:"wait_event"`
	State           string       `db:"state"`
	Query           string       `db:"query"`
	BackendType     string       `db:"backend_type"`
}

func (s SessionState) toExt() pgmodels.SessionState {
	v := pgmodels.SessionState{
		Timestamp:       s.Timestamp,
		Host:            s.Host,
		Pid:             s.Pid,
		Database:        s.Database,
		User:            s.User,
		ApplicationName: s.ApplicationName,
		BackendStart:    optional.Time(s.BackendStart),
		XactStart:       optional.Time(s.XactStart),
		QueryStart:      optional.Time(s.QueryStart),
		StateChange:     optional.Time(s.StateChange),
		WaitEventType:   s.WaitEventType,
		WaitEvent:       s.WaitEvent,
		State:           s.State,
		Query:           s.Query,
		BackendType:     s.BackendType,
	}
	return v
}

type Statements struct {
	Timestamp         time.Time `db:"collect_time"`
	Host              string    `db:"host"`
	User              string    `db:"user"`
	Database          string    `db:"database"`
	Queryid           string    `db:"queryid"`
	Query             string    `db:"query"`
	Calls             int64     `db:"calls"`
	TotalTime         float64   `db:"total_time"`
	MinTime           float64   `db:"min_time"`
	MaxTime           float64   `db:"max_time"`
	MeanTime          float64   `db:"mean_time"`
	StddevTime        float64   `db:"stddev_time"`
	Rows              int64     `db:"rows"`
	SharedBlksHit     int64     `db:"shared_blks_hit"`
	SharedBlksRead    int64     `db:"shared_blks_read"`
	SharedBlksDirtied int64     `db:"shared_blks_dirtied"`
	SharedBlksWritten int64     `db:"shared_blks_written"`
	BlkReadTime       float64   `db:"blk_read_time"`
	BlkWriteTime      float64   `db:"blk_write_time"`
	TempBlksRead      int64     `db:"temp_blks_read"`
	TempBlksWritten   int64     `db:"temp_blks_written"`
	Reads             int64     `db:"reads"`
	Writes            int64     `db:"writes"`
	UserTime          float64   `db:"user_time"`
	SystemTime        float64   `db:"system_time"`
}

func (s Statements) toExt() pgmodels.Statements {
	return pgmodels.Statements(s)
}

type StatementsInterval struct {
	Timestamp         time.Time `db:"collect_time_max"`
	Host              string    `db:"host"`
	User              string    `db:"user"`
	Database          string    `db:"database"`
	Queryid           string    `db:"queryid"`
	Query             string    `db:"query"`
	Calls             int64     `db:"calls"`
	TotalTime         float64   `db:"total_time"`
	MinTime           float64   `db:"min_time"`
	MaxTime           float64   `db:"max_time"`
	MeanTime          float64   `db:"mean_time"`
	StddevTime        float64   `db:"stddev_time"`
	Rows              int64     `db:"rows"`
	SharedBlksHit     int64     `db:"shared_blks_hit"`
	SharedBlksRead    int64     `db:"shared_blks_read"`
	SharedBlksDirtied int64     `db:"shared_blks_dirtied"`
	SharedBlksWritten int64     `db:"shared_blks_written"`
	BlkReadTime       float64   `db:"blk_read_time"`
	BlkWriteTime      float64   `db:"blk_write_time"`
	TempBlksRead      int64     `db:"temp_blks_read"`
	TempBlksWritten   int64     `db:"temp_blks_written"`
	Reads             int64     `db:"reads"`
	Writes            int64     `db:"writes"`
	UserTime          float64   `db:"user_time"`
	SystemTime        float64   `db:"system_time"`
}

func (s StatementsInterval) toExt() pgmodels.Statements {
	return pgmodels.Statements(s)
}

type Interval struct {
	StartTimestamp sql.NullTime `db:"start_collect_time"`
	EndTimestamp   sql.NullTime `db:"end_collect_time"`
}

func (s Interval) toExt() pgmodels.Interval {
	v := pgmodels.Interval{
		StartTimestamp: optional.Time(s.StartTimestamp),
		EndTimestamp:   optional.Time(s.EndTimestamp),
	}
	return v
}

type DiffStatement struct {
	FirstIntervalTime       Interval `db:"first"`
	SecondIntervalTime      Interval `db:"second"`
	Host                    string   `db:"host"`
	User                    string   `db:"user"`
	Database                string   `db:"database"`
	Queryid                 string   `db:"queryid"`
	Query                   string   `db:"query"`
	FirstCalls              int64    `db:"first_calls"`
	SecondCalls             int64    `db:"second_calls"`
	FirstTotalTime          float64  `db:"first_total_time"`
	SecondTotalTime         float64  `db:"second_total_time"`
	FirstMinTime            float64  `db:"first_min_time"`
	SecondMinTime           float64  `db:"second_min_time"`
	FirstMaxTime            float64  `db:"first_max_time"`
	SecondMaxTime           float64  `db:"second_max_time"`
	FirstMeanTime           float64  `db:"first_mean_time"`
	SecondMeanTime          float64  `db:"second_mean_time"`
	FirstStddevTime         float64  `db:"first_stddev_time"`
	SecondStddevTime        float64  `db:"second_stddev_time"`
	FirstRows               int64    `db:"first_rows"`
	SecondRows              int64    `db:"second_rows"`
	FirstSharedBlksHit      int64    `db:"first_shared_blks_hit"`
	SecondSharedBlksHit     int64    `db:"second_shared_blks_hit"`
	FirstSharedBlksRead     int64    `db:"first_shared_blks_read"`
	SecondSharedBlksRead    int64    `db:"second_shared_blks_read"`
	FirstSharedBlksDirtied  int64    `db:"first_shared_blks_dirtied"`
	SecondSharedBlksDirtied int64    `db:"second_shared_blks_dirtied"`
	FirstSharedBlksWritten  int64    `db:"first_shared_blks_written"`
	SecondSharedBlksWritten int64    `db:"second_shared_blks_written"`
	FirstBlkReadTime        float64  `db:"first_blk_read_time"`
	SecondBlkReadTime       float64  `db:"second_blk_read_time"`
	FirstBlkWriteTime       float64  `db:"first_blk_write_time"`
	SecondBlkWriteTime      float64  `db:"second_blk_write_time"`
	FirstTempBlksRead       int64    `db:"first_temp_blks_read"`
	SecondTempBlksRead      int64    `db:"second_temp_blks_read"`
	FirstTempBlksWritten    int64    `db:"first_temp_blks_written"`
	SecondTempBlksWritten   int64    `db:"second_temp_blks_written"`
	FirstReads              int64    `db:"first_reads"`
	SecondReads             int64    `db:"second_reads"`
	FirstWrites             int64    `db:"first_writes"`
	SecondWrites            int64    `db:"second_writes"`
	FirstUserTime           float64  `db:"first_user_time"`
	SecondUserTime          float64  `db:"second_user_time"`
	FirstSystemTime         float64  `db:"first_system_time"`
	SecondSystemTime        float64  `db:"second_system_time"`
	DiffCalls               float64  `db:"diff_calls"`
	DiffTotalTime           float64  `db:"diff_total_time"`
	DiffMinTime             float64  `db:"diff_min_time"`
	DiffMaxTime             float64  `db:"diff_max_time"`
	DiffMeanTime            float64  `db:"diff_mean_time"`
	DiffStddevTime          float64  `db:"diff_stddev_time"`
	DiffRows                float64  `db:"diff_rows"`
	DiffSharedBlksHit       float64  `db:"diff_shared_blks_hit"`
	DiffSharedBlksRead      float64  `db:"diff_shared_blks_read"`
	DiffSharedBlksDirtied   float64  `db:"diff_shared_blks_dirtied"`
	DiffSharedBlksWritten   float64  `db:"diff_shared_blks_written"`
	DiffBlkReadTime         float64  `db:"diff_blk_read_time"`
	DiffBlkWriteTime        float64  `db:"diff_blk_write_time"`
	DiffTempBlksRead        float64  `db:"diff_temp_blks_read"`
	DiffTempBlksWritten     float64  `db:"diff_temp_blks_written"`
	DiffReads               float64  `db:"diff_reads"`
	DiffWrites              float64  `db:"diff_writes"`
	DiffUserTime            float64  `db:"diff_user_time"`
	DiffSystemTime          float64  `db:"diff_system_time"`
}

func (ds DiffStatement) toExt() pgmodels.DiffStatement {
	v := pgmodels.DiffStatement{
		FirstIntervalTime:       ds.FirstIntervalTime.toExt(),
		SecondIntervalTime:      ds.SecondIntervalTime.toExt(),
		Host:                    ds.Host,
		User:                    ds.User,
		Database:                ds.Database,
		Queryid:                 ds.Queryid,
		Query:                   ds.Query,
		FirstCalls:              ds.FirstCalls,
		SecondCalls:             ds.SecondCalls,
		FirstTotalTime:          ds.FirstTotalTime,
		SecondTotalTime:         ds.SecondTotalTime,
		FirstMinTime:            ds.FirstMinTime,
		SecondMinTime:           ds.SecondMinTime,
		FirstMaxTime:            ds.FirstMaxTime,
		SecondMaxTime:           ds.SecondMaxTime,
		FirstMeanTime:           ds.FirstMeanTime,
		SecondMeanTime:          ds.SecondMeanTime,
		FirstStddevTime:         ds.FirstStddevTime,
		SecondStddevTime:        ds.SecondStddevTime,
		FirstRows:               ds.FirstRows,
		SecondRows:              ds.SecondRows,
		FirstSharedBlksHit:      ds.FirstSharedBlksHit,
		SecondSharedBlksHit:     ds.SecondSharedBlksHit,
		FirstSharedBlksRead:     ds.FirstSharedBlksRead,
		SecondSharedBlksRead:    ds.SecondSharedBlksRead,
		FirstSharedBlksDirtied:  ds.FirstSharedBlksDirtied,
		SecondSharedBlksDirtied: ds.SecondSharedBlksDirtied,
		FirstSharedBlksWritten:  ds.FirstSharedBlksWritten,
		SecondSharedBlksWritten: ds.SecondSharedBlksWritten,
		FirstBlkReadTime:        ds.FirstBlkReadTime,
		SecondBlkReadTime:       ds.SecondBlkReadTime,
		FirstBlkWriteTime:       ds.FirstBlkWriteTime,
		SecondBlkWriteTime:      ds.SecondBlkWriteTime,
		FirstTempBlksRead:       ds.FirstTempBlksRead,
		SecondTempBlksRead:      ds.SecondTempBlksRead,
		FirstTempBlksWritten:    ds.FirstTempBlksWritten,
		SecondTempBlksWritten:   ds.SecondTempBlksWritten,
		FirstReads:              ds.FirstReads,
		SecondReads:             ds.SecondReads,
		FirstWrites:             ds.FirstWrites,
		SecondWrites:            ds.SecondWrites,
		FirstUserTime:           ds.FirstUserTime,
		SecondUserTime:          ds.SecondUserTime,
		FirstSystemTime:         ds.FirstSystemTime,
		SecondSystemTime:        ds.SecondSystemTime,
		DiffCalls:               ds.DiffCalls,
		DiffTotalTime:           ds.DiffTotalTime,
		DiffMinTime:             ds.DiffMinTime,
		DiffMaxTime:             ds.DiffMaxTime,
		DiffMeanTime:            ds.DiffMeanTime,
		DiffStddevTime:          ds.DiffStddevTime,
		DiffRows:                ds.DiffRows,
		DiffSharedBlksHit:       ds.DiffSharedBlksHit,
		DiffSharedBlksRead:      ds.DiffSharedBlksRead,
		DiffSharedBlksDirtied:   ds.DiffSharedBlksDirtied,
		DiffSharedBlksWritten:   ds.DiffSharedBlksWritten,
		DiffBlkReadTime:         ds.DiffBlkReadTime,
		DiffBlkWriteTime:        ds.DiffBlkWriteTime,
		DiffTempBlksRead:        ds.DiffTempBlksRead,
		DiffTempBlksWritten:     ds.DiffTempBlksWritten,
		DiffReads:               ds.DiffReads,
		DiffWrites:              ds.DiffWrites,
		DiffUserTime:            ds.DiffUserTime,
		DiffSystemTime:          ds.DiffSystemTime,
	}
	return v
}
