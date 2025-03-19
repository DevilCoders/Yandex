package mymodels

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
)

// SessionsStat from perfdiag
type SessionsStats struct {
	// Timestamp of message
	Timestamp        optional.Time
	Dimensions       map[string]string
	SessionsCount    int64
	NextMessageToken int64
}

type MySessionsColumn string

const (
	MySessionsUnspecified    MySessionsColumn = ""
	MySessionsTime           MySessionsColumn = "collect_time"
	MySessionsHost           MySessionsColumn = "host"
	MySessionsDatabase       MySessionsColumn = "database"
	MySessionsUser           MySessionsColumn = "user"
	MySessionsConnID         MySessionsColumn = "conn_id"
	MySessionsThdID          MySessionsColumn = "thd_id"
	MySessionsCommand        MySessionsColumn = "command"
	MySessionsQuery          MySessionsColumn = "query"
	MySessionsDigest         MySessionsColumn = "digest"
	MySessionsQueryLatency   MySessionsColumn = "query_latency"
	MySessionsLockLatency    MySessionsColumn = "lock_latency"
	MySessionsStage          MySessionsColumn = "stage"
	MySessionsStageLatency   MySessionsColumn = "stage_latency"
	MySessionsCurrentWait    MySessionsColumn = "current_wait"
	MySessionsWaitObject     MySessionsColumn = "wait_object"
	MySessionsWaitLatency    MySessionsColumn = "wait_latency"
	MySessionsTrxLatency     MySessionsColumn = "trx_latency"
	MySessionsCurrentMemory  MySessionsColumn = "current_memory"
	MySessionsClientAddr     MySessionsColumn = "client_addr"
	MySessionsClientHostname MySessionsColumn = "client_hostname"
)

type MyStatementsColumn string

const (
	MyStatementsUnspecified         MyStatementsColumn = ""
	MyStatementsTime                MyStatementsColumn = "collect_time"
	MyStatementsHost                MyStatementsColumn = "host"
	MyStatementsDatabase            MyStatementsColumn = "database"
	MyStatementsDigest              MyStatementsColumn = "digest"
	MyStatementsQuery               MyStatementsColumn = "query"
	MyStatementsTotalQueryLatency   MyStatementsColumn = "total_query_latency"
	MyStatementsTotalLockLatency    MyStatementsColumn = "total_lock_latency"
	MyStatementsAvgQueryLatency     MyStatementsColumn = "avg_query_latency"
	MyStatementsAvgLockLatency      MyStatementsColumn = "avg_lock_latency"
	MyStatementsCalls               MyStatementsColumn = "calls"
	MyStatementsErrors              MyStatementsColumn = "errors"
	MyStatementsWarnings            MyStatementsColumn = "warnings"
	MyStatementsRowsExamined        MyStatementsColumn = "rows_examined"
	MyStatementsRowsSent            MyStatementsColumn = "rows_sent"
	MyStatementsRowsAffected        MyStatementsColumn = "rows_affected"
	MyStatementsTmpTables           MyStatementsColumn = "tmp_tables"
	MyStatementsTmpDiskTables       MyStatementsColumn = "tmp_disk_tables"
	MyStatementsSelectFullJoin      MyStatementsColumn = "select_full_join"
	MyStatementsSelectFullRangeJoin MyStatementsColumn = "select_full_range_join"
	MyStatementsSelectRange         MyStatementsColumn = "select_range"
	MyStatementsSelectRangeCheck    MyStatementsColumn = "select_range_check"
	MyStatementsSelectScan          MyStatementsColumn = "select_scan"
	MyStatementsSortMergePasses     MyStatementsColumn = "sort_merge_passes"
	MyStatementsSortRange           MyStatementsColumn = "sort_range"
	MyStatementsSortRows            MyStatementsColumn = "sort_rows"
	MyStatementsSortScan            MyStatementsColumn = "sort_scan"
	MyStatementsNoIndexUsed         MyStatementsColumn = "no_index_used"
	MyStatementsNoGoodIndexUsed     MyStatementsColumn = "no_good_index_used"
)

type SortOrder string

type StatementsStatsGroupBy string

const (
	StatementsStatsGroupByUnspecified StatementsStatsGroupBy = ""
	StatementsStatsGroupByHost        StatementsStatsGroupBy = "host"
	StatementsStatsGroupByDatabase    StatementsStatsGroupBy = "database"
)

const (
	OrderByUnspecified SortOrder = ""
	OrderByAsc         SortOrder = "asc"
	OrderByDesc        SortOrder = "desc"
)

type OrderBy struct {
	Field     MySessionsColumn
	SortOrder SortOrder
}

type OrderBySessionsAtTime struct {
	Field     MySessionsColumn
	SortOrder SortOrder
}

