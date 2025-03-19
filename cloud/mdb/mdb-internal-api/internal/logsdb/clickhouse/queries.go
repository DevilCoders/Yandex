package clickhouse

import (
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/sqlbuild"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	ParamSeconds      = "log_seconds"
	ParamMilliseconds = "log_milliseconds"
	ParamMessage      = "message"
	ParamHostname     = "hostname"
	filterFieldPrefix = "message."

	selectLogsCommonColumns = "toUnixTimestamp(%[1]s) AS " + ParamSeconds + ", ms AS " + ParamMilliseconds + ", "

	selectLogsWithMillisecondsCondition = " WHERE cluster = :cid " +
		"AND (%[1]s > toDateTime(:from_time) OR (%[1]s = toDateTime(:from_time) AND %[4]s >= :from_ms) ) " +
		"AND (%[1]s < toDateTime(:to_time) OR (%[1]s = toDateTime(:to_time) AND %[4]s < :to_ms) )"
	selectLogsCommonOrder       = "ORDER BY %[1]s ASC, ms ASC "
	selectLogsMongoDBOrder      = "ORDER BY origin ASC, %[1]s ASC, ms ASC "
	selectLogsMongoDBAuditOrder = "ORDER BY %[1]s ASC, ms ASC, origin ASC "
	selectLogsCommonLimit       = "LIMIT :offset, :limit"
)

type logTypeQuery struct {
	Query                sqlutil.Stmt
	Columns              map[string]sqlbuild.ColumnOptions
	QueryFormatArguments []interface{}
}

func queryTemplateForMongoDBTable(table string) string {
	return "SELECT " + selectLogsCommonColumns + "%[2]s FROM " + table + selectLogsWithMillisecondsCondition + " %[3]s " + selectLogsMongoDBOrder + selectLogsCommonLimit
}

func queryTemplateForMongoDBAuditTable(table string) string {
	return "SELECT " + selectLogsCommonColumns + "%[2]s FROM " + table + selectLogsWithMillisecondsCondition + " %[3]s " + selectLogsMongoDBAuditOrder + selectLogsCommonLimit
}

func queryTemplateForTableWithMs(table string) string {
	return "SELECT " + selectLogsCommonColumns + "%[2]s FROM " + table + selectLogsWithMillisecondsCondition + " %[3]s " + selectLogsCommonOrder + selectLogsCommonLimit
}

