package mysql

import (
	mysqlv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

var (
	mapListLogsServiceTypeToGRPC = map[logs.ServiceType]mysqlv1.ListClusterLogsRequest_ServiceType{
		logs.ServiceTypeMySQLGeneral:   mysqlv1.ListClusterLogsRequest_MYSQL_GENERAL,
		logs.ServiceTypeMySQLError:     mysqlv1.ListClusterLogsRequest_MYSQL_ERROR,
		logs.ServiceTypeMySQLSlowQuery: mysqlv1.ListClusterLogsRequest_MYSQL_SLOW_QUERY,
		logs.ServiceTypeMySQLAudit:     mysqlv1.ListClusterLogsRequest_MYSQL_AUDIT,
	}
	mapListLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapListLogsServiceTypeToGRPC).(map[mysqlv1.ListClusterLogsRequest_ServiceType]logs.ServiceType)
)

func ListLogsServiceTypeToGRPC(st logs.ServiceType) mysqlv1.ListClusterLogsRequest_ServiceType {
	v, ok := mapListLogsServiceTypeToGRPC[st]
	if !ok {
		return mysqlv1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func ListLogsServiceTypeFromGRPC(st mysqlv1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapListLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var (
	mapStreamLogsServiceTypeToGRPC = map[logs.ServiceType]mysqlv1.StreamClusterLogsRequest_ServiceType{
		logs.ServiceTypeMySQLGeneral:   mysqlv1.StreamClusterLogsRequest_MYSQL_GENERAL,
		logs.ServiceTypeMySQLError:     mysqlv1.StreamClusterLogsRequest_MYSQL_ERROR,
		logs.ServiceTypeMySQLSlowQuery: mysqlv1.StreamClusterLogsRequest_MYSQL_SLOW_QUERY,
		logs.ServiceTypeMySQLAudit:     mysqlv1.StreamClusterLogsRequest_MYSQL_AUDIT,
	}
	mapStreamLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapStreamLogsServiceTypeToGRPC).(map[mysqlv1.StreamClusterLogsRequest_ServiceType]logs.ServiceType)
)

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) mysqlv1.StreamClusterLogsRequest_ServiceType {
	v, ok := mapStreamLogsServiceTypeToGRPC[st]
	if !ok {
		return mysqlv1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func StreamLogsServiceTypeFromGRPC(st mysqlv1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapStreamLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var groupByToGRPC = map[mymodels.MySessionsColumn]mysqlv1.GetSessionsStatsRequest_GroupBy{
	mymodels.MySessionsTime:           mysqlv1.GetSessionsStatsRequest_TIME,
	mymodels.MySessionsHost:           mysqlv1.GetSessionsStatsRequest_HOST,
	mymodels.MySessionsDatabase:       mysqlv1.GetSessionsStatsRequest_DATABASE,
	mymodels.MySessionsUser:           mysqlv1.GetSessionsStatsRequest_USER,
	mymodels.MySessionsStage:          mysqlv1.GetSessionsStatsRequest_STAGE,
	mymodels.MySessionsCurrentWait:    mysqlv1.GetSessionsStatsRequest_CURRENT_WAIT,
	mymodels.MySessionsWaitObject:     mysqlv1.GetSessionsStatsRequest_WAIT_OBJECT,
	mymodels.MySessionsClientHostname: mysqlv1.GetSessionsStatsRequest_CLIENT_HOSTNAME,
	mymodels.MySessionsDigest:         mysqlv1.GetSessionsStatsRequest_DIGEST,
}
var groupByFromGRPC = reflectutil.ReverseMap(groupByToGRPC).(map[mysqlv1.GetSessionsStatsRequest_GroupBy]mymodels.MySessionsColumn)

func GroupsByFromGRPC(groupBy []mysqlv1.GetSessionsStatsRequest_GroupBy) []mymodels.MySessionsColumn {
	res := make([]mymodels.MySessionsColumn, 0, len(groupBy))
	for _, column := range groupBy {
		if v, ok := groupByFromGRPC[column]; ok {
			res = append(res, v)
		}
	}
	return res
}

var statementsStatsGroupByToGRPC = map[mymodels.StatementsStatsGroupBy]mysqlv1.GetStatementsStatsRequest_GroupBy{
	mymodels.StatementsStatsGroupByDatabase: mysqlv1.GetStatementsStatsRequest_DATABASE,
	mymodels.StatementsStatsGroupByHost:     mysqlv1.GetStatementsStatsRequest_HOST,
}
var statementsStatsGroupByFromGRPC = reflectutil.ReverseMap(statementsStatsGroupByToGRPC).(map[mysqlv1.GetStatementsStatsRequest_GroupBy]mymodels.StatementsStatsGroupBy)

var statementStatsGroupByToGRPC = map[mymodels.StatementsStatsGroupBy]mysqlv1.GetStatementStatsRequest_GroupBy{
	mymodels.StatementsStatsGroupByDatabase: mysqlv1.GetStatementStatsRequest_DATABASE,
	mymodels.StatementsStatsGroupByHost:     mysqlv1.GetStatementStatsRequest_HOST,
}
var statementStatsGroupByFromGRPC = reflectutil.ReverseMap(statementStatsGroupByToGRPC).(map[mysqlv1.GetStatementStatsRequest_GroupBy]mymodels.StatementsStatsGroupBy)

func StatementsStatsGroupByFromGRPC(groupBy []mysqlv1.GetStatementsStatsRequest_GroupBy) []mymodels.StatementsStatsGroupBy {
	res := make([]mymodels.StatementsStatsGroupBy, 0, len(groupBy))
	for _, column := range groupBy {
		if v, ok := statementsStatsGroupByFromGRPC[column]; ok {
			res = append(res, v)
		}
	}
	return res
}

func StatementStatsGroupByFromGRPC(groupBy []mysqlv1.GetStatementStatsRequest_GroupBy) []mymodels.StatementsStatsGroupBy {
	res := make([]mymodels.StatementsStatsGroupBy, 0, len(groupBy))
	for _, column := range groupBy {
		if v, ok := statementStatsGroupByFromGRPC[column]; ok {
			res = append(res, v)
		}
	}
	return res
}

var statementsStatsFieldToGRPC = map[mymodels.MyStatementsColumn]mysqlv1.GetStatementsStatsRequest_Field{
	mymodels.MyStatementsDigest:              mysqlv1.GetStatementsStatsRequest_DIGEST,
	mymodels.MyStatementsQuery:               mysqlv1.GetStatementsStatsRequest_QUERY,
	mymodels.MyStatementsTotalQueryLatency:   mysqlv1.GetStatementsStatsRequest_TOTAL_QUERY_LATENCY,
	mymodels.MyStatementsTotalLockLatency:    mysqlv1.GetStatementsStatsRequest_TOTAL_LOCK_LATENCY,
	mymodels.MyStatementsAvgQueryLatency:     mysqlv1.GetStatementsStatsRequest_AVG_QUERY_LATENCY,
	mymodels.MyStatementsAvgLockLatency:      mysqlv1.GetStatementsStatsRequest_AVG_LOCK_LATENCY,
	mymodels.MyStatementsCalls:               mysqlv1.GetStatementsStatsRequest_CALLS,
	mymodels.MyStatementsErrors:              mysqlv1.GetStatementsStatsRequest_ERRORS,
	mymodels.MyStatementsWarnings:            mysqlv1.GetStatementsStatsRequest_WARNINGS,
	mymodels.MyStatementsRowsExamined:        mysqlv1.GetStatementsStatsRequest_ROWS_EXAMINED,
	mymodels.MyStatementsRowsSent:            mysqlv1.GetStatementsStatsRequest_ROWS_SENT,
	mymodels.MyStatementsRowsAffected:        mysqlv1.GetStatementsStatsRequest_ROWS_AFFECTED,
	mymodels.MyStatementsTmpTables:           mysqlv1.GetStatementsStatsRequest_TMP_TABLES,
	mymodels.MyStatementsTmpDiskTables:       mysqlv1.GetStatementsStatsRequest_TMP_DISK_TABLES,
	mymodels.MyStatementsSelectFullJoin:      mysqlv1.GetStatementsStatsRequest_SELECT_FULL_JOIN,
	mymodels.MyStatementsSelectFullRangeJoin: mysqlv1.GetStatementsStatsRequest_SELECT_FULL_RANGE_JOIN,
	mymodels.MyStatementsSelectRange:         mysqlv1.GetStatementsStatsRequest_SELECT_RANGE,
	mymodels.MyStatementsSelectRangeCheck:    mysqlv1.GetStatementsStatsRequest_SELECT_RANGE_CHECK,
	mymodels.MyStatementsSelectScan:          mysqlv1.GetStatementsStatsRequest_SELECT_SCAN,
	mymodels.MyStatementsSortMergePasses:     mysqlv1.GetStatementsStatsRequest_SORT_MERGE_PASSES,
	mymodels.MyStatementsSortRange:           mysqlv1.GetStatementsStatsRequest_SORT_RANGE,
	mymodels.MyStatementsSortRows:            mysqlv1.GetStatementsStatsRequest_SORT_ROWS,
	mymodels.MyStatementsSortScan:            mysqlv1.GetStatementsStatsRequest_SORT_SCAN,
	mymodels.MyStatementsNoIndexUsed:         mysqlv1.GetStatementsStatsRequest_NO_INDEX_USED,
	mymodels.MyStatementsNoGoodIndexUsed:     mysqlv1.GetStatementsStatsRequest_NO_GOOD_INDEX_USED,
}
var statementsStatsFieldFromGRPC = reflectutil.ReverseMap(statementsStatsFieldToGRPC).(map[mysqlv1.GetStatementsStatsRequest_Field]mymodels.MyStatementsColumn)

var statementStatsFieldToGRPC = map[mymodels.MyStatementsColumn]mysqlv1.GetStatementStatsRequest_Field{
	mymodels.MyStatementsDigest:              mysqlv1.GetStatementStatsRequest_DIGEST,
	mymodels.MyStatementsQuery:               mysqlv1.GetStatementStatsRequest_QUERY,
	mymodels.MyStatementsTotalQueryLatency:   mysqlv1.GetStatementStatsRequest_TOTAL_QUERY_LATENCY,
	mymodels.MyStatementsTotalLockLatency:    mysqlv1.GetStatementStatsRequest_TOTAL_LOCK_LATENCY,
	mymodels.MyStatementsAvgQueryLatency:     mysqlv1.GetStatementStatsRequest_AVG_QUERY_LATENCY,
	mymodels.MyStatementsAvgLockLatency:      mysqlv1.GetStatementStatsRequest_AVG_LOCK_LATENCY,
	mymodels.MyStatementsCalls:               mysqlv1.GetStatementStatsRequest_CALLS,
	mymodels.MyStatementsErrors:              mysqlv1.GetStatementStatsRequest_ERRORS,
	mymodels.MyStatementsWarnings:            mysqlv1.GetStatementStatsRequest_WARNINGS,
	mymodels.MyStatementsRowsExamined:        mysqlv1.GetStatementStatsRequest_ROWS_EXAMINED,
	mymodels.MyStatementsRowsSent:            mysqlv1.GetStatementStatsRequest_ROWS_SENT,
	mymodels.MyStatementsRowsAffected:        mysqlv1.GetStatementStatsRequest_ROWS_AFFECTED,
	mymodels.MyStatementsTmpTables:           mysqlv1.GetStatementStatsRequest_TMP_TABLES,
	mymodels.MyStatementsTmpDiskTables:       mysqlv1.GetStatementStatsRequest_TMP_DISK_TABLES,
	mymodels.MyStatementsSelectFullJoin:      mysqlv1.GetStatementStatsRequest_SELECT_FULL_JOIN,
	mymodels.MyStatementsSelectFullRangeJoin: mysqlv1.GetStatementStatsRequest_SELECT_FULL_RANGE_JOIN,
	mymodels.MyStatementsSelectRange:         mysqlv1.GetStatementStatsRequest_SELECT_RANGE,
	mymodels.MyStatementsSelectRangeCheck:    mysqlv1.GetStatementStatsRequest_SELECT_RANGE_CHECK,
	mymodels.MyStatementsSelectScan:          mysqlv1.GetStatementStatsRequest_SELECT_SCAN,
	mymodels.MyStatementsSortMergePasses:     mysqlv1.GetStatementStatsRequest_SORT_MERGE_PASSES,
	mymodels.MyStatementsSortRange:           mysqlv1.GetStatementStatsRequest_SORT_RANGE,
	mymodels.MyStatementsSortRows:            mysqlv1.GetStatementStatsRequest_SORT_ROWS,
	mymodels.MyStatementsSortScan:            mysqlv1.GetStatementStatsRequest_SORT_SCAN,
	mymodels.MyStatementsNoIndexUsed:         mysqlv1.GetStatementStatsRequest_NO_INDEX_USED,
	mymodels.MyStatementsNoGoodIndexUsed:     mysqlv1.GetStatementStatsRequest_NO_GOOD_INDEX_USED,
}
var statementStatsFieldFromGRPC = reflectutil.ReverseMap(statementStatsFieldToGRPC).(map[mysqlv1.GetStatementStatsRequest_Field]mymodels.MyStatementsColumn)

var sessionAtTimeFieldToGRPC = map[mymodels.MySessionsColumn]mysqlv1.GetSessionsAtTimeRequest_Field{
	mymodels.MySessionsTime:           mysqlv1.GetSessionsAtTimeRequest_TIME,
	mymodels.MySessionsHost:           mysqlv1.GetSessionsAtTimeRequest_HOST,
	mymodels.MySessionsDatabase:       mysqlv1.GetSessionsAtTimeRequest_DATABASE,
	mymodels.MySessionsUser:           mysqlv1.GetSessionsAtTimeRequest_USER,
	mymodels.MySessionsCommand:        mysqlv1.GetSessionsAtTimeRequest_COMMAND,
	mymodels.MySessionsQuery:          mysqlv1.GetSessionsAtTimeRequest_QUERY,
	mymodels.MySessionsDigest:         mysqlv1.GetSessionsAtTimeRequest_DIGEST,
	mymodels.MySessionsConnID:         mysqlv1.GetSessionsAtTimeRequest_CONN_ID,
	mymodels.MySessionsQueryLatency:   mysqlv1.GetSessionsAtTimeRequest_QUERY_LATENCY,
	mymodels.MySessionsLockLatency:    mysqlv1.GetSessionsAtTimeRequest_LOCK_LATENCY,
	mymodels.MySessionsStage:          mysqlv1.GetSessionsAtTimeRequest_STAGE,
	mymodels.MySessionsStageLatency:   mysqlv1.GetSessionsAtTimeRequest_STAGE_LATENCY,
	mymodels.MySessionsCurrentWait:    mysqlv1.GetSessionsAtTimeRequest_CURRENT_WAIT,
	mymodels.MySessionsWaitObject:     mysqlv1.GetSessionsAtTimeRequest_WAIT_OBJECT,
	mymodels.MySessionsWaitLatency:    mysqlv1.GetSessionsAtTimeRequest_WAIT_LATENCY,
	mymodels.MySessionsTrxLatency:     mysqlv1.GetSessionsAtTimeRequest_TRX_LATENCY,
	mymodels.MySessionsCurrentMemory:  mysqlv1.GetSessionsAtTimeRequest_CURRENT_MEMORY,
	mymodels.MySessionsClientAddr:     mysqlv1.GetSessionsAtTimeRequest_CLIENT_ADDR,
	mymodels.MySessionsClientHostname: mysqlv1.GetSessionsAtTimeRequest_CLIENT_HOSTNAME,
}
var sessionAtTimeFieldFromGRPC = reflectutil.ReverseMap(sessionAtTimeFieldToGRPC).(map[mysqlv1.GetSessionsAtTimeRequest_Field]mymodels.MySessionsColumn)

var statementsAtTimeFieldToGRPC = map[mymodels.MyStatementsColumn]mysqlv1.GetStatementsAtTimeRequest_Field{
	mymodels.MyStatementsTime:                mysqlv1.GetStatementsAtTimeRequest_TIME,
	mymodels.MyStatementsHost:                mysqlv1.GetStatementsAtTimeRequest_HOST,
	mymodels.MyStatementsDatabase:            mysqlv1.GetStatementsAtTimeRequest_DATABASE,
	mymodels.MyStatementsDigest:              mysqlv1.GetStatementsAtTimeRequest_DIGEST,
	mymodels.MyStatementsQuery:               mysqlv1.GetStatementsAtTimeRequest_QUERY,
	mymodels.MyStatementsTotalQueryLatency:   mysqlv1.GetStatementsAtTimeRequest_TOTAL_QUERY_LATENCY,
	mymodels.MyStatementsTotalLockLatency:    mysqlv1.GetStatementsAtTimeRequest_TOTAL_LOCK_LATENCY,
	mymodels.MyStatementsAvgQueryLatency:     mysqlv1.GetStatementsAtTimeRequest_AVG_QUERY_LATENCY,
	mymodels.MyStatementsAvgLockLatency:      mysqlv1.GetStatementsAtTimeRequest_AVG_LOCK_LATENCY,
	mymodels.MyStatementsCalls:               mysqlv1.GetStatementsAtTimeRequest_CALLS,
	mymodels.MyStatementsErrors:              mysqlv1.GetStatementsAtTimeRequest_ERRORS,
	mymodels.MyStatementsWarnings:            mysqlv1.GetStatementsAtTimeRequest_WARNINGS,
	mymodels.MyStatementsRowsExamined:        mysqlv1.GetStatementsAtTimeRequest_ROWS_EXAMINED,
	mymodels.MyStatementsRowsSent:            mysqlv1.GetStatementsAtTimeRequest_ROWS_SENT,
	mymodels.MyStatementsRowsAffected:        mysqlv1.GetStatementsAtTimeRequest_ROWS_AFFECTED,
	mymodels.MyStatementsTmpTables:           mysqlv1.GetStatementsAtTimeRequest_TMP_TABLES,
	mymodels.MyStatementsTmpDiskTables:       mysqlv1.GetStatementsAtTimeRequest_TMP_DISK_TABLES,
	mymodels.MyStatementsSelectFullJoin:      mysqlv1.GetStatementsAtTimeRequest_SELECT_FULL_JOIN,
	mymodels.MyStatementsSelectFullRangeJoin: mysqlv1.GetStatementsAtTimeRequest_SELECT_FULL_RANGE_JOIN,
	mymodels.MyStatementsSelectRange:         mysqlv1.GetStatementsAtTimeRequest_SELECT_RANGE,
	mymodels.MyStatementsSelectRangeCheck:    mysqlv1.GetStatementsAtTimeRequest_SELECT_RANGE_CHECK,
	mymodels.MyStatementsSelectScan:          mysqlv1.GetStatementsAtTimeRequest_SELECT_SCAN,
	mymodels.MyStatementsSortMergePasses:     mysqlv1.GetStatementsAtTimeRequest_SORT_MERGE_PASSES,
	mymodels.MyStatementsSortRange:           mysqlv1.GetStatementsAtTimeRequest_SORT_RANGE,
	mymodels.MyStatementsSortRows:            mysqlv1.GetStatementsAtTimeRequest_SORT_ROWS,
	mymodels.MyStatementsSortScan:            mysqlv1.GetStatementsAtTimeRequest_SORT_SCAN,
	mymodels.MyStatementsNoIndexUsed:         mysqlv1.GetStatementsAtTimeRequest_NO_INDEX_USED,
	mymodels.MyStatementsNoGoodIndexUsed:     mysqlv1.GetStatementsAtTimeRequest_NO_GOOD_INDEX_USED,
}
var statementsAtTimeFieldFromGRPC = reflectutil.ReverseMap(statementsAtTimeFieldToGRPC).(map[mysqlv1.GetStatementsAtTimeRequest_Field]mymodels.MyStatementsColumn)

var statementsIntervalFieldToGRPC = map[mymodels.MyStatementsColumn]mysqlv1.GetStatementsIntervalRequest_Field{
	mymodels.MyStatementsTime:                mysqlv1.GetStatementsIntervalRequest_TIME,
	mymodels.MyStatementsHost:                mysqlv1.GetStatementsIntervalRequest_HOST,
	mymodels.MyStatementsDatabase:            mysqlv1.GetStatementsIntervalRequest_DATABASE,
	mymodels.MyStatementsDigest:              mysqlv1.GetStatementsIntervalRequest_DIGEST,
	mymodels.MyStatementsQuery:               mysqlv1.GetStatementsIntervalRequest_QUERY,
	mymodels.MyStatementsTotalQueryLatency:   mysqlv1.GetStatementsIntervalRequest_TOTAL_QUERY_LATENCY,
	mymodels.MyStatementsTotalLockLatency:    mysqlv1.GetStatementsIntervalRequest_TOTAL_LOCK_LATENCY,
	mymodels.MyStatementsAvgQueryLatency:     mysqlv1.GetStatementsIntervalRequest_AVG_QUERY_LATENCY,
	mymodels.MyStatementsAvgLockLatency:      mysqlv1.GetStatementsIntervalRequest_AVG_LOCK_LATENCY,
	mymodels.MyStatementsCalls:               mysqlv1.GetStatementsIntervalRequest_CALLS,
	mymodels.MyStatementsErrors:              mysqlv1.GetStatementsIntervalRequest_ERRORS,
	mymodels.MyStatementsWarnings:            mysqlv1.GetStatementsIntervalRequest_WARNINGS,
	mymodels.MyStatementsRowsExamined:        mysqlv1.GetStatementsIntervalRequest_ROWS_EXAMINED,
	mymodels.MyStatementsRowsSent:            mysqlv1.GetStatementsIntervalRequest_ROWS_SENT,
	mymodels.MyStatementsRowsAffected:        mysqlv1.GetStatementsIntervalRequest_ROWS_AFFECTED,
	mymodels.MyStatementsTmpTables:           mysqlv1.GetStatementsIntervalRequest_TMP_TABLES,
	mymodels.MyStatementsTmpDiskTables:       mysqlv1.GetStatementsIntervalRequest_TMP_DISK_TABLES,
	mymodels.MyStatementsSelectFullJoin:      mysqlv1.GetStatementsIntervalRequest_SELECT_FULL_JOIN,
	mymodels.MyStatementsSelectFullRangeJoin: mysqlv1.GetStatementsIntervalRequest_SELECT_FULL_RANGE_JOIN,
	mymodels.MyStatementsSelectRange:         mysqlv1.GetStatementsIntervalRequest_SELECT_RANGE,
	mymodels.MyStatementsSelectRangeCheck:    mysqlv1.GetStatementsIntervalRequest_SELECT_RANGE_CHECK,
	mymodels.MyStatementsSelectScan:          mysqlv1.GetStatementsIntervalRequest_SELECT_SCAN,
	mymodels.MyStatementsSortMergePasses:     mysqlv1.GetStatementsIntervalRequest_SORT_MERGE_PASSES,
	mymodels.MyStatementsSortRange:           mysqlv1.GetStatementsIntervalRequest_SORT_RANGE,
	mymodels.MyStatementsSortRows:            mysqlv1.GetStatementsIntervalRequest_SORT_ROWS,
	mymodels.MyStatementsSortScan:            mysqlv1.GetStatementsIntervalRequest_SORT_SCAN,
	mymodels.MyStatementsNoIndexUsed:         mysqlv1.GetStatementsIntervalRequest_NO_INDEX_USED,
	mymodels.MyStatementsNoGoodIndexUsed:     mysqlv1.GetStatementsIntervalRequest_NO_GOOD_INDEX_USED,
}
var statementsIntervalFieldFromGRPC = reflectutil.ReverseMap(statementsIntervalFieldToGRPC).(map[mysqlv1.GetStatementsIntervalRequest_Field]mymodels.MyStatementsColumn)

var statementsDiffFieldToGRPC = map[mymodels.MyStatementsColumn]mysqlv1.GetStatementsDiffRequest_Field{
	mymodels.MyStatementsTime:                mysqlv1.GetStatementsDiffRequest_TIME,
	mymodels.MyStatementsHost:                mysqlv1.GetStatementsDiffRequest_HOST,
	mymodels.MyStatementsDatabase:            mysqlv1.GetStatementsDiffRequest_DATABASE,
	mymodels.MyStatementsDigest:              mysqlv1.GetStatementsDiffRequest_DIGEST,
	mymodels.MyStatementsQuery:               mysqlv1.GetStatementsDiffRequest_QUERY,
	mymodels.MyStatementsTotalQueryLatency:   mysqlv1.GetStatementsDiffRequest_TOTAL_QUERY_LATENCY,
	mymodels.MyStatementsTotalLockLatency:    mysqlv1.GetStatementsDiffRequest_TOTAL_LOCK_LATENCY,
	mymodels.MyStatementsAvgQueryLatency:     mysqlv1.GetStatementsDiffRequest_AVG_QUERY_LATENCY,
	mymodels.MyStatementsAvgLockLatency:      mysqlv1.GetStatementsDiffRequest_AVG_LOCK_LATENCY,
	mymodels.MyStatementsCalls:               mysqlv1.GetStatementsDiffRequest_CALLS,
	mymodels.MyStatementsErrors:              mysqlv1.GetStatementsDiffRequest_ERRORS,
	mymodels.MyStatementsWarnings:            mysqlv1.GetStatementsDiffRequest_WARNINGS,
	mymodels.MyStatementsRowsExamined:        mysqlv1.GetStatementsDiffRequest_ROWS_EXAMINED,
	mymodels.MyStatementsRowsSent:            mysqlv1.GetStatementsDiffRequest_ROWS_SENT,
	mymodels.MyStatementsRowsAffected:        mysqlv1.GetStatementsDiffRequest_ROWS_AFFECTED,
	mymodels.MyStatementsTmpTables:           mysqlv1.GetStatementsDiffRequest_TMP_TABLES,
	mymodels.MyStatementsTmpDiskTables:       mysqlv1.GetStatementsDiffRequest_TMP_DISK_TABLES,
	mymodels.MyStatementsSelectFullJoin:      mysqlv1.GetStatementsDiffRequest_SELECT_FULL_JOIN,
	mymodels.MyStatementsSelectFullRangeJoin: mysqlv1.GetStatementsDiffRequest_SELECT_FULL_RANGE_JOIN,
	mymodels.MyStatementsSelectRange:         mysqlv1.GetStatementsDiffRequest_SELECT_RANGE,
	mymodels.MyStatementsSelectRangeCheck:    mysqlv1.GetStatementsDiffRequest_SELECT_RANGE_CHECK,
	mymodels.MyStatementsSelectScan:          mysqlv1.GetStatementsDiffRequest_SELECT_SCAN,
	mymodels.MyStatementsSortMergePasses:     mysqlv1.GetStatementsDiffRequest_SORT_MERGE_PASSES,
	mymodels.MyStatementsSortRange:           mysqlv1.GetStatementsDiffRequest_SORT_RANGE,
	mymodels.MyStatementsSortRows:            mysqlv1.GetStatementsDiffRequest_SORT_ROWS,
	mymodels.MyStatementsSortScan:            mysqlv1.GetStatementsDiffRequest_SORT_SCAN,
	mymodels.MyStatementsNoIndexUsed:         mysqlv1.GetStatementsDiffRequest_NO_INDEX_USED,
	mymodels.MyStatementsNoGoodIndexUsed:     mysqlv1.GetStatementsDiffRequest_NO_GOOD_INDEX_USED,
}
var statementsDiffFieldFromGRPC = reflectutil.ReverseMap(statementsDiffFieldToGRPC).(map[mysqlv1.GetStatementsDiffRequest_Field]mymodels.MyStatementsColumn)

func StatementAtTimeToGRPC(st mymodels.Statements) *mysqlv1.Statement {
	v := &mysqlv1.Statement{
		Time:                grpcapi.TimeToGRPC(st.Timestamp),
		Host:                st.Host,
		Database:            st.Database,
		Digest:              st.Digest,
		Query:               st.Query,
		TotalQueryLatency:   st.TotalQueryLatency,
		TotalLockLatency:    st.TotalLockLatency,
		AvgQueryLatency:     st.AvgQueryLatency,
		AvgLockLatency:      st.AvgLockLatency,
		Calls:               st.Calls,
		Errors:              st.Errors,
		Warnings:            st.Warnings,
		RowsExamined:        st.RowsExamined,
		RowsSent:            st.RowsSent,
		RowsAffected:        st.RowsAffected,
		TmpTables:           st.TmpTables,
		TmpDiskTables:       st.TmpDiskTables,
		SelectFullJoin:      st.SelectFullJoin,
		SelectFullRangeJoin: st.SelectFullRangeJoin,
		SelectRange:         st.SelectRange,
		SelectRangeCheck:    st.SelectRangeCheck,
		SelectScan:          st.SelectScan,
		SortMergePasses:     st.SortMergePasses,
		SortRange:           st.SortRange,
		SortRows:            st.SortRows,
		SortScan:            st.SortScan,
		NoIndexUsed:         st.NoIndexUsed,
		NoGoodIndexUsed:     st.NoGoodIndexUsed,
	}
	return v
}

func StatementsAtTimeToGRPC(sessions []mymodels.Statements) []*mysqlv1.Statement {
	v := make([]*mysqlv1.Statement, 0, len(sessions))
	for _, session := range sessions {
		v = append(v, StatementAtTimeToGRPC(session))
	}
	return v
}

func StatementsStatsGRPC(statements []mymodels.Statements) []*mysqlv1.Statement {
	v := make([]*mysqlv1.Statement, 0, len(statements))
	for _, statement := range statements {
		v = append(v, StatementAtTimeToGRPC(statement))
	}
	return v
}

func SessionStateToGRPC(s mymodels.SessionState) *mysqlv1.SessionState {
	v := &mysqlv1.SessionState{
		Time:           grpcapi.TimeToGRPC(s.Timestamp),
		Host:           s.Host,
		Database:       s.Database,
		User:           s.User,
		ThdId:          s.ThdID,
		ConnId:         s.ConnID,
		Command:        s.Command,
		Query:          s.Query,
		Digest:         s.Digest,
		QueryLatency:   s.QueryLatency,
		LockLatency:    s.LockLatency,
		Stage:          s.Stage,
		StageLatency:   s.StageLatency,
		CurrentWait:    s.CurrentWait,
		WaitObject:     s.WaitObject,
		WaitLatency:    s.WaitLatency,
		TrxLatency:     s.TrxLatency,
		CurrentMemory:  s.CurrentMemory,
		ClientAddr:     s.ClientAddr,
		ClientHostname: s.ClientHostname,
		ClientPort:     s.ClientPort,
	}
	return v
}

func SessionsStateToGRPC(sessions []mymodels.SessionState) []*mysqlv1.SessionState {
	v := make([]*mysqlv1.SessionState, 0, len(sessions))
	for _, session := range sessions {
		v = append(v, SessionStateToGRPC(session))
	}
	return v
}

func IntervalToGRPC(interval mymodels.Interval) *mysqlv1.Interval {
	if !interval.StartTimestamp.Valid || !interval.EndTimestamp.Valid {
		return nil
	}
	v := &mysqlv1.Interval{
		StartTime: grpcapi.OptionalTimeToGRPC(interval.StartTimestamp),
		EndTime:   grpcapi.OptionalTimeToGRPC(interval.EndTimestamp),
	}
	return v
}

func DiffStatementToGRPC(diff mymodels.DiffStatement) *mysqlv1.DiffStatement {
	v := &mysqlv1.DiffStatement{
		FirstInterval:  IntervalToGRPC(diff.FirstIntervalTime),
		SecondInterval: IntervalToGRPC(diff.SecondIntervalTime),
		Host:           diff.Host,
		Database:       diff.Database,
		Query:          diff.Query,
		Digest:         diff.Digest,

		FirstTotalQueryLatency:    diff.FirstTotalQueryLatency,
		FirstTotalLockLatency:     diff.FirstTotalLockLatency,
		FirstAvgQueryLatency:      diff.FirstAvgQueryLatency,
		FirstAvgLockLatency:       diff.FirstAvgLockLatency,
		FirstCalls:                diff.FirstCalls,
		FirstErrors:               diff.FirstErrors,
		FirstWarnings:             diff.FirstWarnings,
		FirstRowsExamined:         diff.FirstRowsExamined,
		FirstRowsSent:             diff.FirstRowsSent,
		FirstRowsAffected:         diff.FirstRowsAffected,
		FirstTmpTables:            diff.FirstTmpTables,
		FirstTmpDiskTables:        diff.FirstTmpDiskTables,
		FirstSelectFullJoin:       diff.FirstSelectFullJoin,
		FirstSelectFullRangeJoin:  diff.FirstSelectFullRangeJoin,
		FirstSelectRange:          diff.FirstSelectRange,
		FirstSelectRangeCheck:     diff.FirstSelectRangeCheck,
		FirstSelectScan:           diff.FirstSelectScan,
		FirstSortMergePasses:      diff.FirstSortMergePasses,
		FirstSortRange:            diff.FirstSortRange,
		FirstSortRows:             diff.FirstSortRows,
		FirstSortScan:             diff.FirstSortScan,
		FirstNoIndexUsed:          diff.FirstNoIndexUsed,
		FirstNoGoodIndexUsed:      diff.FirstNoGoodIndexUsed,
		SecondTotalQueryLatency:   diff.SecondTotalQueryLatency,
		SecondTotalLockLatency:    diff.SecondTotalLockLatency,
		SecondAvgQueryLatency:     diff.SecondAvgQueryLatency,
		SecondAvgLockLatency:      diff.SecondAvgLockLatency,
		SecondCalls:               diff.SecondCalls,
		SecondErrors:              diff.SecondErrors,
		SecondWarnings:            diff.SecondWarnings,
		SecondRowsExamined:        diff.SecondRowsExamined,
		SecondRowsSent:            diff.SecondRowsSent,
		SecondRowsAffected:        diff.SecondRowsAffected,
		SecondTmpTables:           diff.SecondTmpTables,
		SecondTmpDiskTables:       diff.SecondTmpDiskTables,
		SecondSelectFullJoin:      diff.SecondSelectFullJoin,
		SecondSelectFullRangeJoin: diff.SecondSelectFullRangeJoin,
		SecondSelectRange:         diff.SecondSelectRange,
		SecondSelectRangeCheck:    diff.SecondSelectRangeCheck,
		SecondSelectScan:          diff.SecondSelectScan,
		SecondSortMergePasses:     diff.SecondSortMergePasses,
		SecondSortRange:           diff.SecondSortRange,
		SecondSortRows:            diff.SecondSortRows,
		SecondSortScan:            diff.SecondSortScan,
		SecondNoIndexUsed:         diff.SecondNoIndexUsed,
		SecondNoGoodIndexUsed:     diff.SecondNoGoodIndexUsed,
		DiffTotalQueryLatency:     diff.DiffTotalQueryLatency,
		DiffTotalLockLatency:      diff.DiffTotalLockLatency,
		DiffAvgQueryLatency:       diff.DiffAvgQueryLatency,
		DiffAvgLockLatency:        diff.DiffAvgLockLatency,
		DiffCalls:                 diff.DiffCalls,
		DiffErrors:                diff.DiffErrors,
		DiffWarnings:              diff.DiffWarnings,
		DiffRowsExamined:          diff.DiffRowsExamined,
		DiffRowsSent:              diff.DiffRowsSent,
		DiffRowsAffected:          diff.DiffRowsAffected,
		DiffTmpTables:             diff.DiffTmpTables,
		DiffTmpDiskTables:         diff.DiffTmpDiskTables,
		DiffSelectFullJoin:        diff.DiffSelectFullJoin,
		DiffSelectFullRangeJoin:   diff.DiffSelectFullRangeJoin,
		DiffSelectRange:           diff.DiffSelectRange,
		DiffSelectRangeCheck:      diff.DiffSelectRangeCheck,
		DiffSelectScan:            diff.DiffSelectScan,
		DiffSortMergePasses:       diff.DiffSortMergePasses,
		DiffSortRange:             diff.DiffSortRange,
		DiffSortRows:              diff.DiffSortRows,
		DiffSortScan:              diff.DiffSortScan,
		DiffNoIndexUsed:           diff.DiffNoIndexUsed,
		DiffNoGoodIndexUsed:       diff.DiffNoGoodIndexUsed,
	}
	return v
}

func StatementsDiffToGRPC(diffStatements []mymodels.DiffStatement) []*mysqlv1.DiffStatement {
	v := make([]*mysqlv1.DiffStatement, 0, len(diffStatements))
	for _, diffStatement := range diffStatements {
		v = append(v, DiffStatementToGRPC(diffStatement))
	}
	return v
}

var sortOrderToGRPC = map[mymodels.SortOrder]mysqlv1.GetSessionsStatsRequest_SortOrder{
	mymodels.OrderByAsc:  mysqlv1.GetSessionsStatsRequest_ASC,
	mymodels.OrderByDesc: mysqlv1.GetSessionsStatsRequest_DESC,
}
var sortOrderFromGRPC = reflectutil.ReverseMap(sortOrderToGRPC).(map[mysqlv1.GetSessionsStatsRequest_SortOrder]mymodels.SortOrder)

var sortOrderSessionAtTimeToGRPC = map[mymodels.SortOrder]mysqlv1.GetSessionsAtTimeRequest_SortOrder{
	mymodels.OrderByAsc:  mysqlv1.GetSessionsAtTimeRequest_ASC,
	mymodels.OrderByDesc: mysqlv1.GetSessionsAtTimeRequest_DESC,
}
var sortOrderSessionAtTimeFromGRPC = reflectutil.ReverseMap(sortOrderSessionAtTimeToGRPC).(map[mysqlv1.GetSessionsAtTimeRequest_SortOrder]mymodels.SortOrder)

var sortOrderStatementsAtTimeToGRPC = map[mymodels.SortOrder]mysqlv1.GetStatementsAtTimeRequest_SortOrder{
	mymodels.OrderByAsc:  mysqlv1.GetStatementsAtTimeRequest_ASC,
	mymodels.OrderByDesc: mysqlv1.GetStatementsAtTimeRequest_DESC,
}
var sortOrderStatementsAtTimeFromGRPC = reflectutil.ReverseMap(sortOrderStatementsAtTimeToGRPC).(map[mysqlv1.GetStatementsAtTimeRequest_SortOrder]mymodels.SortOrder)

var sortOrderStatementsDiffToGRPC = map[mymodels.SortOrder]mysqlv1.GetStatementsDiffRequest_SortOrder{
	mymodels.OrderByAsc:  mysqlv1.GetStatementsDiffRequest_ASC,
	mymodels.OrderByDesc: mysqlv1.GetStatementsDiffRequest_DESC,
}
var sortOrderStatementsDiffFromGRPC = reflectutil.ReverseMap(sortOrderStatementsDiffToGRPC).(map[mysqlv1.GetStatementsDiffRequest_SortOrder]mymodels.SortOrder)

var sortOrderStatementsIntervalToGRPC = map[mymodels.SortOrder]mysqlv1.GetStatementsIntervalRequest_SortOrder{
	mymodels.OrderByAsc:  mysqlv1.GetStatementsIntervalRequest_ASC,
	mymodels.OrderByDesc: mysqlv1.GetStatementsIntervalRequest_DESC,
}
var sortOrderStatementsIntervalFromGRPC = reflectutil.ReverseMap(sortOrderStatementsIntervalToGRPC).(map[mysqlv1.GetStatementsIntervalRequest_SortOrder]mymodels.SortOrder)

func SessionAtTimeOrderByFromGRPC(orderBy []*mysqlv1.GetSessionsAtTimeRequest_OrderBy) []mymodels.OrderBySessionsAtTime {
	res := make([]mymodels.OrderBySessionsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, mymodels.OrderBySessionsAtTime{
			Field:     sessionAtTimeFieldFromGRPC[order.Field],
			SortOrder: sortOrderSessionAtTimeFromGRPC[order.Order],
		})
	}
	return res
}

func StatementsAtTimeOrderByFromGRPC(orderBy []*mysqlv1.GetStatementsAtTimeRequest_OrderBy) []mymodels.OrderByStatementsAtTime {
	res := make([]mymodels.OrderByStatementsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, mymodels.OrderByStatementsAtTime{
			Field:     statementsAtTimeFieldFromGRPC[order.Field],
			SortOrder: sortOrderStatementsAtTimeFromGRPC[order.Order],
		})
	}
	return res
}

func StatementsIntervalOrderByFromGRPC(orderBy []*mysqlv1.GetStatementsIntervalRequest_OrderBy) []mymodels.OrderByStatementsAtTime {
	res := make([]mymodels.OrderByStatementsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, mymodels.OrderByStatementsAtTime{
			Field:     statementsIntervalFieldFromGRPC[order.Field],
			SortOrder: sortOrderStatementsIntervalFromGRPC[order.Order],
		})
	}
	return res
}

func StatementsDiffOrderByFromGRPC(orderBy []*mysqlv1.GetStatementsDiffRequest_OrderBy) []mymodels.OrderByStatementsAtTime {
	res := make([]mymodels.OrderByStatementsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, mymodels.OrderByStatementsAtTime{
			Field:     statementsDiffFieldFromGRPC[order.Field],
			SortOrder: sortOrderStatementsDiffFromGRPC[order.Order],
		})
	}
	return res
}