type OrderByStatementsAtTime struct {
	Field     MyStatementsColumn
	SortOrder SortOrder
}

type SessionState struct {
	Timestamp      time.Time
	Host           string
	Database       string
	User           string
	ThdID          int64
	ConnID         int64
	Command        string
	Query          string
	Digest         string
	QueryLatency   float64
	LockLatency    float64
	Stage          string
	StageLatency   float64
	CurrentWait    string
	WaitObject     string
	WaitLatency    float64
	TrxLatency     float64
	CurrentMemory  int64
	ClientAddr     string
	ClientHostname string
	ClientPort     int64
}

type Statements struct {
	Timestamp           time.Time
	Host                string
	Database            string
	Query               string
	Digest              string
	TotalQueryLatency   float64
	TotalLockLatency    float64
	AvgQueryLatency     float64
	AvgLockLatency      float64
	Calls               int64
	Errors              int64
	Warnings            int64
	RowsExamined        int64
	RowsSent            int64
	RowsAffected        int64
	TmpTables           int64
	TmpDiskTables       int64
	SelectFullJoin      int64
	SelectFullRangeJoin int64
	SelectRange         int64
	SelectRangeCheck    int64
	SelectScan          int64
	SortMergePasses     int64
	SortRange           int64
	SortRows            int64
	SortScan            int64
	NoIndexUsed         int64
	NoGoodIndexUsed     int64
}

type Interval struct {
	StartTimestamp optional.Time
	EndTimestamp   optional.Time
}

type DiffStatement struct {
	FirstIntervalTime  Interval
	SecondIntervalTime Interval
	DiffStatementBase
}

type DiffStatementBase struct {
	Host     string
	Database string
	Query    string
	Digest   string

	FirstTotalQueryLatency   float64
	FirstTotalLockLatency    float64
	FirstAvgQueryLatency     float64
	FirstAvgLockLatency      float64
	FirstCalls               int64
	FirstErrors              int64
	FirstWarnings            int64
	FirstRowsExamined        int64
	FirstRowsSent            int64
	FirstRowsAffected        int64
	FirstTmpTables           int64
	FirstTmpDiskTables       int64
	FirstSelectFullJoin      int64
	FirstSelectFullRangeJoin int64
	FirstSelectRange         int64
	FirstSelectRangeCheck    int64
	FirstSelectScan          int64
	FirstSortMergePasses     int64
	FirstSortRange           int64
	FirstSortRows            int64
	FirstSortScan            int64
	FirstNoIndexUsed         int64
	FirstNoGoodIndexUsed     int64

	SecondTotalQueryLatency   float64
	SecondTotalLockLatency    float64
	SecondAvgQueryLatency     float64
	SecondAvgLockLatency      float64
	SecondCalls               int64
	SecondErrors              int64
	SecondWarnings            int64
	SecondRowsExamined        int64
	SecondRowsSent            int64
	SecondRowsAffected        int64
	SecondTmpTables           int64
	SecondTmpDiskTables       int64
	SecondSelectFullJoin      int64
	SecondSelectFullRangeJoin int64
	SecondSelectRange         int64
	SecondSelectRangeCheck    int64
	SecondSelectScan          int64
	SecondSortMergePasses     int64
	SecondSortRange           int64
	SecondSortRows            int64
	SecondSortScan            int64
	SecondNoIndexUsed         int64
	SecondNoGoodIndexUsed     int64

	DiffTotalQueryLatency   float64
	DiffTotalLockLatency    float64
	DiffAvgQueryLatency     float64
	DiffAvgLockLatency      float64
	DiffCalls               float64
	DiffErrors              float64
	DiffWarnings            float64
	DiffRowsExamined        float64
	DiffRowsSent            float64
	DiffRowsAffected        float64
	DiffTmpTables           float64
	DiffTmpDiskTables       float64
	DiffSelectFullJoin      float64
	DiffSelectFullRangeJoin float64
	DiffSelectRange         float64
	DiffSelectRangeCheck    float64
	DiffSelectScan          float64
	DiffSortMergePasses     float64
	DiffSortRange           float64
	DiffSortRows            float64
	DiffSortScan            float64
	DiffNoIndexUsed         float64
	DiffNoGoodIndexUsed     float64
}

// SessionsAtTime from perfdiag
type SessionsAtTime struct {
	Sessions            []SessionState
	PreviousCollectTime time.Time
	NextCollectTime     time.Time
	NextMessageToken    int64
}

// StatementsAtTime from perfdiag
type StatementsAtTime struct {
	Statements          []Statements
	PreviousCollectTime time.Time
	NextCollectTime     time.Time
	NextMessageToken    int64
}

// StatementsInterval from perfdiag
type StatementsInterval struct {
	Statements       []Statements
	NextMessageToken int64
}