var logTypeQueries = map[logsdb.LogType]logTypeQuery{
	logsdb.LogTypeClickHouse: {
		Query: sqlutil.Stmt{
			Name:  "SelectClickHouseLogs",
			Query: queryTemplateForTableWithMs("mdb.clickhouse"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			"severity":    {Filterable: true},
			ParamHostname: {Filterable: true},
			"component":   {},
			ParamMessage:  {},
			"thread":      {},
			"query_id":    {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMongoD: {
		Query: sqlutil.Stmt{
			Name:  "SelectMongoDLogs",
			Query: queryTemplateForMongoDBTable("mdb.mongod"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"severity":    {Filterable: true},
			"component":   {},
			"context":     {},
			ParamMessage:  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMongoS: {
		Query: sqlutil.Stmt{
			Name:  "SelectMongoSLogs",
			Query: queryTemplateForMongoDBTable("mdb.mongos"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"severity":    {Filterable: true},
			"component":   {},
			"context":     {},
			ParamMessage:  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMongoCFG: {
		Query: sqlutil.Stmt{
			Name:  "SelectMongoCFGLogs",
			Query: queryTemplateForMongoDBTable("mdb.mongocfg"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			"severity":    {Filterable: true},
			ParamHostname: {Filterable: true},
			"component":   {},
			"context":     {},
			ParamMessage:  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMongoDBAudit: {
		Query: sqlutil.Stmt{
			Name:  "SelectMongoDBAuditLogs",
			Query: queryTemplateForMongoDBAuditTable("mdb.mongodb_audit"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			ParamMessage:  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMySQLGeneral: {
		Query: sqlutil.Stmt{
			Name:  "SelectMySQLGeneral",
			Query: queryTemplateForTableWithMs("mdb.mysql_general"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"id":          {},
			"command":     {},
			"argument":    {},
			"raw":         {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMySQLError: {
		Query: sqlutil.Stmt{
			Name:  "SelectMySQLError",
			Query: queryTemplateForTableWithMs("mdb.mysql_error"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"id":          {},
			"status":      {Filterable: true},
			ParamMessage:  {},
			"raw":         {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMySQLSlowQuery: {
		Query: sqlutil.Stmt{
			Name:  "SelectMySQLSlowQuery",
			Query: queryTemplateForTableWithMs("mdb.mysql_slow_query"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname:   {Filterable: true},
			"id":            {},
			"user":          {},
			"schema":        {},
			"last_errno":    {},
			"killed":        {},
			"query_time":    {},
			"lock_time":     {},
			"rows_sent":     {},
			"rows_examined": {},
			"rows_affected": {},
			"bytes_sent":    {},
			"query":         {},
			"raw":           {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeMySQLAudit: {
		Query: sqlutil.Stmt{
			Name:  "SelectMySQLAudit",
			Query: queryTemplateForTableWithMs("mdb.mysql_audit"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname:      {Filterable: true},
			"name":             {},
			"record":           {},
			"command_class":    {},
			"connection_id":    {},
			"db":               {},
			"ip":               {},
			"mysql_version":    {},
			"os_login":         {},
			"os_version":       {},
			"priv_user":        {},
			"proxy_user":       {},
			"server_id":        {},
			"sqltext":          {},
			"startup_optionsi": {},
			"status":           {},
			"status_code":      {},
			"user":             {},
			"version":          {},
			"raw":              {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypePostgreSQL: {
		Query: sqlutil.Stmt{
			Name:  "SelectPostgreSQLLogs",
			Query: queryTemplateForTableWithMs("mdb.postgres"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname:            {Filterable: true},
			"error_severity":         {Filterable: true},
			"application_name":       {},
			"command_tag":            {},
			"connection_from":        {},
			"context":                {},
			"database_name":          {},
			"detail":                 {},
			"hint":                   {},
			"internal_query":         {},
			"internal_query_pos":     {},
			"location":               {},
			ParamMessage:             {},
			"process_id":             {},
			"query":                  {},
			"query_pos":              {},
			"session_id":             {},
			"session_line_num":       {},
			"session_start_time":     {},
			"sql_state_code":         {},
			"transaction_id":         {},
			"user_name":              {},
			"virtual_transaction_id": {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeOdyssey: {
		Query: sqlutil.Stmt{
			Name:  "SelectOdysseyLogs",
			Query: queryTemplateForTableWithMs("mdb.odyssey"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"level":       {Filterable: true},
			"client_id":   {},
			"context":     {},
			"db":          {},
			"pid":         {},
			"server_id":   {},
			"text":        {},
			"user":        {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypePGBouncer: {
		Query: sqlutil.Stmt{
			Name:  "SelectPGBouncerLogs",
			Query: queryTemplateForTableWithMs("mdb.pgbouncer"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"level":       {Filterable: true},
			"db":          {},
			"pid":         {},
			"session_id":  {},
			"source":      {},
			"text":        {},
			"user ":       {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeRedis: {
		Query: sqlutil.Stmt{
			Name:  "SelectRedisLogs",
			Query: queryTemplateForTableWithMs("mdb.redis"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"role":        {},
			"pid":         {},
			ParamMessage:  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeElasticsearch: {
		Query: sqlutil.Stmt{
			Name:  "SelectElasticsearchLogs",
			Query: queryTemplateForTableWithMs("mdb.elasticsearch"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"level":       {Filterable: true},
			"component":   {Filterable: true},
			ParamMessage:  {},
			"stacktrace":  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeKibana: {
		Query: sqlutil.Stmt{
			Name:  "SelectKibanaLogs",
			Query: queryTemplateForTableWithMs("mdb.kibana"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"type":        {Filterable: true},
			ParamMessage:  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeKafka: {
		Query: sqlutil.Stmt{
			Name:  "SelectKafkaLogs",
			Query: queryTemplateForTableWithMs("mdb.kafka"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			ParamHostname: {Filterable: true},
			"severity":    {Filterable: true},
			"origin":      {Filterable: true},
			ParamMessage:  {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeGreenPlum: {
		Query: sqlutil.Stmt{
			Name:  "SelectGreenplumLogs",
			Query: queryTemplateForTableWithMs("mdb.greenplum"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			"cluster":            {},
			"database_name":      {},
			"debug_query_string": {},
			"distr_tranx_id":     {},
			"error_cursor_pos":   {},
			"event_context":      {},
			"event_detail":       {},
			"event_hint":         {},
			"event_message":      {},
			"event_severity":     {Filterable: true},
			"event_time":         {},
			"file_line":          {},
			"file_name":          {},
			"func_name":          {},
			"gp_command_count":   {},
			"gp_host_type":       {},
			"gp_preferred_role":  {},
			"gp_segment":         {},
			"gp_session_id":      {},
			"internal_query":     {},
			"internal_query_pos": {},
			"local_tranx_id":     {},
			ParamHostname:        {Filterable: true},
			"process_id":         {},
			"remote_host":        {},
			"remote_port":        {},
			"session_start_time": {},
			"slice_id":           {},
			"sql_state_code":     {},
			"stack_trace":        {},
			"sub_tranx_id":       {},
			"thread_id":          {},
			"transaction_id":     {},
			"user_name":          {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
	logsdb.LogTypeGreenPlumOdyssey: {
		Query: sqlutil.Stmt{
			Name:  "SelectGreenplumPoolerLogs",
			Query: queryTemplateForTableWithMs("mdb.greenplum_odyssey"),
		},
		Columns: map[string]sqlbuild.ColumnOptions{
			"client_id":   {},
			"context":     {},
			"db":          {},
			ParamHostname: {Filterable: true},
			"level":       {Filterable: true},
			"pid":         {},
			"server_id":   {},
			"text":        {},
			"user":        {},
		},
		QueryFormatArguments: []interface{}{"ms"},
	},
}

func BuildQuery(st logsdb.LogType, columnFilter []string, conditions []sqlfilter.Term, timeCol string) (sqlutil.Stmt, map[string]interface{}, error) {
	query, err := chooseSelectLogsQueryAndParams(st)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	columns, err := sqlbuild.FilterQueryColumns(query.Columns, columnFilter)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	sortedColumns := sortedKeys(columns) // preserve deterministic order in order to make code easier to test
	columnsString := strings.Join(sortedColumns, ",")

	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(query.Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	formatArguments := []interface{}{timeCol, columnsString, whereCond}
	if len(query.QueryFormatArguments) > 0 {
		formatArguments = append(formatArguments, query.QueryFormatArguments...)
	}

	return query.Query.Format(formatArguments...), whereParams, nil
}

func BuildQueryAndParams(lst logsdb.LogType, columnFilter []string, conditions []sqlfilter.Term, timeCol string) (sqlutil.Stmt, map[string]interface{}, []string, error) {
	query, err := chooseSelectLogsQueryAndParams(lst)
	if err != nil {
		return sqlutil.Stmt{}, nil, nil, err
	}

	columns, err := sqlbuild.FilterQueryColumns(query.Columns, columnFilter)
	if err != nil {
		return sqlutil.Stmt{}, nil, nil, err
	}

	whereCond, whereParams, err := sqlbuild.MakeQueryConditions(query.Columns, filterFieldPrefix, conditions)
	if err != nil {
		return sqlutil.Stmt{}, nil, nil, err
	}

	userColumns := make([]string, 0)
	for k := range columns {
		userColumns = append(userColumns, k)
	}
	sort.Strings(userColumns) // preserve deterministic order in order to make code easier to test
	columnsString := strings.Join(userColumns, ",")

	allColumns := []string{ParamMilliseconds, ParamSeconds}
	allColumns = append(allColumns, userColumns...)

	formatArguments := []interface{}{timeCol, columnsString, whereCond}
	if len(query.QueryFormatArguments) > 0 {
		formatArguments = append(formatArguments, query.QueryFormatArguments...)
	}

	return query.Query.Format(formatArguments...), whereParams, allColumns, nil
}

func chooseSelectLogsQueryAndParams(lst logsdb.LogType) (logTypeQuery, error) {
	query, ok := logTypeQueries[lst]
	if !ok {
		return logTypeQuery{}, xerrors.Errorf("unknown logs service type %q", lst)
	}

	return query, nil
}

func sortedKeys(a map[string]sqlbuild.ColumnOptions) []string {
	keys := make([]string, 0, len(a))
	for k := range a {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	return keys
}