func OrderByFromGRPC(orderBy []*mysqlv1.GetSessionsStatsRequest_OrderBy) []mymodels.OrderBy {
	res := make([]mymodels.OrderBy, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, mymodels.OrderBy{
			Field:     groupByFromGRPC[order.Field],
			SortOrder: sortOrderFromGRPC[order.Order],
		})
	}
	return res
}

func SessionColumnFilterFromGRPC(fields []mysqlv1.GetSessionsAtTimeRequest_Field) []mymodels.MySessionsColumn {
	res := make([]mymodels.MySessionsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, sessionAtTimeFieldFromGRPC[field])
	}
	return res
}

func StatementsColumnFilterFromGRPC(fields []mysqlv1.GetStatementsAtTimeRequest_Field) []mymodels.MyStatementsColumn {
	res := make([]mymodels.MyStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsAtTimeFieldFromGRPC[field])
	}
	return res
}

func StatementsStatsColumnFilterFromGRPC(fields []mysqlv1.GetStatementsStatsRequest_Field) []mymodels.MyStatementsColumn {
	res := make([]mymodels.MyStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsStatsFieldFromGRPC[field])
	}
	return res
}

func StatementStatsColumnFilterFromGRPC(fields []mysqlv1.GetStatementStatsRequest_Field) []mymodels.MyStatementsColumn {
	res := make([]mymodels.MyStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementStatsFieldFromGRPC[field])
	}
	return res
}