// StatementsStats from perfdiag
type StatementsStats struct {
	Statements       []Statements
	NextMessageToken int64
}

type StatementStats struct {
	Query            string
	Statements       []Statements
	NextMessageToken int64
}

type DiffStatements struct {
	DiffStatements   []DiffStatement
	NextMessageToken int64
}

type GetSessionsStatsOptions struct {
	Filter       []sqlfilter.Term
	FromTS       optional.Time
	ToTS         optional.Time
	GroupBy      []MySessionsColumn
	RollupPeriod optional.Int64
	OrderBy      []OrderBy
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetSessionsAtTimeOptions struct {
	ColumnFilter []MySessionsColumn
	Filter       []sqlfilter.Term
	Time         time.Time
	OrderBy      []OrderBySessionsAtTime
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetStatementsAtTimeOptions struct {
	ColumnFilter []MyStatementsColumn
	Filter       []sqlfilter.Term
	Time         time.Time
	OrderBy      []OrderByStatementsAtTime
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetStatementsIntervalOptions struct {
	ColumnFilter []MyStatementsColumn
	Filter       []sqlfilter.Term
	FromTS       optional.Time
	ToTS         optional.Time
	OrderBy      []OrderByStatementsAtTime
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetStatementsStatsOptions struct {
	ColumnFilter []MyStatementsColumn
	Filter       []sqlfilter.Term
	FromTS       optional.Time
	ToTS         optional.Time
	GroupBy      []StatementsStatsGroupBy
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetStatementStatsOptions struct {
	Digest       string
	ColumnFilter []MyStatementsColumn
	Filter       []sqlfilter.Term
	FromTS       optional.Time
	ToTS         optional.Time
	GroupBy      []StatementsStatsGroupBy
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetStatementsDiffOptions struct {
	ColumnFilter        []MyStatementsColumn
	Filter              []sqlfilter.Term
	FirstIntervalStart  time.Time
	SecondIntervalStart time.Time
	IntervalsDuration   optional.Int64
	OrderBy             []OrderByStatementsAtTime
	// PageSize
	Limit int64
	// PageToken
	Offset optional.Int64
}

func (ss GetSessionsStatsOptions) ValidateOrderBy() error {
	groupBy := make(map[string]struct{})
	for _, g := range ss.GroupBy {
		groupBy[string(g)] = struct{}{}
	}
	for _, o := range ss.OrderBy {
		_, ok := groupBy[string(o.Field)]
		if !ok {
			return semerr.InvalidInputf("%q present in order by but not preset in group by", o.Field)
		}
	}
	return nil
}

func (ss GetStatementsDiffOptions) ValidateOrderBy() error {
	if len(ss.ColumnFilter) == 0 {
		return nil
	}
	columnFilter := make(map[MyStatementsColumn]struct{})
	for _, f := range ss.ColumnFilter {
		columnFilter[f] = struct{}{}
	}
	for _, o := range ss.OrderBy {
		_, ok := columnFilter[o.Field]
		if !ok {
			return semerr.InvalidInputf("%q present in order by but not preset in column filter", o.Field)
		}
	}
	return nil
}

func (ss GetSessionsAtTimeOptions) ValidateOrderBy() error {
	if len(ss.ColumnFilter) == 0 {
		return nil
	}
	columnFilter := make(map[MySessionsColumn]struct{})
	for _, f := range ss.ColumnFilter {
		columnFilter[f] = struct{}{}
	}
	for _, o := range ss.OrderBy {
		_, ok := columnFilter[o.Field]
		if !ok {
			return semerr.InvalidInputf("%q present in order by but not preset in column filter", o.Field)
		}
	}
	return nil
}

func (ss GetStatementsAtTimeOptions) ValidateOrderBy() error {
	if len(ss.ColumnFilter) == 0 {
		return nil
	}
	columnFilter := make(map[MyStatementsColumn]struct{})
	for _, f := range ss.ColumnFilter {
		columnFilter[f] = struct{}{}
	}
	for _, o := range ss.OrderBy {
		_, ok := columnFilter[o.Field]
		if !ok {
			return semerr.InvalidInputf("%q present in order by but not preset in column filter", o.Field)
		}
	}
	return nil
}

func (ss GetStatementsIntervalOptions) ValidateOrderBy() error {
	if len(ss.ColumnFilter) == 0 {
		return nil
	}
	columnFilter := make(map[MyStatementsColumn]struct{})
	for _, f := range ss.ColumnFilter {
		columnFilter[f] = struct{}{}
	}
	for _, o := range ss.OrderBy {
		_, ok := columnFilter[o.Field]
		if !ok {
			return semerr.InvalidInputf("%q present in order by but not preset in column filter", o.Field)
		}
	}
	return nil
}
