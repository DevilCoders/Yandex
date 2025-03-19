package clickhouse

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/sqlbuild"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
)

const filterFieldPrefix string = ""

var MongoProfilerColumns = map[interface{}]sqlbuild.ColumnOptions{
	mongomodels.ProfilerStatsColumnTS:             {Filterable: false, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnShard:          {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnHostname:       {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnUser:           {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnNS:             {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnDatabase:       {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnCollection:     {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnOperation:      {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnForm:           {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnDuration:       {Filterable: false, AggFunc: "avg", Integer: true},
	mongomodels.ProfilerStatsColumnPlanSummary:    {Filterable: true, AggFunc: "", Integer: false},
	mongomodels.ProfilerStatsColumnResponseLength: {Filterable: false, AggFunc: "avg", Integer: true},
	mongomodels.ProfilerStatsColumnKeysExamined:   {Filterable: false, AggFunc: "sum", Integer: true},
	mongomodels.ProfilerStatsColumnDocsExamined:   {Filterable: false, AggFunc: "sum", Integer: true},
	mongomodels.ProfilerStatsColumnDocsReturned:   {Filterable: false, AggFunc: "sum", Integer: true},
	mongomodels.ProfilerStatsColumnCount:          {Filterable: false, AggFunc: "sum", Integer: true},
	mongomodels.ProfilerStatsColumnKeysPerDoc:     {Filterable: false, AggFunc: "sum", Integer: true},
	mongomodels.ProfilerStatsColumnDocsPerDoc:     {Filterable: false, AggFunc: "sum", Integer: true},
	mongomodels.ProfilerStatsColumnRawRequest:     {Filterable: false, AggFunc: "", Integer: false},
}

func makeColumnMap(inMap map[interface{}]sqlbuild.ColumnOptions) map[string]sqlbuild.ColumnOptions {
	m := make(map[string]sqlbuild.ColumnOptions)
	for k, v := range inMap {
		switch k := k.(type) {
		case mongomodels.ProfilerStatsColumn:
			m[string(k)] = v
		}
	}
	return m
}

type queries struct {
	Query   sqlutil.Stmt
	Columns map[string]sqlbuild.ColumnOptions
}

var perfDiagQueries = map[string]queries{
	"queryProfilerStats": {
		Query: sqlutil.Stmt{
			Name: "profilerStats",
			Query: `
SELECT
	toStartOfInterval(ts, toIntervalSecond(:rollup_period)) as ts,
    if(%s in (:top_list), %s, 'Other') as %s
    %s,
	ROUND(%s(%s)) as value
FROM perfdiag.mongodb_profiler_view
WHERE
    cluster_id = :cid
    AND ts >= toDateTime(:from_time)
    AND ts <=  toDateTime(:to_time)
    %s
GROUP BY
	ts, %s %s
ORDER BY
	ts
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MongoProfilerColumns),
	},
	"queryProfilerTopX": {
		Query: sqlutil.Stmt{
			Name: "profilerTopX",
			Query: `
SELECT
	%s as key
FROM perfdiag.mongodb_profiler_view
WHERE
	cluster_id = :cid
	AND ts >= toDateTime(:from_time)
	AND ts <=  toDateTime(:to_time)
	%s
GROUP BY
	%s
ORDER BY
	%s(%s) DESC
LIMIT :limit`},
		Columns: makeColumnMap(MongoProfilerColumns),
	},
	"queryProfilerRecords": {
		Query: sqlutil.Stmt{
			Name: "profilerRecords",
			Query: `
SELECT
	ts,
	raw,
	form,
	hostname,
	user,
	ns,
	op,
	duration,
	plan_summary,
	response_length,
	keys_examined,
	docs_examined,
	docs_returned
FROM perfdiag.mongodb_profiler
WHERE
	cluster_id=:cid
	AND ts >= toDateTime(:from_time)
	AND ts <=  toDateTime(:to_time)
	AND form = :request_form
    AND('' = :hostname OR trim(TRAILING '::012789' from hostname) = :hostname)
ORDER BY ts
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MongoProfilerColumns),
	},
	"queryTopForms": {
		Query: sqlutil.Stmt{
			Name: "topForms",
			Query: `
SELECT
	form,
	plan_summary,
	COUNT() as scount,
	SUM(duration) as sduration,
	SUM(response_length) as sresponse_length,
	SUM(keys_examined) as skeys_examined,
	SUM(docs_examined) as sdocs_examined,
	SUM(docs_returned) as sdocs_returned
FROM perfdiag.mongodb_profiler_view
WHERE
	cluster_id=:cid
	AND ts >= toDateTime(:from_time)
	AND ts <=  toDateTime(:to_time)
	%s
GROUP BY
	form,
	plan_summary
ORDER BY 
	%s(%s) DESC,
	form,
	plan_summary
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MongoProfilerColumns),
	},
	"queryPossibleIndexes": {
		Query: sqlutil.Stmt{
			Name: "possibleIndexes",
			Query: `
SELECT
	database,
	collection,
	trimBoth(visitParamExtractRaw(form, 'filter')) AS index,
	SUM(count) as count
FROM perfdiag.mongodb_profiler_view
WHERE
	cluster_id = :cid
	AND ts >= toDateTime(:from_time)
	AND ts <= toDateTime(:to_time)
	AND plan_summary = 'COLLSCAN'
    AND op = 'query'
    AND index != '{}'
	%s
GROUP BY
    form,
    database,
    collection
ORDER BY
	form
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MongoProfilerColumns),
	},
}

func BuildProfilerStatsQuery(conditions []sqlfilter.Term, aggregateBy mongomodels.ProfilerStatsColumn, aggregationFunction mongomodels.AggregationType, groupBy []mongomodels.ProfilerStatsGroupBy) (sqlutil.Stmt, map[string]interface{}, error) {

	gBy := make([]string, 0, len(groupBy))
	var groupByString0 string
	for i, r := range groupBy {
		if i == 0 {
			groupByString0 = string(r)
			gBy = append(gBy, "")
		} else {
			gBy = append(gBy, string(r))
		}
	}

	groupByStringRest := strings.Join(gBy, " , ")

	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perfDiagQueries["queryProfilerStats"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perfDiagQueries["queryProfilerStats"].Query.Format(
		groupByString0, // if ( %s in (:top_list),
		groupByString0, // %s, 'Other')
		groupByString0, // as %s
		groupByStringRest,
		string(aggregationFunction),
		string(aggregateBy),
		whereCond,
		groupByString0,
		groupByStringRest,
	), whereParams, nil
}

func BuildProfilerTopXQuery(conditions []sqlfilter.Term, aggregateBy mongomodels.ProfilerStatsColumn, aggregationFunction mongomodels.AggregationType, groupBy []mongomodels.ProfilerStatsGroupBy) (sqlutil.Stmt, map[string]interface{}, error) {
	groupByString := string(groupBy[0])

	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perfDiagQueries["queryProfilerStats"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perfDiagQueries["queryProfilerTopX"].Query.Format(
		groupByString,
		whereCond,
		groupByString,
		string(aggregationFunction),
		string(aggregateBy),
	), whereParams, nil
}

func BuildProfilerRecordsQuery() (sqlutil.Stmt, map[string]interface{}, error) {
	return perfDiagQueries["queryProfilerRecords"].Query, map[string]interface{}{}, nil
}

func BuildTopFormsQuery(conditions []sqlfilter.Term, aggregateBy mongomodels.ProfilerStatsColumn, aggregationFunction mongomodels.AggregationType) (sqlutil.Stmt, map[string]interface{}, error) {
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perfDiagQueries["queryTopForms"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perfDiagQueries["queryTopForms"].Query.Format(
		whereCond,
		string(aggregationFunction),
		string(aggregateBy),
	), whereParams, nil
}

func BuildPossibleIndexesQuery(conditions []sqlfilter.Term) (sqlutil.Stmt, map[string]interface{}, error) {
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perfDiagQueries["queryPossibleIndexes"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perfDiagQueries["queryPossibleIndexes"].Query.Format(
		whereCond,
	), whereParams, nil
}