func StatementsDiffColumnFilterFromGRPC(fields []mysqlv1.GetStatementsDiffRequest_Field) []mymodels.MyStatementsColumn {
	res := make([]mymodels.MyStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsDiffFieldFromGRPC[field])
	}
	return res
}

func StatementsIntervalColumnFilterFromGRPC(fields []mysqlv1.GetStatementsIntervalRequest_Field) []mymodels.MyStatementsColumn {
	res := make([]mymodels.MyStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsIntervalFieldFromGRPC[field])
	}
	return res
}

func GetSessionsAtTimeFromGRPC(req *mysqlv1.GetSessionsAtTimeRequest) (mymodels.GetSessionsAtTimeOptions, error) {
	var GetSessionsAtTimeArgs mymodels.GetSessionsAtTimeOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return mymodels.GetSessionsAtTimeOptions{}, err
	}
	GetSessionsAtTimeArgs.Filter = flt
	GetSessionsAtTimeArgs.Time = grpcapi.TimeFromGRPC(req.GetTime())
	GetSessionsAtTimeArgs.Limit = req.GetPageSize()
	GetSessionsAtTimeArgs.ColumnFilter = SessionColumnFilterFromGRPC(req.GetColumnFilter())
	GetSessionsAtTimeArgs.OrderBy = SessionAtTimeOrderByFromGRPC(req.GetOrderBy())
	return GetSessionsAtTimeArgs, nil
}

