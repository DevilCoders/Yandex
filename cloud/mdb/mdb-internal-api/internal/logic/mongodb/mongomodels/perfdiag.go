package mongomodels

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
)

type ProfilerStatsColumn string

const (
	ProfilerStatsColumnTS             ProfilerStatsColumn = "ts"
	ProfilerStatsColumnShard          ProfilerStatsColumn = "shard"
	ProfilerStatsColumnHostname       ProfilerStatsColumn = "hostname"
	ProfilerStatsColumnUser           ProfilerStatsColumn = "user"
	ProfilerStatsColumnNS             ProfilerStatsColumn = "ns"
	ProfilerStatsColumnDatabase       ProfilerStatsColumn = "database"
	ProfilerStatsColumnCollection     ProfilerStatsColumn = "collection"
	ProfilerStatsColumnOperation      ProfilerStatsColumn = "op"
	ProfilerStatsColumnForm           ProfilerStatsColumn = "form"
	ProfilerStatsColumnDuration       ProfilerStatsColumn = "duration"
	ProfilerStatsColumnPlanSummary    ProfilerStatsColumn = "plan_summary"
	ProfilerStatsColumnResponseLength ProfilerStatsColumn = "response_length"
	ProfilerStatsColumnKeysExamined   ProfilerStatsColumn = "keys_examined"
	ProfilerStatsColumnDocsExamined   ProfilerStatsColumn = "docs_examined"
	ProfilerStatsColumnDocsReturned   ProfilerStatsColumn = "docs_returned"
	ProfilerStatsColumnCount          ProfilerStatsColumn = "count"
	ProfilerStatsColumnKeysPerDoc     ProfilerStatsColumn = "keys_examined_per_docs_returned"
	ProfilerStatsColumnDocsPerDoc     ProfilerStatsColumn = "docs_examined_per_docs_returned"
	ProfilerStatsColumnRawRequest     ProfilerStatsColumn = "raw"
)

type AggregationType string

const (
	AggregationTypeUnspecified AggregationType = ""
	AggregationTypeSum         AggregationType = "sum"
	AggregationTypeAvg         AggregationType = "avg"
)

type ProfilerStatsGroupBy string

const (
	ProfilerStatsGroupByForm      ProfilerStatsGroupBy = "form"
	ProfilerStatsGroupByNS        ProfilerStatsGroupBy = "ns"
	ProfilerStatsGroupByHostname  ProfilerStatsGroupBy = "hostname"
	ProfilerStatsGroupByUser      ProfilerStatsGroupBy = "user"
	ProfilerStatsGroupByShard     ProfilerStatsGroupBy = "shard"
	ProfilerStatsGroupByNamespace ProfilerStatsGroupBy = "namespace"
)

type GetProfilerStatsOptions struct {
	Filter              []sqlfilter.Term
	FromTS              time.Time
	ToTS                time.Time
	AggregateBy         ProfilerStatsColumn
	AggregationFunction AggregationType
	GroupBy             []ProfilerStatsGroupBy
	RollupPeriod        optional.Int64
	Limit               int64          // Limit is PageSize
	Offset              optional.Int64 // Offset is PageToken
	TopX                optional.Int64
}

type ProfilerStats struct {
	Time       time.Time
	Dimensions map[string]string
	Value      int64
}

type GetProfilerRecsAtTimeOptions struct {
	FromTS      time.Time
	ToTS        time.Time
	RequestForm string
	Hostname    string
	Limit       int64          // Limit is PageSize
	Offset      optional.Int64 // Offset is PageToken
}

type ProfilerRecs struct {
	Time              time.Time
	RawRequest        string
	RequestForm       string
	Hostname          string
	User              string
	Namespace         string
	Operation         string
	Duration          int64
	PlanSummary       string
	ResponseLength    int64
	KeysExamined      int64
	DocumentsExamined int64
	DocumentsReturned int64
}

type GetProfilerTopFormsByStatOptions struct {
	Filter              []sqlfilter.Term
	FromTS              time.Time
	ToTS                time.Time
	AggregateBy         ProfilerStatsColumn
	AggregationFunction AggregationType
	Limit               int64          // Limit is PageSize
	Offset              optional.Int64 // Offset is PageToken
}

type TopForms struct {
	RequestForm                          string
	PlanSummary                          string
	QueriesCount                         int64
	TotalQueriesDuration                 int64
	AVGQueryDuration                     int64
	TotalResponseLength                  int64
	AVGResponseLength                    int64
	TotalKeysExamined                    int64
	TotalDocumentsExamined               int64
	TotalDocumentsReturned               int64
	KeysExaminedPerDocumentReturned      int64
	DocumentsExaminedPerDocumentReturned int64
}

type GetPossibleIndexesOptions struct {
	Filter []sqlfilter.Term
	FromTS time.Time
	ToTS   time.Time
	Limit  int64          // Limit is PageSize
	Offset optional.Int64 // Offset is PageToken
}

type PossibleIndexes struct {
	Database     string
	Collection   string
	Index        string
	RequestCount int64
}

type ValidOperation string
