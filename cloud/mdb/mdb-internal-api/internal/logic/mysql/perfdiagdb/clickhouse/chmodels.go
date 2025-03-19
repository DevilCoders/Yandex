package clickhouse

import (
	"database/sql"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
)

type SessionStats struct {
	Time           sql.NullInt64  `db:"time"`
	Host           sql.NullString `db:"host"`
	Database       sql.NullString `db:"database"`
	User           sql.NullString `db:"user"`
	Stage          sql.NullString `db:"stage"`
	CurrentWait    sql.NullString `db:"current_wait"`
	WaitObject     sql.NullString `db:"wait_object"`
	ClientHostname sql.NullString `db:"client_hostname"`
	Digest         sql.NullString `db:"digest"`
	Query          sql.NullString `db:"query"`
	SessionCount   int64          `db:"session_count"`
}

func (ss SessionStats) makeDimensionsMap() map[string]string {
	m := make(map[string]string)
	if ss.Host.Valid {
		m[string(mymodels.MySessionsHost)] = ss.Host.String
	}
	if ss.Database.Valid {
		m[string(mymodels.MySessionsDatabase)] = ss.Database.String
	}
	if ss.User.Valid {
		m[string(mymodels.MySessionsUser)] = ss.User.String
	}
	if ss.Stage.Valid {
		m[string(mymodels.MySessionsStage)] = ss.Stage.String
	}
	if ss.CurrentWait.Valid {
		m[string(mymodels.MySessionsCurrentWait)] = ss.CurrentWait.String
	}
	if ss.WaitObject.Valid {
		m[string(mymodels.MySessionsWaitObject)] = ss.WaitObject.String
	}
	if ss.ClientHostname.Valid {
		m[string(mymodels.MySessionsClientHostname)] = ss.ClientHostname.String
	}
	if ss.Digest.Valid {
		m[string(mymodels.MySessionsDigest)] = ss.Digest.String
	}
	if ss.Query.Valid {
		m[string(mymodels.MySessionsQuery)] = ss.Query.String
	}
	return m
}

type SessionState struct {
	Timestamp      time.Time `db:"collect_time"`
	Host           string    `db:"host"`
	Database       string    `db:"database"`
	User           string    `db:"user"`
	ThdID          int64     `db:"thd_id"`
	ConnID         int64     `db:"conn_id"`
	Command        string    `db:"command"`
	Query          string    `db:"query"`
	Digest         string    `db:"digest"`
	QueryLatency   float64   `db:"query_latency"`
	LockLatency    float64   `db:"lock_latency"`
	Stage          string    `db:"stage"`
	StageLatency   float64   `db:"stage_latency"`
	CurrentWait    string    `db:"current_wait"`
	WaitObject     string    `db:"wait_object"`
	WaitLatency    float64   `db:"wait_latency"`
	TrxLatency     float64   `db:"trx_latency"`
	CurrentMemory  int64     `db:"current_memory"`
	ClientAddr     string    `db:"client_addr"`
	ClientHostname string    `db:"client_hostname"`
	ClientPort     int64     `db:"client_port"`
}

func (s SessionState) toExt() mymodels.SessionState {
	return mymodels.SessionState(s)
}

type Statements struct {
	Timestamp           time.Time `db:"collect_time"`
	Host                string    `db:"host"`
	Database            string    `db:"database"`
	Query               string    `db:"query"`
	Digest              string    `db:"digest"`
	TotalQueryLatency   float64   `db:"total_query_latency"`
	TotalLockLatency    float64   `db:"total_lock_latency"`
	AvgQueryLatency     float64   `db:"avg_query_latency"`
	AvgLockLatency      float64   `db:"avg_lock_latency"`
	Calls               int64     `db:"calls"`
	Errors              int64     `db:"errors"`
	Warnings            int64     `db:"warnings"`
	RowsExamined        int64     `db:"rows_examined"`
	RowsSent            int64     `db:"rows_sent"`
	RowsAffected        int64     `db:"rows_affected"`
	TmpTables           int64     `db:"tmp_tables"`
	TmpDiskTables       int64     `db:"tmp_disk_tables"`
	SelectFullJoin      int64     `db:"select_full_join"`
	SelectFullRangeJoin int64     `db:"select_full_range_join"`
	SelectRange         int64     `db:"select_range"`
	SelectRangeCheck    int64     `db:"select_range_check"`
	SelectScan          int64     `db:"select_scan"`
	SortMergePasses     int64     `db:"sort_merge_passes"`
	SortRange           int64     `db:"sort_range"`
	SortRows            int64     `db:"sort_rows"`
	SortScan            int64     `db:"sort_scan"`
	NoIndexUsed         int64     `db:"no_index_used"`
	NoGoodIndexUsed     int64     `db:"no_good_index_used"`
}

func (s Statements) toExt() mymodels.Statements {
	return mymodels.Statements(s)
}

type Interval struct {
	StartTimestamp sql.NullTime `db:"start_collect_time"`
	EndTimestamp   sql.NullTime `db:"end_collect_time"`
}

func (s Interval) toExt() mymodels.Interval {
	v := mymodels.Interval{
		StartTimestamp: optional.Time(s.StartTimestamp),
		EndTimestamp:   optional.Time(s.EndTimestamp),
	}
	return v
}