func GetSessionsStatsFromGRPC(req *mysqlv1.GetSessionsStatsRequest) (mymodels.GetSessionsStatsOptions, error) {
	var sessionStatsArgs mymodels.GetSessionsStatsOptions

	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return mymodels.GetSessionsStatsOptions{}, err
	}
	sessionStatsArgs.Filter = flt

	if req.GetFromTime() != nil {
		sessionStatsArgs.FromTS.Set(grpcapi.TimeFromGRPC(req.GetFromTime()))
	}
	if req.GetToTime() != nil {
		sessionStatsArgs.ToTS.Set(grpcapi.TimeFromGRPC(req.GetToTime()))
	}

	sessionStatsArgs.GroupBy = GroupsByFromGRPC(req.GetGroupBy())
	sessionStatsArgs.OrderBy = OrderByFromGRPC(req.GetOrderBy())

	sessionStatsArgs.RollupPeriod.Set(req.GetRollupPeriod())

	sessionStatsArgs.Limit = req.GetPageSize()

	return sessionStatsArgs, nil
}

func GetStatementsAtTimeFromGRPC(req *mysqlv1.GetStatementsAtTimeRequest) (mymodels.GetStatementsAtTimeOptions, error) {
	var GetStatementsAtTimeArgs mymodels.GetStatementsAtTimeOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return mymodels.GetStatementsAtTimeOptions{}, err
	}
	GetStatementsAtTimeArgs.Filter = flt
	GetStatementsAtTimeArgs.Time = grpcapi.TimeFromGRPC(req.GetTime())
	GetStatementsAtTimeArgs.Limit = req.GetPageSize()
	GetStatementsAtTimeArgs.ColumnFilter = StatementsColumnFilterFromGRPC(req.GetColumnFilter())
	GetStatementsAtTimeArgs.OrderBy = StatementsAtTimeOrderByFromGRPC(req.GetOrderBy())
	return GetStatementsAtTimeArgs, nil
}

