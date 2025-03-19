package clickhouse

import (
	"database/sql"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
)

type ProfilerStats struct {
	Time     time.Time      `db:"ts"`
	Value    int64          `db:"value"`
	Form     sql.NullString `db:"form"`
	NS       sql.NullString `db:"ns"`
	Hostname sql.NullString `db:"hostname"`
	User     sql.NullString `db:"user"`
	Shard    sql.NullString `db:"shard"`
}

func (ps ProfilerStats) toExt() mongomodels.ProfilerStats {
	m := make(map[string]string)
	if ps.Form.Valid {
		m[string(mongomodels.ProfilerStatsGroupByForm)] = ps.Form.String
	}
	if ps.NS.Valid {
		m[string(mongomodels.ProfilerStatsGroupByNamespace)] = ps.NS.String
	}
	if ps.Hostname.Valid {
		m[string(mongomodels.ProfilerStatsGroupByHostname)] = ps.Hostname.String
	}
	if ps.User.Valid {
		m[string(mongomodels.ProfilerStatsGroupByUser)] = ps.User.String
	}
	if ps.Shard.Valid {
		m[string(mongomodels.ProfilerStatsGroupByShard)] = ps.Shard.String
	}

	v := mongomodels.ProfilerStats{
		Time:       ps.Time,
		Dimensions: m,
		Value:      ps.Value,
	}
	return v
}

type ProfilerRecord struct {
	Time              time.Time `db:"ts"`
	RawRequest        string    `db:"raw"`
	RequestForm       string    `db:"form"`
	Hostname          string    `db:"hostname"`
	User              string    `db:"user"`
	Namespace         string    `db:"ns"`
	Operation         string    `db:"op"`
	Duration          int64     `db:"duration"`
	PlanSummary       string    `db:"plan_summary"`
	ResponseLength    int64     `db:"response_length"`
	KeysExamined      int64     `db:"keys_examined"`
	DocumentsExamined int64     `db:"docs_examined"`
	DocumentsReturned int64     `db:"docs_returned"`
}

func (pr ProfilerRecord) toExt() mongomodels.ProfilerRecs {
	v := mongomodels.ProfilerRecs{
		Time:              pr.Time,
		RawRequest:        pr.RawRequest,
		RequestForm:       pr.RequestForm,
		Hostname:          pr.Hostname,
		User:              pr.User,
		Namespace:         pr.Namespace,
		Operation:         pr.Operation,
		Duration:          pr.Duration,
		PlanSummary:       pr.PlanSummary,
		ResponseLength:    pr.ResponseLength,
		KeysExamined:      pr.KeysExamined,
		DocumentsExamined: pr.DocumentsExamined,
		DocumentsReturned: pr.DocumentsReturned,
	}
	return v
}

type TopForms struct {
	RequestForm       string `db:"form"`
	PlanSummary       string `db:"plan_summary"`
	QueriesCount      int64  `db:"scount"`
	QueriesDuration   int64  `db:"sduration"`
	ResponseLength    int64  `db:"sresponse_length"`
	KeysExamined      int64  `db:"skeys_examined"`
	DocumentsExamined int64  `db:"sdocs_examined"`
	DocumentsReturned int64  `db:"sdocs_returned"`
}

func (tf TopForms) toExt() mongomodels.TopForms {
	v := mongomodels.TopForms{
		RequestForm:                          tf.RequestForm,
		PlanSummary:                          tf.PlanSummary,
		QueriesCount:                         tf.QueriesCount,
		TotalQueriesDuration:                 tf.QueriesDuration,
		AVGQueryDuration:                     0,
		TotalResponseLength:                  tf.ResponseLength,
		AVGResponseLength:                    0,
		TotalKeysExamined:                    tf.KeysExamined,
		TotalDocumentsExamined:               tf.DocumentsExamined,
		TotalDocumentsReturned:               tf.DocumentsReturned,
		KeysExaminedPerDocumentReturned:      0,
		DocumentsExaminedPerDocumentReturned: 0,
	}

	if v.QueriesCount > 0 {
		v.AVGQueryDuration = v.TotalQueriesDuration / v.QueriesCount
		v.AVGResponseLength = v.TotalResponseLength / v.QueriesCount
	}

	if v.TotalDocumentsReturned > 0 {
		v.KeysExaminedPerDocumentReturned = v.TotalKeysExamined / v.TotalDocumentsReturned
		v.DocumentsExaminedPerDocumentReturned = v.TotalDocumentsExamined / v.TotalDocumentsReturned
	}

	return v
}

type PossibleIndexes struct {
	Database     string `db:"database"`
	Collection   string `db:"collection"`
	Index        string `db:"index"`
	RequestCount int64  `db:"count"`
}

func (pi PossibleIndexes) toExt() mongomodels.PossibleIndexes {
	return mongomodels.PossibleIndexes(pi)
}