type DiffStatement struct {
	FirstIntervalTime  Interval `db:"first"`
	SecondIntervalTime Interval `db:"second"`
	DiffStatementBase
}
type DiffStatementBase struct {
	Host     string `db:"host"`
	Database string `db:"database"`
	Query    string `db:"query"`
	Digest   string `db:"digest"`

	FirstTotalQueryLatency   float64 `db:"first_total_query_latency"`
	FirstTotalLockLatency    float64 `db:"first_total_lock_latency"`
	FirstAvgQueryLatency     float64 `db:"first_avg_query_latency"`
	FirstAvgLockLatency      float64 `db:"first_avg_lock_latency"`
	FirstCalls               int64   `db:"first_calls"`
	FirstErrors              int64   `db:"first_errors"`
	FirstWarnings            int64   `db:"first_warnings"`
	FirstRowsExamined        int64   `db:"first_rows_examined"`
	FirstRowsSent            int64   `db:"first_rows_sent"`
	FirstRowsAffected        int64   `db:"first_rows_affected"`
	FirstTmpTables           int64   `db:"first_tmp_tables"`
	FirstTmpDiskTables       int64   `db:"first_tmp_disk_tables"`
	FirstSelectFullJoin      int64   `db:"first_select_full_join"`
	FirstSelectFullRangeJoin int64   `db:"first_select_full_range_join"`
	FirstSelectRange         int64   `db:"first_select_range"`
	FirstSelectRangeCheck    int64   `db:"first_select_range_check"`
	FirstSelectScan          int64   `db:"first_select_scan"`
	FirstSortMergePasses     int64   `db:"first_sort_merge_passes"`
	FirstSortRange           int64   `db:"first_sort_range"`
	FirstSortRows            int64   `db:"first_sort_rows"`
	FirstSortScan            int64   `db:"first_sort_scan"`
	FirstNoIndexUsed         int64   `db:"first_no_index_used"`
	FirstNoGoodIndexUsed     int64   `db:"first_no_good_index_used"`

	SecondTotalQueryLatency   float64 `db:"second_total_query_latency"`
	SecondTotalLockLatency    float64 `db:"second_total_lock_latency"`
	SecondAvgQueryLatency     float64 `db:"second_avg_query_latency"`
	SecondAvgLockLatency      float64 `db:"second_avg_lock_latency"`
	SecondCalls               int64   `db:"second_calls"`
	SecondErrors              int64   `db:"second_errors"`
	SecondWarnings            int64   `db:"second_warnings"`
	SecondRowsExamined        int64   `db:"second_rows_examined"`
	SecondRowsSent            int64   `db:"second_rows_sent"`
	SecondRowsAffected        int64   `db:"second_rows_affected"`
	SecondTmpTables           int64   `db:"second_tmp_tables"`
	SecondTmpDiskTables       int64   `db:"second_tmp_disk_tables"`
	SecondSelectFullJoin      int64   `db:"second_select_full_join"`
	SecondSelectFullRangeJoin int64   `db:"second_select_full_range_join"`
	SecondSelectRange         int64   `db:"second_select_range"`
	SecondSelectRangeCheck    int64   `db:"second_select_range_check"`
	SecondSelectScan          int64   `db:"second_select_scan"`
	SecondSortMergePasses     int64   `db:"second_sort_merge_passes"`
	SecondSortRange           int64   `db:"second_sort_range"`
	SecondSortRows            int64   `db:"second_sort_rows"`
	SecondSortScan            int64   `db:"second_sort_scan"`
	SecondNoIndexUsed         int64   `db:"second_no_index_used"`
	SecondNoGoodIndexUsed     int64   `db:"second_no_good_index_used"`

	DiffTotalQueryLatency   float64 `db:"diff_total_query_latency"`
	DiffTotalLockLatency    float64 `db:"diff_total_lock_latency"`
	DiffAvgQueryLatency     float64 `db:"diff_avg_query_latency"`
	DiffAvgLockLatency      float64 `db:"diff_avg_lock_latency"`
	DiffCalls               float64 `db:"diff_calls"`
	DiffErrors              float64 `db:"diff_errors"`
	DiffWarnings            float64 `db:"diff_warnings"`
	DiffRowsExamined        float64 `db:"diff_rows_examined"`
	DiffRowsSent            float64 `db:"diff_rows_sent"`
	DiffRowsAffected        float64 `db:"diff_rows_affected"`
	DiffTmpTables           float64 `db:"diff_tmp_tables"`
	DiffTmpDiskTables       float64 `db:"diff_tmp_disk_tables"`
	DiffSelectFullJoin      float64 `db:"diff_select_full_join"`
	DiffSelectFullRangeJoin float64 `db:"diff_select_full_range_join"`
	DiffSelectRange         float64 `db:"diff_select_range"`
	DiffSelectRangeCheck    float64 `db:"diff_select_range_check"`
	DiffSelectScan          float64 `db:"diff_select_scan"`
	DiffSortMergePasses     float64 `db:"diff_sort_merge_passes"`
	DiffSortRange           float64 `db:"diff_sort_range"`
	DiffSortRows            float64 `db:"diff_sort_rows"`
	DiffSortScan            float64 `db:"diff_sort_scan"`
	DiffNoIndexUsed         float64 `db:"diff_no_index_used"`
	DiffNoGoodIndexUsed     float64 `db:"diff_no_good_index_used"`
}

func (ds DiffStatement) toExt() mymodels.DiffStatement {
	return mymodels.DiffStatement{
		FirstIntervalTime:  ds.FirstIntervalTime.toExt(),
		SecondIntervalTime: ds.SecondIntervalTime.toExt(),
		DiffStatementBase:  mymodels.DiffStatementBase(ds.DiffStatementBase),
	}
}