func GetStatementsStatsFromGRPC(req *mysqlv1.GetStatementsStatsRequest) (mymodels.GetStatementsStatsOptions, error) {
	var statementsStatsArgs mymodels.GetStatementsStatsOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return mymodels.GetStatementsStatsOptions{}, err
	}
	statementsStatsArgs.Filter = flt
	if req.GetFromTime() != nil {
		statementsStatsArgs.FromTS.Set(grpcapi.TimeFromGRPC(req.GetFromTime()))
	}
	if req.GetToTime() != nil {
		statementsStatsArgs.ToTS.Set(grpcapi.TimeFromGRPC(req.GetToTime()))
	}

	statementsStatsArgs.GroupBy = StatementsStatsGroupByFromGRPC(req.GetGroupBy())
	statementsStatsArgs.ColumnFilter = StatementsStatsColumnFilterFromGRPC(req.GetColumnFilter())
	statementsStatsArgs.Limit = req.GetPageSize()
	return statementsStatsArgs, nil
}

func GetStatementStatsFromGRPC(req *mysqlv1.GetStatementStatsRequest) (mymodels.GetStatementStatsOptions, error) {
	var statementStatsArgs mymodels.GetStatementStatsOptions
	statementStatsArgs.Digest = req.GetDigest()
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return mymodels.GetStatementStatsOptions{}, err
	}
	statementStatsArgs.Filter = flt
	if req.GetFromTime() != nil {
		statementStatsArgs.FromTS.Set(grpcapi.TimeFromGRPC(req.GetFromTime()))
	}
	if req.GetToTime() != nil {
		statementStatsArgs.ToTS.Set(grpcapi.TimeFromGRPC(req.GetToTime()))
	}

	statementStatsArgs.GroupBy = StatementStatsGroupByFromGRPC(req.GetGroupBy())
	statementStatsArgs.ColumnFilter = StatementStatsColumnFilterFromGRPC(req.GetColumnFilter())
	statementStatsArgs.Limit = req.GetPageSize()
	return statementStatsArgs, nil
}

