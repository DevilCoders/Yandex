package pgmodels

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

type PGStatActivityColumn string

const (
	PGStatActivityUnspecified     PGStatActivityColumn = ""
	PGStatActivityTime            PGStatActivityColumn = "collect_time"
	PGStatActivityPid             PGStatActivityColumn = "pid"
	PGStatActivityHost            PGStatActivityColumn = "host"
	PGStatActivityDatabase        PGStatActivityColumn = "database"
	PGStatActivityUser            PGStatActivityColumn = "user"
	PGStatActivityApplicationName PGStatActivityColumn = "application_name"
	PGStatActivityBackendStart    PGStatActivityColumn = "backend_start"
	PGStatActivityXactStart       PGStatActivityColumn = "xact_start"
	PGStatActivityQueryStart      PGStatActivityColumn = "query_start"
	PGStatActivityStateChange     PGStatActivityColumn = "state_change"
	PGStatActivityWaitEventType   PGStatActivityColumn = "wait_event_type"
	PGStatActivityWaitEvent       PGStatActivityColumn = "wait_event"
	PGStatActivityState           PGStatActivityColumn = "state"
	PGStatActivityQuery           PGStatActivityColumn = "query"
	PGStatActivityBackendType     PGStatActivityColumn = "backend_type"
)

type PGStatStatementsColumn string

const (
	PGStatStatementsUnspecified       PGStatStatementsColumn = ""
	PGStatStatementsTime              PGStatStatementsColumn = "collect_time"
	PGStatStatementsHost              PGStatStatementsColumn = "host"
	PGStatStatementsUser              PGStatStatementsColumn = "user"
	PGStatStatementsDatabase          PGStatStatementsColumn = "database"
	PGStatStatementsQueryid           PGStatStatementsColumn = "queryid"
	PGStatStatementsQuery             PGStatStatementsColumn = "query"
	PGStatStatementsCalls             PGStatStatementsColumn = "calls"
	PGStatStatementsTotalTime         PGStatStatementsColumn = "total_time"
	PGStatStatementsMinTime           PGStatStatementsColumn = "min_time"
	PGStatStatementsMaxTime           PGStatStatementsColumn = "max_time"
	PGStatStatementsMeanTime          PGStatStatementsColumn = "mean_time"
	PGStatStatementsStddevTime        PGStatStatementsColumn = "stddev_time"
	PGStatStatementsRows              PGStatStatementsColumn = "rows"
	PGStatStatementsSharedBlksHit     PGStatStatementsColumn = "shared_blks_hit"
	PGStatStatementsSharedBlksRead    PGStatStatementsColumn = "shared_blks_read"
	PGStatStatementsSharedBlksDirtied PGStatStatementsColumn = "shared_blks_dirtied"
	PGStatStatementsSharedBlksWritten PGStatStatementsColumn = "shared_blks_written"
	PGStatStatementsBlkReadTime       PGStatStatementsColumn = "blk_read_time"
	PGStatStatementsBlkWriteTime      PGStatStatementsColumn = "blk_write_time"
	PGStatStatementsTempBlksRead      PGStatStatementsColumn = "temp_blks_read"
	PGStatStatementsTempBlksWritten   PGStatStatementsColumn = "temp_blks_written"
	PGStatStatementsReads             PGStatStatementsColumn = "reads"
	PGStatStatementsWrites            PGStatStatementsColumn = "writes"
	PGStatStatementsUserTime          PGStatStatementsColumn = "user_time"
	PGStatStatementsSystemTime        PGStatStatementsColumn = "system_time"
)

type SortOrder string

type StatementsStatsGroupBy string

const (
	StatementsStatsGroupByUnspecified StatementsStatsGroupBy = ""
	StatementsStatsGroupByHost        StatementsStatsGroupBy = "host"
	StatementsStatsGroupByUser        StatementsStatsGroupBy = "user"
	StatementsStatsGroupByDatabase    StatementsStatsGroupBy = "database"
)

type StatementsStatsField string

const (
	StatementsStatsFieldUnspecified       StatementsStatsField = ""
	StatementsStatsFieldCalls             StatementsStatsField = "calls"
	StatementsStatsFieldTotalTime         StatementsStatsField = "total_time"
	StatementsStatsFieldMinTime           StatementsStatsField = "min_time"
	StatementsStatsFieldMaxTime           StatementsStatsField = "max_time"
	StatementsStatsFieldMeanTime          StatementsStatsField = "mean_time"
	StatementsStatsFieldStddevTime        StatementsStatsField = "stddev_time"
	StatementsStatsFieldRows              StatementsStatsField = "rows"
	StatementsStatsFieldSharedBlksHit     StatementsStatsField = "shared_blks_hit"
	StatementsStatsFieldSharedBlksRead    StatementsStatsField = "shared_blks_read"
	StatementsStatsFieldSharedBlksDirtied StatementsStatsField = "shared_blks_dirtied"
	StatementsStatsFieldSharedBlksWritten StatementsStatsField = "shared_blks_written"
	StatementsStatsFieldBlkReadTime       StatementsStatsField = "blk_read_time"
	StatementsStatsFieldBlkWriteTime      StatementsStatsField = "blk_write_time"
	StatementsStatsFieldTempBlksRead      StatementsStatsField = "temp_blks_read"
	StatementsStatsFieldTempBlksWritten   StatementsStatsField = "temp_blks_written"
	StatementsStatsFieldReads             StatementsStatsField = "reads"
	StatementsStatsFieldWrites            StatementsStatsField = "writes"
	StatementsStatsFieldUserTime          StatementsStatsField = "user_time"
	StatementsStatsFieldSystemTime        StatementsStatsField = "system_time"
)

