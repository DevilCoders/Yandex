package clickhouse

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/sqlbuild"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
)

const (
	filterFieldPrefix = "stats."
	timeColumn        = "toUnixTimestamp(toStartOfInterval(collect_time, toIntervalSecond(:rollup_period))) AS time"
)

var MyStatementsCols = map[interface{}]sqlbuild.ColumnOptions{
	mymodels.MyStatementsTime:                {},
	mymodels.MyStatementsHost:                {Filterable: true},
	mymodels.MyStatementsDatabase:            {Filterable: true},
	mymodels.MyStatementsDigest:              {Filterable: true},
	mymodels.MyStatementsQuery:               {Filterable: true},
	mymodels.MyStatementsTotalQueryLatency:   {AggFunc: "sum"},
	mymodels.MyStatementsTotalLockLatency:    {AggFunc: "sum"},
	mymodels.MyStatementsAvgQueryLatency:     {AggFunc: "avg"},
	mymodels.MyStatementsAvgLockLatency:      {AggFunc: "avg"},
	mymodels.MyStatementsCalls:               {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsErrors:              {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsWarnings:            {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsRowsExamined:        {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsRowsSent:            {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsRowsAffected:        {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsTmpTables:           {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsTmpDiskTables:       {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSelectFullJoin:      {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSelectFullRangeJoin: {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSelectRange:         {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSelectRangeCheck:    {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSelectScan:          {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSortMergePasses:     {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSortRange:           {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSortRows:            {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsSortScan:            {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsNoIndexUsed:         {AggFunc: "sum", Integer: true},
	mymodels.MyStatementsNoGoodIndexUsed:     {AggFunc: "sum", Integer: true},
}

var MySessionsCols = map[interface{}]sqlbuild.ColumnOptions{
	mymodels.MySessionsTime:           {},
	mymodels.MySessionsHost:           {Filterable: true},
	mymodels.MySessionsDatabase:       {Filterable: true},
	mymodels.MySessionsUser:           {Filterable: true},
	mymodels.MySessionsConnID:         {},
	mymodels.MySessionsThdID:          {},
	mymodels.MySessionsCommand:        {Filterable: true},
	mymodels.MySessionsQuery:          {},
	mymodels.MySessionsDigest:         {},
	mymodels.MySessionsQueryLatency:   {},
	mymodels.MySessionsLockLatency:    {},
	mymodels.MySessionsStage:          {Filterable: true},
	mymodels.MySessionsStageLatency:   {},
	mymodels.MySessionsCurrentWait:    {Filterable: true},
	mymodels.MySessionsWaitObject:     {},
	mymodels.MySessionsWaitLatency:    {},
	mymodels.MySessionsTrxLatency:     {},
	mymodels.MySessionsCurrentMemory:  {Integer: true},
	mymodels.MySessionsClientAddr:     {Filterable: true},
	mymodels.MySessionsClientHostname: {Filterable: true},
}

func makeColumnMap(inMap map[interface{}]sqlbuild.ColumnOptions) map[string]sqlbuild.ColumnOptions {
	m := make(map[string]sqlbuild.ColumnOptions)
	for k, v := range inMap {
		switch k := k.(type) {
		case mymodels.MySessionsColumn:
			m[string(k)] = v
		case mymodels.MyStatementsColumn:
			m[string(k)] = v
		}
	}
	return m
}

type queries struct {
	Query   sqlutil.Stmt
	Columns map[string]sqlbuild.ColumnOptions
}

var perDiagQueries = map[string]queries{
	"sessionsStats": {
		Query: sqlutil.Stmt{
			Name: "sessionsStats",
			Query: `
SELECT COUNT() as session_count,
%s
FROM my_sessions
WHERE cluster_id=:cid
AND collect_time >= toDateTime(:from_time)
AND collect_time <=  toDateTime(:to_time)
%s
%s
GROUP BY
%s
ORDER BY
%s
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MySessionsCols),
	},
	"querySessionsAtTime": {
		Query: sqlutil.Stmt{
			Name: "sessionsAtTime",
			Query: `
SELECT
%s
FROM my_sessions
WHERE cluster_id=:cid
AND collect_time = toDateTime(:ts)
%s
ORDER BY
%s
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MySessionsCols),
	},
	"queryFindSessionsClosestDate": {
		Query: sqlutil.Stmt{
			Name: "queryFindClosestDate",
			Query: `
SELECT DISTINCT toUnixTimestamp(collect_time) AS ts
FROM
my_sessions
WHERE cluster_id=:cid AND collect_time>=toDateTime(:ts)
%s
ORDER BY collect_time LIMIT 2
UNION ALL
SELECT DISTINCT toUnixTimestamp(collect_time) AS ts
FROM
my_sessions
WHERE cluster_id=:cid AND collect_time<=toDateTime(:ts)
%s
ORDER BY collect_time DESC LIMIT 2`},
		Columns: makeColumnMap(MySessionsCols),
	},
	"queryStatementsAtTime": {
		Query: sqlutil.Stmt{
			Name: "statementsAtTime",
			Query: `
SELECT
%s
FROM my_statements
WHERE cluster_id=:cid
AND collect_time = toDateTime(:ts)
%s
ORDER BY
%s
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MyStatementsCols),
	},
	"queryStatementsInterval": {
		Query: sqlutil.Stmt{
			Name: "statementsInterval",
			Query: `
SELECT
%s
FROM my_statements
WHERE cluster_id=:cid
AND collect_time >= toDateTime(:from_time)
AND collect_time <= toDateTime(:to_time)
%s
GROUP BY cluster_id, host, database, digest, query
ORDER BY
%s
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MyStatementsCols),
	},
	"queryFindStatementsClosestDate": {
		Query: sqlutil.Stmt{
			Name: "queryFindStatementsClosestDate",
			Query: `
SELECT DISTINCT toUnixTimestamp(collect_time) AS ts
FROM
my_statements
WHERE cluster_id=:cid AND collect_time>=toDateTime(:ts)
%s
ORDER BY collect_time LIMIT 2
UNION ALL
SELECT DISTINCT toUnixTimestamp(collect_time) AS ts
FROM
my_statements
WHERE cluster_id=:cid AND collect_time<=toDateTime(:ts)
%s
ORDER BY collect_time DESC LIMIT 2`},
		Columns: makeColumnMap(MyStatementsCols),
	},
	"subQueryStatementsDiff": {
		Query: sqlutil.Stmt{
			Name: "StatementsDiffSubQuery",
			Query: `
SELECT
	host,
	database,
	digest,
	query,
	%s
FROM my_statements
WHERE cluster_id=:cid
AND (collect_time >= toDateTime(:%s_ts_start)) AND (collect_time <= toDateTime(:%s_ts_end))
%s
GROUP BY host, database, digest, query`},
		Columns: makeColumnMap(MyStatementsCols),
	},
	"queryStatementsDiff": {
		Query: sqlutil.Stmt{
			Name: "StatementsDiff",
			Query: `
SELECT %s FROM (SELECT
*
FROM
(
%s
) first
FULL OUTER JOIN
(
%s
) second USING (host, database, digest)) m
ORDER BY %s
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MyStatementsCols),
	},
	"queryStatementsStats": {
		Query: sqlutil.Stmt{
			Name: "statementsStats",
			Query: `
SELECT
    toDateTime(t) as collect_time,
%s,
%s
FROM
(
    SELECT
        (intDiv(toUInt32(collect_time), 10) * 10) as t,
       %s,
       %s
    FROM perf_diag.my_statements
    WHERE
		(cluster_id=:cid
		AND collect_time >= toDateTime(:from_time)
		AND collect_time <= toDateTime(:to_time))
		%s
    GROUP BY
		t,
		%s
    ORDER BY t
)
GROUP BY collect_time, %s
ORDER BY collect_time
LIMIT :offset, :limit`},
		Columns: makeColumnMap(MyStatementsCols),
	},
	"queryStatementStats": {
		Query: sqlutil.Stmt{
			Name: "statementStats",
			Query: `
    SELECT
        toDateTime((intDiv(toUInt32(collect_time), 10) * 10)) as collect_time,
		digest,
		any(query) as query,
        %s
        %s
    FROM perf_diag.my_statements
    WHERE
		(cluster_id=:cid
        AND digest = :digest
		AND perf_diag.my_statements.collect_time >= toDateTime(:from_time)
		AND perf_diag.my_statements.collect_time <= toDateTime(:to_time))
		%s
    GROUP BY
        digest,
		collect_time
		%s
    ORDER BY collect_time %s
	LIMIT :offset, :limit`},
		Columns: makeColumnMap(MyStatementsCols),
	},
}

func BuildSessionsStatsQuery(conditions []sqlfilter.Term, groupBy []mymodels.MySessionsColumn, orderBy []mymodels.OrderBy, rollup int64) (sqlutil.Stmt, map[string]interface{}, error) {
	columnExp := make([]string, 0, len(groupBy))
	gBy := make([]string, 0, len(groupBy))
	gByCond := ""
	for _, r := range groupBy {
		if r == mymodels.MySessionsTime {
			columnExp = append(columnExp, timeColumn)
			gBy = append(gBy, "time")
		} else if r == mymodels.MySessionsDigest {
			columnExp = append(columnExp, fmt.Sprintf("any(%s) as %s", mymodels.MySessionsQuery, mymodels.MySessionsQuery))
			columnExp = append(columnExp, string(r))
			gBy = append(gBy, string(r))
		} else {
			columnExp = append(columnExp, string(r))
			gBy = append(gBy, string(r))
		}
		gByCond += fmt.Sprintf(" AND %s IS NOT NULL ", string(r))
	}

	oBy := make([]string, 0, len(orderBy))
	for _, r := range orderBy {
		if r.Field == mymodels.MySessionsTime {
			oBy = append(oBy, "time "+string(r.SortOrder))
		} else {
			oBy = append(oBy, string(r.Field)+" "+string(r.SortOrder))
		}
	}

	columnExpSting := strings.Join(columnExp, " , ")
	groupByString := strings.Join(gBy, " , ")
	orderByString := strings.Join(oBy, " , ")
	if orderByString == "" {
		orderByString = "1 ASC"
	}
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries["sessionsStats"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perDiagQueries["sessionsStats"].Query.Format(columnExpSting, whereCond, gByCond, groupByString, orderByString), whereParams, nil
}

func BuildStatementsStatsQuery(conditions []sqlfilter.Term, columnFilter []mymodels.MyStatementsColumn, groupBy []mymodels.StatementsStatsGroupBy) (sqlutil.Stmt, map[string]interface{}, error) {
	gBy := make([]string, 0, len(groupBy))
	for _, r := range groupBy {
		gBy = append(gBy, string(r))
	}

	cf := make([]string, 0, len(columnFilter))
	for _, column := range columnFilter {
		cf = append(cf, string(column))
	}

	subColumns, err := filterColumnsSubStatsStatements(perDiagQueries["queryStatementsStats"].Columns, cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	subColumnsString := strings.Join(subColumns, ",")
	Columns, err := filterColumnsMainStatsStatements(perDiagQueries["queryStatementsStats"].Columns, cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	ColumnsString := strings.Join(Columns, ",")
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries["queryStatementsStats"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	groupByString := strings.Join(gBy, " , ")
	return perDiagQueries["queryStatementsStats"].Query.Format(ColumnsString, groupByString, subColumnsString, groupByString, whereCond, groupByString, groupByString), whereParams, nil
}

func BuildStatementStatsQuery(conditions []sqlfilter.Term, columnFilter []mymodels.MyStatementsColumn, groupBy []mymodels.StatementsStatsGroupBy) (sqlutil.Stmt, map[string]interface{}, error) {
	gBy := make([]string, 0, len(groupBy))
	for _, r := range groupBy {
		gBy = append(gBy, string(r))
	}

	cf := make([]string, 0, len(columnFilter))
	for _, column := range columnFilter {
		cf = append(cf, string(column))
	}

	columns, err := filterColumnsStatementStats(perDiagQueries["queryStatementStats"].Columns, cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	columnsString := strings.Join(columns, ",")
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries["queryStatementStats"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	groupByString := strings.Join(gBy, " , ")
	if groupByString != "" {
		groupByString = ", " + groupByString
	}
	return perDiagQueries["queryStatementStats"].Query.Format(columnsString, groupByString, whereCond, groupByString, groupByString), whereParams, nil
}

func BuildSessionsAtTimeQuery(conditions []sqlfilter.Term, columnFilter []mymodels.MySessionsColumn, orderBy []mymodels.OrderBySessionsAtTime) (sqlutil.Stmt, map[string]interface{}, error) {
	oBy := make([]string, 0, len(orderBy))
	for _, r := range orderBy {
		oBy = append(oBy, string(r.Field)+" "+string(r.SortOrder))
	}

	cf := make([]string, 0, len(columnFilter))
	for _, column := range columnFilter {
		cf = append(cf, string(column))
	}

	columns, err := filterColumnsSessionsAtTime(perDiagQueries["querySessionsAtTime"].Columns, cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	columnsString := strings.Join(columns, ",")
	orderByString := strings.Join(oBy, " , ")
	if orderByString == "" {
		orderByString = "collect_time ASC"
	}
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries["querySessionsAtTime"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perDiagQueries["querySessionsAtTime"].Query.Format(columnsString, whereCond, orderByString), whereParams, nil
}

func BuildStatementsAtTimeQuery(conditions []sqlfilter.Term, columnFilter []mymodels.MyStatementsColumn, orderBy []mymodels.OrderByStatementsAtTime) (sqlutil.Stmt, map[string]interface{}, error) {
	oBy := make([]string, 0, len(orderBy))
	for _, r := range orderBy {
		oBy = append(oBy, string(r.Field)+" "+string(r.SortOrder))
	}

	cf := make([]string, 0, len(columnFilter))
	for _, column := range columnFilter {
		cf = append(cf, string(column))
	}

	columns, err := sqlbuild.FilterQueryColumns(perDiagQueries["queryStatementsAtTime"].Columns, cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	columnsString := sqlbuild.MapJoin(columns, ",")
	orderByString := strings.Join(oBy, " , ")
	if orderByString == "" {
		orderByString = "collect_time ASC"
	}
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries["queryStatementsAtTime"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perDiagQueries["queryStatementsAtTime"].Query.Format(columnsString, whereCond, orderByString), whereParams, nil
}

func BuildStatementsDiffQuery(conditions []sqlfilter.Term, columnFilter []mymodels.MyStatementsColumn, orderBy []mymodels.OrderByStatementsAtTime) (sqlutil.Stmt, map[string]interface{}, error) {
	oByFirst := make([]string, 0, len(orderBy))
	for _, r := range orderBy {
		if r.Field == mymodels.MyStatementsTime {
			oByFirst = append(oByFirst, "first.start_"+string(r.Field)+" "+string(r.SortOrder))
			oByFirst = append(oByFirst, "first.end_"+string(r.Field)+" "+string(r.SortOrder))
		} else if perDiagQueries["subQueryStatementsDiff"].Columns[string(r.Field)].AggFunc == "" {
			oByFirst = append(oByFirst, string(r.Field)+" "+string(r.SortOrder))
		} else {
			oByFirst = append(oByFirst, "first_"+string(r.Field)+" "+string(r.SortOrder))
		}
	}

	cf := make([]string, 0, len(columnFilter))
	for _, column := range columnFilter {
		cf = append(cf, string(column))
	}

	firstColumns, err := filterColumnsSubDiffStatements(perDiagQueries["subQueryStatementsDiff"].Columns, "first", cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	secondColumns, err := filterColumnsSubDiffStatements(perDiagQueries["subQueryStatementsDiff"].Columns, "second", cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	selectColumns, err := filterColumnsMainDiffStatements(perDiagQueries["subQueryStatementsDiff"].Columns, cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	firstColumnsString := strings.Join(firstColumns, ",")
	secondColumnsString := strings.Join(secondColumns, ",")
	selectColumnsString := strings.Join(selectColumns, ",")

	FirstOrderByString := strings.Join(oByFirst, " , ")
	if FirstOrderByString == "" {
		FirstOrderByString = "1 ASC"
	}
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries["subQueryStatementsDiff"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	firstSubQuery := perDiagQueries["subQueryStatementsDiff"].Query.Format(firstColumnsString, "first", "first", whereCond)
	secondSubQuery := perDiagQueries["subQueryStatementsDiff"].Query.Format(secondColumnsString, "second", "second", whereCond)
	return perDiagQueries["queryStatementsDiff"].Query.Format(selectColumnsString, firstSubQuery.Query, secondSubQuery.Query, FirstOrderByString), whereParams, nil
}

func BuildFindClosestDatesQuery(queryType string, conditions []sqlfilter.Term) (sqlutil.Stmt, map[string]interface{}, error) {
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries[queryType].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perDiagQueries[queryType].Query.Format(whereCond, whereCond), whereParams, nil
}

func filterColumnsSubDiffStatements(columns map[string]sqlbuild.ColumnOptions, prefix string, filter []string) ([]string, error) {
	col, err := sqlbuild.FilterQueryColumns(columns, filter)
	if err != nil {
		return nil, err
	}
	ret := make([]string, 0, len(col))
	for k, v := range col {
		if v.AggFunc != "" {
			ret = append(ret, fmt.Sprintf("%s(%s) as %s_%s", v.AggFunc, k, prefix, k))
		} else if k == string(mymodels.MyStatementsTime) {
			ret = append(ret,
				fmt.Sprintf(`toNullable(min(%s)) as "%s.start_%s"`, k, prefix, k),
				fmt.Sprintf(`toNullable(max(%s)) as "%s.end_%s"`, k, prefix, k),
			)
		} else if k == string(mymodels.MyStatementsQuery) {
			continue
		} else {
			ret = append(ret, k)
		}
	}
	return ret, nil
}

func BuildStatementsIntervalQuery(conditions []sqlfilter.Term, columnFilter []mymodels.MyStatementsColumn, orderBy []mymodels.OrderByStatementsAtTime) (sqlutil.Stmt, map[string]interface{}, error) {
	oBy := make([]string, 0, len(orderBy))
	for _, r := range orderBy {
		oBy = append(oBy, string(r.Field)+" "+string(r.SortOrder))
	}

	cf := make([]string, 0, len(columnFilter))
	for _, column := range columnFilter {
		cf = append(cf, string(column))
	}

	columns, err := filterColumnsIntervalStatements(perDiagQueries["queryStatementsInterval"].Columns, cf)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}
	columnsString := strings.Join(columns, ",")
	orderByString := strings.Join(oBy, " , ")
	if orderByString == "" {
		orderByString = "collect_time_max ASC"
	}
	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(perDiagQueries["queryStatementsInterval"].Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	return perDiagQueries["queryStatementsInterval"].Query.Format(columnsString, whereCond, orderByString), whereParams, nil
}

func filterColumnsSessionsAtTime(columns map[string]sqlbuild.ColumnOptions, filter []string) ([]string, error) {
	col, err := sqlbuild.FilterQueryColumns(columns, filter)
	if err != nil {
		return nil, err
	}
	ret := make([]string, 0, len(col))
	for k := range col {
		ret = append(ret, fmt.Sprintf("assumeNotNull(%s) as %s", k, k))
	}
	return ret, nil
}

func filterColumnsIntervalStatements(columns map[string]sqlbuild.ColumnOptions, filter []string) ([]string, error) {
	col, err := sqlbuild.FilterQueryColumns(columns, filter)
	if err != nil {
		return nil, err
	}
	ret := make([]string, 0, len(col))
	for k, v := range col {
		if v.AggFunc != "" {
			ret = append(ret, fmt.Sprintf("round(%s(%s), 3) as %s", v.AggFunc, k, k))
		} else if k == string(mymodels.MyStatementsTime) {
			ret = append(ret, fmt.Sprintf(`max(%s) as %s_max`, k, k))
		} else {
			ret = append(ret, k)
		}
	}
	return ret, nil
}

func filterColumnsMainDiffStatements(columns map[string]sqlbuild.ColumnOptions, filter []string) ([]string, error) {
	col, err := sqlbuild.FilterQueryColumns(columns, filter)
	if err != nil {
		return nil, err
	}
	ret := make([]string, 0, len(col))
	for k, v := range col {
		if v.AggFunc != "" {
			ret = append(ret,
				fmt.Sprintf("coalesce(first_%s,0) as first_%s", k, k),
				fmt.Sprintf("coalesce(second_%s,0) as second_%s", k, k),
				fmt.Sprintf("multiIf(first_%s = 0, 0, trunc((second_%s / first_%s) - 1, 5) * 100) AS diff_%s", k, k, k, k),
			)
		} else if k == string(mymodels.MyStatementsTime) {
			ret = append(ret,
				`"first.start_collect_time"`,
				`"first.end_collect_time"`,
				`"second.start_collect_time"`,
				`"second.end_collect_time"`,
			)
		} else if k == string(mymodels.MyStatementsQuery) {
			ret = append(ret, fmt.Sprintf("coalesce(%s, second.%s) as %s", k, k, k))
		} else {
			ret = append(ret, k)
		}
	}
	return ret, nil
}

func filterColumnsMainStatsStatements(columns map[string]sqlbuild.ColumnOptions, filter []string) ([]string, error) {
	col, err := sqlbuild.FilterQueryColumns(columns, filter)
	if err != nil {
		return nil, err
	}
	ret := make([]string, 0, len(col))
	for k, v := range col {
		if v.AggFunc != "" {
			s := ""
			if v.Integer {
				s = fmt.Sprintf("toInt64(%s(%s)) as %s", v.AggFunc, k, k)
			} else {
				s = fmt.Sprintf("round(%s(%s), 3) as %s", v.AggFunc, k, k)
			}
			ret = append(ret, s)
		}
	}
	return ret, nil
}

func filterColumnsSubStatsStatements(columns map[string]sqlbuild.ColumnOptions, filter []string) ([]string, error) {
	col, err := sqlbuild.FilterQueryColumns(columns, filter)
	if err != nil {
		return nil, err
	}
	ret := make([]string, 0, len(col))
	for k, v := range col {
		if v.AggFunc != "" {
			ret = append(ret, fmt.Sprintf("avg(%s) as %s", k, k))
		}
	}
	return ret, nil
}

func filterColumnsStatementStats(columns map[string]sqlbuild.ColumnOptions, filter []string) ([]string, error) {
	col, err := sqlbuild.FilterQueryColumns(columns, filter)
	if err != nil {
		return nil, err
	}
	ret := make([]string, 0, len(col))
	for k, v := range col {
		if v.AggFunc != "" {
			s := ""
			if v.Integer {
				s = fmt.Sprintf("toInt64(avg(%s)) as %s", k, k)
			} else {
				s = fmt.Sprintf("round(avg(%s), 3) as %s", k, k)
			}
			ret = append(ret, s)
		}
	}
	return ret, nil
}