func GetStatementsDiffFromGRPC(req *mysqlv1.GetStatementsDiffRequest) (mymodels.GetStatementsDiffOptions, error) {
	var GetStatementsDiffOptions mymodels.GetStatementsDiffOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return mymodels.GetStatementsDiffOptions{}, err
	}
	GetStatementsDiffOptions.Filter = flt
	GetStatementsDiffOptions.FirstIntervalStart = grpcapi.TimeFromGRPC(req.GetFirstIntervalStart())
	GetStatementsDiffOptions.SecondIntervalStart = grpcapi.TimeFromGRPC(req.GetSecondIntervalStart())
	GetStatementsDiffOptions.IntervalsDuration.Set(req.GetIntervalsDuration())
	GetStatementsDiffOptions.Limit = req.GetPageSize()
	GetStatementsDiffOptions.ColumnFilter = StatementsDiffColumnFilterFromGRPC(req.GetColumnFilter())
	GetStatementsDiffOptions.OrderBy = StatementsDiffOrderByFromGRPC(req.GetOrderBy())
	return GetStatementsDiffOptions, nil
}

func GetStatementsIntervalFromGRPC(req *mysqlv1.GetStatementsIntervalRequest) (mymodels.GetStatementsIntervalOptions, error) {
	var GetStatementsIntervalArgs mymodels.GetStatementsIntervalOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return mymodels.GetStatementsIntervalOptions{}, err
	}
	GetStatementsIntervalArgs.Filter = flt
	if req.GetFromTime() != nil {
		GetStatementsIntervalArgs.FromTS.Set(grpcapi.TimeFromGRPC(req.GetFromTime()))
	}
	if req.GetToTime() != nil {
		GetStatementsIntervalArgs.ToTS.Set(grpcapi.TimeFromGRPC(req.GetToTime()))
	}
	GetStatementsIntervalArgs.Limit = req.GetPageSize()
	GetStatementsIntervalArgs.ColumnFilter = StatementsIntervalColumnFilterFromGRPC(req.GetColumnFilter())
	GetStatementsIntervalArgs.OrderBy = StatementsIntervalOrderByFromGRPC(req.GetOrderBy())
	return GetStatementsIntervalArgs, nil
}