const (
	OrderByUnspecified SortOrder = ""
	OrderByAsc         SortOrder = "asc"
	OrderByDesc        SortOrder = "desc"
)

type OrderBy struct {
	Field     PGStatActivityColumn
	SortOrder SortOrder
}

type OrderBySessionsAtTime struct {
	Field     PGStatActivityColumn
	SortOrder SortOrder
}

type OrderByStatementsAtTime struct {
	Field     PGStatStatementsColumn
	SortOrder SortOrder
}

type SessionState struct {
	Timestamp       time.Time
	Host            string
	Pid             int64
	Database        string
	User            string
	ApplicationName string
	BackendStart    optional.Time
	XactStart       optional.Time
	QueryStart      optional.Time
	StateChange     optional.Time
	WaitEventType   string
	WaitEvent       string
	State           string
	Query           string
	BackendType     string
}

type Statements struct {
	Timestamp         time.Time
	Host              string
	User              string
	Database          string
	Queryid           string
	Query             string
	Calls             int64
	TotalTime         float64
	MinTime           float64
	MaxTime           float64
	MeanTime          float64
	StddevTime        float64
	Rows              int64
	SharedBlksHit     int64
	SharedBlksRead    int64
	SharedBlksDirtied int64
	SharedBlksWritten int64
	BlkReadTime       float64
	BlkWriteTime      float64
	TempBlksRead      int64
	TempBlksWritten   int64
	Reads             int64
	Writes            int64
	UserTime          float64
	SystemTime        float64
}

type Interval struct {
	StartTimestamp optional.Time
	EndTimestamp   optional.Time
}

type DiffStatement struct {
	FirstIntervalTime       Interval
	SecondIntervalTime      Interval
	Host                    string
	User                    string
	Database                string
	Queryid                 string
	Query                   string
	FirstCalls              int64
	SecondCalls             int64
	FirstTotalTime          float64
	SecondTotalTime         float64
	FirstMinTime            float64
	SecondMinTime           float64
	FirstMaxTime            float64
	SecondMaxTime           float64
	FirstMeanTime           float64
	SecondMeanTime          float64
	FirstStddevTime         float64
	SecondStddevTime        float64
	FirstRows               int64
	SecondRows              int64
	FirstSharedBlksHit      int64
	SecondSharedBlksHit     int64
	FirstSharedBlksRead     int64
	SecondSharedBlksRead    int64
	FirstSharedBlksDirtied  int64
	SecondSharedBlksDirtied int64
	FirstSharedBlksWritten  int64
	SecondSharedBlksWritten int64
	FirstBlkReadTime        float64
	SecondBlkReadTime       float64
	FirstBlkWriteTime       float64
	SecondBlkWriteTime      float64
	FirstTempBlksRead       int64
	SecondTempBlksRead      int64
	FirstTempBlksWritten    int64
	SecondTempBlksWritten   int64
	FirstReads              int64
	SecondReads             int64
	FirstWrites             int64
	SecondWrites            int64
	FirstUserTime           float64
	SecondUserTime          float64
	FirstSystemTime         float64
	SecondSystemTime        float64
	DiffCalls               float64
	DiffTotalTime           float64
	DiffMinTime             float64
	DiffMaxTime             float64
	DiffMeanTime            float64
	DiffStddevTime          float64
	DiffRows                float64
	DiffSharedBlksHit       float64
	DiffSharedBlksRead      float64
	DiffSharedBlksDirtied   float64
	DiffSharedBlksWritten   float64
	DiffBlkReadTime         float64
	DiffBlkWriteTime        float64
	DiffTempBlksRead        float64
	DiffTempBlksWritten     float64
	DiffReads               float64
	DiffWrites              float64
	DiffUserTime            float64
	DiffSystemTime          float64
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

// StatementsInterval from perfdiag
type StatementsStats struct {
	Statements       []Statements
	Query            string
	NextMessageToken int64
}

type StatementStats struct {
	Statements       []Statements
	Query            string
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
	GroupBy      []PGStatActivityColumn
	RollupPeriod optional.Int64
	OrderBy      []OrderBy
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetSessionsAtTimeOptions struct {
	ColumnFilter []PGStatActivityColumn
	Filter       []sqlfilter.Term
	Time         time.Time
	OrderBy      []OrderBySessionsAtTime
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetStatementsAtTimeOptions struct {
	ColumnFilter []PGStatStatementsColumn
	Filter       []sqlfilter.Term
	Time         time.Time
	OrderBy      []OrderByStatementsAtTime
	// Limit is PageSize
	Limit int64
	// Offset is PageToken
	Offset optional.Int64
}

type GetStatementsIntervalOptions struct {
	ColumnFilter []PGStatStatementsColumn
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
	ColumnFilter []StatementsStatsField
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
	Queryid      string
	ColumnFilter []StatementsStatsField
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
	ColumnFilter        []PGStatStatementsColumn
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
	columnFilter := make(map[PGStatStatementsColumn]struct{})
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
	columnFilter := make(map[PGStatActivityColumn]struct{})
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
	columnFilter := make(map[PGStatStatementsColumn]struct{})
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
	columnFilter := make(map[PGStatStatementsColumn]struct{})
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
