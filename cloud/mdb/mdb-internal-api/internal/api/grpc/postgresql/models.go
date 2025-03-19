package postgresql

import (
	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
)

var (
	mapListLogsServiceTypeToGRPC = map[logs.ServiceType]pgv1.ListClusterLogsRequest_ServiceType{
		logs.ServiceTypePostgreSQL: pgv1.ListClusterLogsRequest_POSTGRESQL,
		logs.ServiceTypePooler:     pgv1.ListClusterLogsRequest_POOLER,
	}
	mapListLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapListLogsServiceTypeToGRPC).(map[pgv1.ListClusterLogsRequest_ServiceType]logs.ServiceType)
)

func ListLogsServiceTypeToGRPC(st logs.ServiceType) pgv1.ListClusterLogsRequest_ServiceType {
	v, ok := mapListLogsServiceTypeToGRPC[st]
	if !ok {
		return pgv1.ListClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func ListLogsServiceTypeFromGRPC(st pgv1.ListClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapListLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var (
	mapStreamLogsServiceTypeToGRPC = map[logs.ServiceType]pgv1.StreamClusterLogsRequest_ServiceType{
		logs.ServiceTypePostgreSQL: pgv1.StreamClusterLogsRequest_POSTGRESQL,
		logs.ServiceTypePooler:     pgv1.StreamClusterLogsRequest_POOLER,
	}
	mapStreamLogsServiceTypeFromGRPC = reflectutil.ReverseMap(mapStreamLogsServiceTypeToGRPC).(map[pgv1.StreamClusterLogsRequest_ServiceType]logs.ServiceType)
)

func StreamLogsServiceTypeToGRPC(st logs.ServiceType) pgv1.StreamClusterLogsRequest_ServiceType {
	v, ok := mapStreamLogsServiceTypeToGRPC[st]
	if !ok {
		return pgv1.StreamClusterLogsRequest_SERVICE_TYPE_UNSPECIFIED
	}

	return v
}

func StreamLogsServiceTypeFromGRPC(st pgv1.StreamClusterLogsRequest_ServiceType) (logs.ServiceType, error) {
	v, ok := mapStreamLogsServiceTypeFromGRPC[st]
	if !ok {
		return logs.ServiceTypeInvalid, semerr.InvalidInput("unknown service type")
	}

	return v, nil
}

var groupByToGRPC = map[pgmodels.PGStatActivityColumn]pgv1.GetSessionsStatsRequest_GroupBy{
	pgmodels.PGStatActivityTime:          pgv1.GetSessionsStatsRequest_TIME,
	pgmodels.PGStatActivityHost:          pgv1.GetSessionsStatsRequest_HOST,
	pgmodels.PGStatActivityDatabase:      pgv1.GetSessionsStatsRequest_DATABASE,
	pgmodels.PGStatActivityUser:          pgv1.GetSessionsStatsRequest_USER,
	pgmodels.PGStatActivityWaitEventType: pgv1.GetSessionsStatsRequest_WAIT_EVENT_TYPE,
	pgmodels.PGStatActivityWaitEvent:     pgv1.GetSessionsStatsRequest_WAIT_EVENT,
	pgmodels.PGStatActivityState:         pgv1.GetSessionsStatsRequest_STATE,
	pgmodels.PGStatActivityQuery:         pgv1.GetSessionsStatsRequest_QUERY,
}
var groupByFromGRPC = reflectutil.ReverseMap(groupByToGRPC).(map[pgv1.GetSessionsStatsRequest_GroupBy]pgmodels.PGStatActivityColumn)

func GroupsByFromGRPC(groupBy []pgv1.GetSessionsStatsRequest_GroupBy) []pgmodels.PGStatActivityColumn {
	res := make([]pgmodels.PGStatActivityColumn, 0, len(groupBy))
	for _, column := range groupBy {
		if v, ok := groupByFromGRPC[column]; ok {
			res = append(res, v)
		}
	}
	return res
}

var statementsStatsGroupByToGRPC = map[pgmodels.StatementsStatsGroupBy]pgv1.GetStatementsStatsRequest_GroupBy{
	pgmodels.StatementsStatsGroupByDatabase: pgv1.GetStatementsStatsRequest_DATABASE,
	pgmodels.StatementsStatsGroupByHost:     pgv1.GetStatementsStatsRequest_HOST,
	pgmodels.StatementsStatsGroupByUser:     pgv1.GetStatementsStatsRequest_USER,
}
var statementsStatsGroupByFromGRPC = reflectutil.ReverseMap(statementsStatsGroupByToGRPC).(map[pgv1.GetStatementsStatsRequest_GroupBy]pgmodels.StatementsStatsGroupBy)

func StatementsStatsGroupByFromGRPC(groupBy []pgv1.GetStatementsStatsRequest_GroupBy) []pgmodels.StatementsStatsGroupBy {
	res := make([]pgmodels.StatementsStatsGroupBy, 0, len(groupBy))
	for _, column := range groupBy {
		if v, ok := statementsStatsGroupByFromGRPC[column]; ok {
			res = append(res, v)
		}
	}
	return res
}

var statementStatsGroupByToGRPC = map[pgmodels.StatementsStatsGroupBy]pgv1.GetStatementStatsRequest_GroupBy{
	pgmodels.StatementsStatsGroupByDatabase: pgv1.GetStatementStatsRequest_DATABASE,
	pgmodels.StatementsStatsGroupByHost:     pgv1.GetStatementStatsRequest_HOST,
	pgmodels.StatementsStatsGroupByUser:     pgv1.GetStatementStatsRequest_USER,
}
var statementStatsGroupByFromGRPC = reflectutil.ReverseMap(statementStatsGroupByToGRPC).(map[pgv1.GetStatementStatsRequest_GroupBy]pgmodels.StatementsStatsGroupBy)

func StatementStatsGroupByFromGRPC(groupBy []pgv1.GetStatementStatsRequest_GroupBy) []pgmodels.StatementsStatsGroupBy {
	res := make([]pgmodels.StatementsStatsGroupBy, 0, len(groupBy))
	for _, column := range groupBy {
		if v, ok := statementStatsGroupByFromGRPC[column]; ok {
			res = append(res, v)
		}
	}
	return res
}

var statementsStatsFieldToGRPC = map[pgmodels.StatementsStatsField]pgv1.GetStatementsStatsRequest_Field{
	pgmodels.StatementsStatsFieldCalls:             pgv1.GetStatementsStatsRequest_CALLS,
	pgmodels.StatementsStatsFieldTotalTime:         pgv1.GetStatementsStatsRequest_TOTAL_TIME,
	pgmodels.StatementsStatsFieldMinTime:           pgv1.GetStatementsStatsRequest_MIN_TIME,
	pgmodels.StatementsStatsFieldMaxTime:           pgv1.GetStatementsStatsRequest_MAX_TIME,
	pgmodels.StatementsStatsFieldMeanTime:          pgv1.GetStatementsStatsRequest_MEAN_TIME,
	pgmodels.StatementsStatsFieldStddevTime:        pgv1.GetStatementsStatsRequest_STDDEV_TIME,
	pgmodels.StatementsStatsFieldRows:              pgv1.GetStatementsStatsRequest_ROWS,
	pgmodels.StatementsStatsFieldSharedBlksHit:     pgv1.GetStatementsStatsRequest_SHARED_BLKS_HIT,
	pgmodels.StatementsStatsFieldSharedBlksRead:    pgv1.GetStatementsStatsRequest_SHARED_BLKS_READ,
	pgmodels.StatementsStatsFieldSharedBlksDirtied: pgv1.GetStatementsStatsRequest_SHARED_BLKS_DIRTIED,
	pgmodels.StatementsStatsFieldSharedBlksWritten: pgv1.GetStatementsStatsRequest_SHARED_BLKS_WRITTEN,
	pgmodels.StatementsStatsFieldBlkReadTime:       pgv1.GetStatementsStatsRequest_BLK_READ_TIME,
	pgmodels.StatementsStatsFieldBlkWriteTime:      pgv1.GetStatementsStatsRequest_BLK_WRITE_TIME,
	pgmodels.StatementsStatsFieldTempBlksRead:      pgv1.GetStatementsStatsRequest_TEMP_BLKS_READ,
	pgmodels.StatementsStatsFieldTempBlksWritten:   pgv1.GetStatementsStatsRequest_TEMP_BLKS_WRITTEN,
	pgmodels.StatementsStatsFieldReads:             pgv1.GetStatementsStatsRequest_READS,
	pgmodels.StatementsStatsFieldWrites:            pgv1.GetStatementsStatsRequest_WRITES,
	pgmodels.StatementsStatsFieldUserTime:          pgv1.GetStatementsStatsRequest_USER_TIME,
	pgmodels.StatementsStatsFieldSystemTime:        pgv1.GetStatementsStatsRequest_SYSTEM_TIME,
}
var statementsStatsFieldFromGRPC = reflectutil.ReverseMap(statementsStatsFieldToGRPC).(map[pgv1.GetStatementsStatsRequest_Field]pgmodels.StatementsStatsField)

var statementStatsFieldToGRPC = map[pgmodels.StatementsStatsField]pgv1.GetStatementStatsRequest_Field{
	pgmodels.StatementsStatsFieldCalls:             pgv1.GetStatementStatsRequest_CALLS,
	pgmodels.StatementsStatsFieldTotalTime:         pgv1.GetStatementStatsRequest_TOTAL_TIME,
	pgmodels.StatementsStatsFieldMinTime:           pgv1.GetStatementStatsRequest_MIN_TIME,
	pgmodels.StatementsStatsFieldMaxTime:           pgv1.GetStatementStatsRequest_MAX_TIME,
	pgmodels.StatementsStatsFieldMeanTime:          pgv1.GetStatementStatsRequest_MEAN_TIME,
	pgmodels.StatementsStatsFieldStddevTime:        pgv1.GetStatementStatsRequest_STDDEV_TIME,
	pgmodels.StatementsStatsFieldRows:              pgv1.GetStatementStatsRequest_ROWS,
	pgmodels.StatementsStatsFieldSharedBlksHit:     pgv1.GetStatementStatsRequest_SHARED_BLKS_HIT,
	pgmodels.StatementsStatsFieldSharedBlksRead:    pgv1.GetStatementStatsRequest_SHARED_BLKS_READ,
	pgmodels.StatementsStatsFieldSharedBlksDirtied: pgv1.GetStatementStatsRequest_SHARED_BLKS_DIRTIED,
	pgmodels.StatementsStatsFieldSharedBlksWritten: pgv1.GetStatementStatsRequest_SHARED_BLKS_WRITTEN,
	pgmodels.StatementsStatsFieldBlkReadTime:       pgv1.GetStatementStatsRequest_BLK_READ_TIME,
	pgmodels.StatementsStatsFieldBlkWriteTime:      pgv1.GetStatementStatsRequest_BLK_WRITE_TIME,
	pgmodels.StatementsStatsFieldTempBlksRead:      pgv1.GetStatementStatsRequest_TEMP_BLKS_READ,
	pgmodels.StatementsStatsFieldTempBlksWritten:   pgv1.GetStatementStatsRequest_TEMP_BLKS_WRITTEN,
	pgmodels.StatementsStatsFieldReads:             pgv1.GetStatementStatsRequest_READS,
	pgmodels.StatementsStatsFieldWrites:            pgv1.GetStatementStatsRequest_WRITES,
	pgmodels.StatementsStatsFieldUserTime:          pgv1.GetStatementStatsRequest_USER_TIME,
	pgmodels.StatementsStatsFieldSystemTime:        pgv1.GetStatementStatsRequest_SYSTEM_TIME,
}
var statementStatsFieldFromGRPC = reflectutil.ReverseMap(statementStatsFieldToGRPC).(map[pgv1.GetStatementStatsRequest_Field]pgmodels.StatementsStatsField)

var sessionAtTimeFieldToGRPC = map[pgmodels.PGStatActivityColumn]pgv1.GetSessionsAtTimeRequest_Field{
	pgmodels.PGStatActivityTime:            pgv1.GetSessionsAtTimeRequest_TIME,
	pgmodels.PGStatActivityHost:            pgv1.GetSessionsAtTimeRequest_HOST,
	pgmodels.PGStatActivityDatabase:        pgv1.GetSessionsAtTimeRequest_DATABASE,
	pgmodels.PGStatActivityUser:            pgv1.GetSessionsAtTimeRequest_USER,
	pgmodels.PGStatActivityWaitEventType:   pgv1.GetSessionsAtTimeRequest_WAIT_EVENT_TYPE,
	pgmodels.PGStatActivityWaitEvent:       pgv1.GetSessionsAtTimeRequest_WAIT_EVENT,
	pgmodels.PGStatActivityState:           pgv1.GetSessionsAtTimeRequest_STATE,
	pgmodels.PGStatActivityQuery:           pgv1.GetSessionsAtTimeRequest_QUERY,
	pgmodels.PGStatActivityPid:             pgv1.GetSessionsAtTimeRequest_PID,
	pgmodels.PGStatActivityApplicationName: pgv1.GetSessionsAtTimeRequest_APPLICATION_NAME,
	pgmodels.PGStatActivityBackendStart:    pgv1.GetSessionsAtTimeRequest_BACKEND_START,
	pgmodels.PGStatActivityXactStart:       pgv1.GetSessionsAtTimeRequest_XACT_START,
	pgmodels.PGStatActivityStateChange:     pgv1.GetSessionsAtTimeRequest_STATE_CHANGE,
	pgmodels.PGStatActivityBackendType:     pgv1.GetSessionsAtTimeRequest_BACKEND_TYPE,
	pgmodels.PGStatActivityQueryStart:      pgv1.GetSessionsAtTimeRequest_QUERY_START,
}
var sessionAtTimeFieldFromGRPC = reflectutil.ReverseMap(sessionAtTimeFieldToGRPC).(map[pgv1.GetSessionsAtTimeRequest_Field]pgmodels.PGStatActivityColumn)

var statementsAtTimeFieldToGRPC = map[pgmodels.PGStatStatementsColumn]pgv1.GetStatementsAtTimeRequest_Field{
	pgmodels.PGStatStatementsTime:              pgv1.GetStatementsAtTimeRequest_TIME,
	pgmodels.PGStatStatementsHost:              pgv1.GetStatementsAtTimeRequest_HOST,
	pgmodels.PGStatStatementsUser:              pgv1.GetStatementsAtTimeRequest_USER,
	pgmodels.PGStatStatementsDatabase:          pgv1.GetStatementsAtTimeRequest_DATABASE,
	pgmodels.PGStatStatementsQueryid:           pgv1.GetStatementsAtTimeRequest_QUERYID,
	pgmodels.PGStatStatementsQuery:             pgv1.GetStatementsAtTimeRequest_QUERY,
	pgmodels.PGStatStatementsCalls:             pgv1.GetStatementsAtTimeRequest_CALLS,
	pgmodels.PGStatStatementsTotalTime:         pgv1.GetStatementsAtTimeRequest_TOTAL_TIME,
	pgmodels.PGStatStatementsMinTime:           pgv1.GetStatementsAtTimeRequest_MIN_TIME,
	pgmodels.PGStatStatementsMaxTime:           pgv1.GetStatementsAtTimeRequest_MAX_TIME,
	pgmodels.PGStatStatementsMeanTime:          pgv1.GetStatementsAtTimeRequest_MEAN_TIME,
	pgmodels.PGStatStatementsStddevTime:        pgv1.GetStatementsAtTimeRequest_STDDEV_TIME,
	pgmodels.PGStatStatementsRows:              pgv1.GetStatementsAtTimeRequest_ROWS,
	pgmodels.PGStatStatementsSharedBlksHit:     pgv1.GetStatementsAtTimeRequest_SHARED_BLKS_HIT,
	pgmodels.PGStatStatementsSharedBlksRead:    pgv1.GetStatementsAtTimeRequest_SHARED_BLKS_READ,
	pgmodels.PGStatStatementsSharedBlksDirtied: pgv1.GetStatementsAtTimeRequest_SHARED_BLKS_DIRTIED,
	pgmodels.PGStatStatementsSharedBlksWritten: pgv1.GetStatementsAtTimeRequest_SHARED_BLKS_WRITTEN,
	pgmodels.PGStatStatementsBlkReadTime:       pgv1.GetStatementsAtTimeRequest_BLK_READ_TIME,
	pgmodels.PGStatStatementsBlkWriteTime:      pgv1.GetStatementsAtTimeRequest_BLK_WRITE_TIME,
	pgmodels.PGStatStatementsTempBlksRead:      pgv1.GetStatementsAtTimeRequest_TEMP_BLKS_READ,
	pgmodels.PGStatStatementsTempBlksWritten:   pgv1.GetStatementsAtTimeRequest_TEMP_BLKS_WRITTEN,
	pgmodels.PGStatStatementsReads:             pgv1.GetStatementsAtTimeRequest_READS,
	pgmodels.PGStatStatementsWrites:            pgv1.GetStatementsAtTimeRequest_WRITES,
	pgmodels.PGStatStatementsUserTime:          pgv1.GetStatementsAtTimeRequest_USER_TIME,
	pgmodels.PGStatStatementsSystemTime:        pgv1.GetStatementsAtTimeRequest_SYSTEM_TIME,
}
var statementsAtTimeFieldFromGRPC = reflectutil.ReverseMap(statementsAtTimeFieldToGRPC).(map[pgv1.GetStatementsAtTimeRequest_Field]pgmodels.PGStatStatementsColumn)

var statementsIntervalFieldToGRPC = map[pgmodels.PGStatStatementsColumn]pgv1.GetStatementsIntervalRequest_Field{
	pgmodels.PGStatStatementsTime:              pgv1.GetStatementsIntervalRequest_TIME,
	pgmodels.PGStatStatementsHost:              pgv1.GetStatementsIntervalRequest_HOST,
	pgmodels.PGStatStatementsUser:              pgv1.GetStatementsIntervalRequest_USER,
	pgmodels.PGStatStatementsDatabase:          pgv1.GetStatementsIntervalRequest_DATABASE,
	pgmodels.PGStatStatementsQueryid:           pgv1.GetStatementsIntervalRequest_QUERYID,
	pgmodels.PGStatStatementsQuery:             pgv1.GetStatementsIntervalRequest_QUERY,
	pgmodels.PGStatStatementsCalls:             pgv1.GetStatementsIntervalRequest_CALLS,
	pgmodels.PGStatStatementsTotalTime:         pgv1.GetStatementsIntervalRequest_TOTAL_TIME,
	pgmodels.PGStatStatementsMinTime:           pgv1.GetStatementsIntervalRequest_MIN_TIME,
	pgmodels.PGStatStatementsMaxTime:           pgv1.GetStatementsIntervalRequest_MAX_TIME,
	pgmodels.PGStatStatementsMeanTime:          pgv1.GetStatementsIntervalRequest_MEAN_TIME,
	pgmodels.PGStatStatementsStddevTime:        pgv1.GetStatementsIntervalRequest_STDDEV_TIME,
	pgmodels.PGStatStatementsRows:              pgv1.GetStatementsIntervalRequest_ROWS,
	pgmodels.PGStatStatementsSharedBlksHit:     pgv1.GetStatementsIntervalRequest_SHARED_BLKS_HIT,
	pgmodels.PGStatStatementsSharedBlksRead:    pgv1.GetStatementsIntervalRequest_SHARED_BLKS_READ,
	pgmodels.PGStatStatementsSharedBlksDirtied: pgv1.GetStatementsIntervalRequest_SHARED_BLKS_DIRTIED,
	pgmodels.PGStatStatementsSharedBlksWritten: pgv1.GetStatementsIntervalRequest_SHARED_BLKS_WRITTEN,
	pgmodels.PGStatStatementsBlkReadTime:       pgv1.GetStatementsIntervalRequest_BLK_READ_TIME,
	pgmodels.PGStatStatementsBlkWriteTime:      pgv1.GetStatementsIntervalRequest_BLK_WRITE_TIME,
	pgmodels.PGStatStatementsTempBlksRead:      pgv1.GetStatementsIntervalRequest_TEMP_BLKS_READ,
	pgmodels.PGStatStatementsTempBlksWritten:   pgv1.GetStatementsIntervalRequest_TEMP_BLKS_WRITTEN,
	pgmodels.PGStatStatementsReads:             pgv1.GetStatementsIntervalRequest_READS,
	pgmodels.PGStatStatementsWrites:            pgv1.GetStatementsIntervalRequest_WRITES,
	pgmodels.PGStatStatementsUserTime:          pgv1.GetStatementsIntervalRequest_USER_TIME,
	pgmodels.PGStatStatementsSystemTime:        pgv1.GetStatementsIntervalRequest_SYSTEM_TIME,
}
var statementsIntervalFieldFromGRPC = reflectutil.ReverseMap(statementsIntervalFieldToGRPC).(map[pgv1.GetStatementsIntervalRequest_Field]pgmodels.PGStatStatementsColumn)

var statementsDiffFieldToGRPC = map[pgmodels.PGStatStatementsColumn]pgv1.GetStatementsDiffRequest_Field{
	pgmodels.PGStatStatementsTime:              pgv1.GetStatementsDiffRequest_TIME,
	pgmodels.PGStatStatementsHost:              pgv1.GetStatementsDiffRequest_HOST,
	pgmodels.PGStatStatementsUser:              pgv1.GetStatementsDiffRequest_USER,
	pgmodels.PGStatStatementsDatabase:          pgv1.GetStatementsDiffRequest_DATABASE,
	pgmodels.PGStatStatementsQueryid:           pgv1.GetStatementsDiffRequest_QUERYID,
	pgmodels.PGStatStatementsQuery:             pgv1.GetStatementsDiffRequest_QUERY,
	pgmodels.PGStatStatementsCalls:             pgv1.GetStatementsDiffRequest_CALLS,
	pgmodels.PGStatStatementsTotalTime:         pgv1.GetStatementsDiffRequest_TOTAL_TIME,
	pgmodels.PGStatStatementsMinTime:           pgv1.GetStatementsDiffRequest_MIN_TIME,
	pgmodels.PGStatStatementsMaxTime:           pgv1.GetStatementsDiffRequest_MAX_TIME,
	pgmodels.PGStatStatementsMeanTime:          pgv1.GetStatementsDiffRequest_MEAN_TIME,
	pgmodels.PGStatStatementsStddevTime:        pgv1.GetStatementsDiffRequest_STDDEV_TIME,
	pgmodels.PGStatStatementsRows:              pgv1.GetStatementsDiffRequest_ROWS,
	pgmodels.PGStatStatementsSharedBlksHit:     pgv1.GetStatementsDiffRequest_SHARED_BLKS_HIT,
	pgmodels.PGStatStatementsSharedBlksRead:    pgv1.GetStatementsDiffRequest_SHARED_BLKS_READ,
	pgmodels.PGStatStatementsSharedBlksDirtied: pgv1.GetStatementsDiffRequest_SHARED_BLKS_DIRTIED,
	pgmodels.PGStatStatementsSharedBlksWritten: pgv1.GetStatementsDiffRequest_SHARED_BLKS_WRITTEN,
	pgmodels.PGStatStatementsBlkReadTime:       pgv1.GetStatementsDiffRequest_BLK_READ_TIME,
	pgmodels.PGStatStatementsBlkWriteTime:      pgv1.GetStatementsDiffRequest_BLK_WRITE_TIME,
	pgmodels.PGStatStatementsTempBlksRead:      pgv1.GetStatementsDiffRequest_TEMP_BLKS_READ,
	pgmodels.PGStatStatementsTempBlksWritten:   pgv1.GetStatementsDiffRequest_TEMP_BLKS_WRITTEN,
	pgmodels.PGStatStatementsReads:             pgv1.GetStatementsDiffRequest_READS,
	pgmodels.PGStatStatementsWrites:            pgv1.GetStatementsDiffRequest_WRITES,
	pgmodels.PGStatStatementsUserTime:          pgv1.GetStatementsDiffRequest_USER_TIME,
	pgmodels.PGStatStatementsSystemTime:        pgv1.GetStatementsDiffRequest_SYSTEM_TIME,
}
var statementsDiffFieldFromGRPC = reflectutil.ReverseMap(statementsDiffFieldToGRPC).(map[pgv1.GetStatementsDiffRequest_Field]pgmodels.PGStatStatementsColumn)

func StatementAtTimeToGRPC(statement pgmodels.Statements) *pgv1.Statement {
	v := &pgv1.Statement{
		Time:              grpcapi.TimeToGRPC(statement.Timestamp),
		Host:              statement.Host,
		User:              statement.User,
		Database:          statement.Database,
		Queryid:           statement.Queryid,
		Query:             statement.Query,
		Calls:             statement.Calls,
		TotalTime:         statement.TotalTime,
		MinTime:           statement.MinTime,
		MaxTime:           statement.MaxTime,
		MeanTime:          statement.MeanTime,
		StddevTime:        statement.StddevTime,
		Rows:              statement.Rows,
		SharedBlksHit:     statement.SharedBlksHit,
		SharedBlksRead:    statement.SharedBlksRead,
		SharedBlksDirtied: statement.SharedBlksDirtied,
		SharedBlksWritten: statement.SharedBlksWritten,
		BlkReadTime:       statement.BlkReadTime,
		BlkWriteTime:      statement.BlkWriteTime,
		TempBlksRead:      statement.TempBlksRead,
		TempBlksWritten:   statement.TempBlksWritten,
		Reads:             statement.Reads,
		Writes:            statement.Writes,
		UserTime:          statement.UserTime,
		SystemTime:        statement.SystemTime,
	}
	return v
}

func StatementsAtTimeToGRPC(sessions []pgmodels.Statements) []*pgv1.Statement {
	v := make([]*pgv1.Statement, 0, len(sessions))
	for _, session := range sessions {
		v = append(v, StatementAtTimeToGRPC(session))
	}
	return v
}

func StatementsStatsGRPC(statements []pgmodels.Statements) []*pgv1.Statement {
	v := make([]*pgv1.Statement, 0, len(statements))
	for _, statement := range statements {
		v = append(v, StatementAtTimeToGRPC(statement))
	}
	return v
}

func SessionStateToGRPC(state pgmodels.SessionState) *pgv1.SessionState {
	v := &pgv1.SessionState{
		Time:            grpcapi.TimeToGRPC(state.Timestamp),
		Host:            state.Host,
		Pid:             state.Pid,
		Database:        state.Database,
		User:            state.User,
		ApplicationName: state.ApplicationName,
		BackendStart:    grpcapi.OptionalTimeToGRPC(state.BackendStart),
		XactStart:       grpcapi.OptionalTimeToGRPC(state.XactStart),
		QueryStart:      grpcapi.OptionalTimeToGRPC(state.QueryStart),
		StateChange:     grpcapi.OptionalTimeToGRPC(state.StateChange),
		WaitEventType:   state.WaitEventType,
		WaitEvent:       state.WaitEvent,
		State:           state.State,
		Query:           state.Query,
		BackendType:     state.BackendType,
	}
	return v
}

func SessionsStateToGRPC(sessions []pgmodels.SessionState) []*pgv1.SessionState {
	v := make([]*pgv1.SessionState, 0, len(sessions))
	for _, session := range sessions {
		v = append(v, SessionStateToGRPC(session))
	}
	return v
}

func IntervalToGRPC(interval pgmodels.Interval) *pgv1.Interval {
	if !interval.StartTimestamp.Valid || !interval.EndTimestamp.Valid {
		return nil
	}
	v := &pgv1.Interval{
		StartTime: grpcapi.OptionalTimeToGRPC(interval.StartTimestamp),
		EndTime:   grpcapi.OptionalTimeToGRPC(interval.EndTimestamp),
	}
	return v
}

func DiffStatementToGRPC(diffStatement pgmodels.DiffStatement) *pgv1.DiffStatement {
	v := &pgv1.DiffStatement{
		FirstInterval:           IntervalToGRPC(diffStatement.FirstIntervalTime),
		SecondInterval:          IntervalToGRPC(diffStatement.SecondIntervalTime),
		Host:                    diffStatement.Host,
		User:                    diffStatement.User,
		Database:                diffStatement.Database,
		Queryid:                 diffStatement.Queryid,
		Query:                   diffStatement.Query,
		FirstCalls:              diffStatement.FirstCalls,
		SecondCalls:             diffStatement.SecondCalls,
		DiffCalls:               diffStatement.DiffCalls,
		FirstTotalTime:          diffStatement.FirstTotalTime,
		SecondTotalTime:         diffStatement.SecondTotalTime,
		FirstMinTime:            diffStatement.FirstMinTime,
		SecondMinTime:           diffStatement.SecondMinTime,
		FirstMaxTime:            diffStatement.FirstMaxTime,
		SecondMaxTime:           diffStatement.SecondMaxTime,
		FirstMeanTime:           diffStatement.FirstMeanTime,
		SecondMeanTime:          diffStatement.SecondMeanTime,
		FirstStddevTime:         diffStatement.FirstStddevTime,
		SecondStddevTime:        diffStatement.SecondStddevTime,
		FirstRows:               diffStatement.FirstRows,
		SecondRows:              diffStatement.SecondRows,
		FirstSharedBlksHit:      diffStatement.FirstSharedBlksHit,
		SecondSharedBlksHit:     diffStatement.SecondSharedBlksHit,
		FirstSharedBlksRead:     diffStatement.FirstSharedBlksRead,
		SecondSharedBlksRead:    diffStatement.SecondSharedBlksRead,
		FirstSharedBlksDirtied:  diffStatement.FirstSharedBlksDirtied,
		SecondSharedBlksDirtied: diffStatement.SecondSharedBlksDirtied,
		FirstSharedBlksWritten:  diffStatement.FirstSharedBlksWritten,
		SecondSharedBlksWritten: diffStatement.SecondSharedBlksWritten,
		FirstBlkReadTime:        diffStatement.FirstBlkReadTime,
		SecondBlkReadTime:       diffStatement.SecondBlkReadTime,
		FirstBlkWriteTime:       diffStatement.FirstBlkWriteTime,
		SecondBlkWriteTime:      diffStatement.SecondBlkWriteTime,
		FirstTempBlksRead:       diffStatement.FirstTempBlksRead,
		SecondTempBlksRead:      diffStatement.SecondTempBlksRead,
		FirstTempBlksWritten:    diffStatement.FirstTempBlksWritten,
		SecondTempBlksWritten:   diffStatement.SecondTempBlksWritten,
		FirstReads:              diffStatement.FirstReads,
		SecondReads:             diffStatement.SecondReads,
		FirstWrites:             diffStatement.FirstWrites,
		SecondWrites:            diffStatement.SecondWrites,
		FirstUserTime:           diffStatement.FirstUserTime,
		SecondUserTime:          diffStatement.SecondUserTime,
		FirstSystemTime:         diffStatement.FirstSystemTime,
		SecondSystemTime:        diffStatement.SecondSystemTime,
		DiffTotalTime:           diffStatement.DiffTotalTime,
		DiffMinTime:             diffStatement.DiffMinTime,
		DiffMaxTime:             diffStatement.DiffMaxTime,
		DiffMeanTime:            diffStatement.DiffMeanTime,
		DiffStddevTime:          diffStatement.DiffStddevTime,
		DiffRows:                diffStatement.DiffRows,
		DiffSharedBlksHit:       diffStatement.DiffSharedBlksHit,
		DiffSharedBlksRead:      diffStatement.DiffSharedBlksRead,
		DiffSharedBlksDirtied:   diffStatement.DiffSharedBlksDirtied,
		DiffSharedBlksWritten:   diffStatement.DiffSharedBlksWritten,
		DiffBlkReadTime:         diffStatement.DiffBlkReadTime,
		DiffBlkWriteTime:        diffStatement.DiffBlkWriteTime,
		DiffTempBlksRead:        diffStatement.DiffTempBlksRead,
		DiffTempBlksWritten:     diffStatement.DiffTempBlksWritten,
		DiffReads:               diffStatement.DiffReads,
		DiffWrites:              diffStatement.DiffWrites,
		DiffUserTime:            diffStatement.DiffUserTime,
		DiffSystemTime:          diffStatement.DiffSystemTime,
	}
	return v
}

func StatementsDiffToGRPC(diffStatements []pgmodels.DiffStatement) []*pgv1.DiffStatement {
	v := make([]*pgv1.DiffStatement, 0, len(diffStatements))
	for _, diffStatement := range diffStatements {
		v = append(v, DiffStatementToGRPC(diffStatement))
	}
	return v
}

var sortOrderToGRPC = map[pgmodels.SortOrder]pgv1.GetSessionsStatsRequest_SortOrder{
	pgmodels.OrderByAsc:  pgv1.GetSessionsStatsRequest_ASC,
	pgmodels.OrderByDesc: pgv1.GetSessionsStatsRequest_DESC,
}
var sortOrderFromGRPC = reflectutil.ReverseMap(sortOrderToGRPC).(map[pgv1.GetSessionsStatsRequest_SortOrder]pgmodels.SortOrder)

var sortOrderSessionAtTimeToGRPC = map[pgmodels.SortOrder]pgv1.GetSessionsAtTimeRequest_SortOrder{
	pgmodels.OrderByAsc:  pgv1.GetSessionsAtTimeRequest_ASC,
	pgmodels.OrderByDesc: pgv1.GetSessionsAtTimeRequest_DESC,
}
var sortOrderSessionAtTimeFromGRPC = reflectutil.ReverseMap(sortOrderSessionAtTimeToGRPC).(map[pgv1.GetSessionsAtTimeRequest_SortOrder]pgmodels.SortOrder)

var sortOrderStatementsAtTimeToGRPC = map[pgmodels.SortOrder]pgv1.GetStatementsAtTimeRequest_SortOrder{
	pgmodels.OrderByAsc:  pgv1.GetStatementsAtTimeRequest_ASC,
	pgmodels.OrderByDesc: pgv1.GetStatementsAtTimeRequest_DESC,
}
var sortOrderStatementsAtTimeFromGRPC = reflectutil.ReverseMap(sortOrderStatementsAtTimeToGRPC).(map[pgv1.GetStatementsAtTimeRequest_SortOrder]pgmodels.SortOrder)

var sortOrderStatementsDiffToGRPC = map[pgmodels.SortOrder]pgv1.GetStatementsDiffRequest_SortOrder{
	pgmodels.OrderByAsc:  pgv1.GetStatementsDiffRequest_ASC,
	pgmodels.OrderByDesc: pgv1.GetStatementsDiffRequest_DESC,
}
var sortOrderStatementsDiffFromGRPC = reflectutil.ReverseMap(sortOrderStatementsDiffToGRPC).(map[pgv1.GetStatementsDiffRequest_SortOrder]pgmodels.SortOrder)

var sortOrderStatementsIntervalToGRPC = map[pgmodels.SortOrder]pgv1.GetStatementsIntervalRequest_SortOrder{
	pgmodels.OrderByAsc:  pgv1.GetStatementsIntervalRequest_ASC,
	pgmodels.OrderByDesc: pgv1.GetStatementsIntervalRequest_DESC,
}
var sortOrderStatementsIntervalFromGRPC = reflectutil.ReverseMap(sortOrderStatementsIntervalToGRPC).(map[pgv1.GetStatementsIntervalRequest_SortOrder]pgmodels.SortOrder)

func SessionAtTimeOrderByFromGRPC(orderBy []*pgv1.GetSessionsAtTimeRequest_OrderBy) []pgmodels.OrderBySessionsAtTime {
	res := make([]pgmodels.OrderBySessionsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, pgmodels.OrderBySessionsAtTime{
			Field:     sessionAtTimeFieldFromGRPC[order.Field],
			SortOrder: sortOrderSessionAtTimeFromGRPC[order.Order],
		})
	}
	return res
}

func StatementsAtTimeOrderByFromGRPC(orderBy []*pgv1.GetStatementsAtTimeRequest_OrderBy) []pgmodels.OrderByStatementsAtTime {
	res := make([]pgmodels.OrderByStatementsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, pgmodels.OrderByStatementsAtTime{
			Field:     statementsAtTimeFieldFromGRPC[order.Field],
			SortOrder: sortOrderStatementsAtTimeFromGRPC[order.Order],
		})
	}
	return res
}

func StatementsIntervalOrderByFromGRPC(orderBy []*pgv1.GetStatementsIntervalRequest_OrderBy) []pgmodels.OrderByStatementsAtTime {
	res := make([]pgmodels.OrderByStatementsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, pgmodels.OrderByStatementsAtTime{
			Field:     statementsIntervalFieldFromGRPC[order.Field],
			SortOrder: sortOrderStatementsIntervalFromGRPC[order.Order],
		})
	}
	return res
}

func StatementsDiffOrderByFromGRPC(orderBy []*pgv1.GetStatementsDiffRequest_OrderBy) []pgmodels.OrderByStatementsAtTime {
	res := make([]pgmodels.OrderByStatementsAtTime, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, pgmodels.OrderByStatementsAtTime{
			Field:     statementsDiffFieldFromGRPC[order.Field],
			SortOrder: sortOrderStatementsDiffFromGRPC[order.Order],
		})
	}
	return res
}

func OrderByFromGRPC(orderBy []*pgv1.GetSessionsStatsRequest_OrderBy) []pgmodels.OrderBy {
	res := make([]pgmodels.OrderBy, 0, len(orderBy))
	for _, order := range orderBy {
		res = append(res, pgmodels.OrderBy{
			Field:     groupByFromGRPC[order.Field],
			SortOrder: sortOrderFromGRPC[order.Order],
		})
	}
	return res
}

func SessionColumnFilterFromGRPC(fields []pgv1.GetSessionsAtTimeRequest_Field) []pgmodels.PGStatActivityColumn {
	res := make([]pgmodels.PGStatActivityColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, sessionAtTimeFieldFromGRPC[field])
	}
	return res
}

func StatementsColumnFilterFromGRPC(fields []pgv1.GetStatementsAtTimeRequest_Field) []pgmodels.PGStatStatementsColumn {
	res := make([]pgmodels.PGStatStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsAtTimeFieldFromGRPC[field])
	}
	return res
}

func StatementsStatsColumnFilterFromGRPC(fields []pgv1.GetStatementsStatsRequest_Field) []pgmodels.StatementsStatsField {
	res := make([]pgmodels.StatementsStatsField, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsStatsFieldFromGRPC[field])
	}
	return res
}

func StatementStatsColumnFilterFromGRPC(fields []pgv1.GetStatementStatsRequest_Field) []pgmodels.StatementsStatsField {
	res := make([]pgmodels.StatementsStatsField, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementStatsFieldFromGRPC[field])
	}
	return res
}

func StatementsDiffColumnFilterFromGRPC(fields []pgv1.GetStatementsDiffRequest_Field) []pgmodels.PGStatStatementsColumn {
	res := make([]pgmodels.PGStatStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsDiffFieldFromGRPC[field])
	}
	return res
}

func StatementsIntervalColumnFilterFromGRPC(fields []pgv1.GetStatementsIntervalRequest_Field) []pgmodels.PGStatStatementsColumn {
	res := make([]pgmodels.PGStatStatementsColumn, 0, len(fields))
	for _, field := range fields {
		res = append(res, statementsIntervalFieldFromGRPC[field])
	}
	return res
}

func GetSessionsAtTimeFromGRPC(req *pgv1.GetSessionsAtTimeRequest) (pgmodels.GetSessionsAtTimeOptions, error) {
	var GetSessionsAtTimeArgs pgmodels.GetSessionsAtTimeOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return pgmodels.GetSessionsAtTimeOptions{}, err
	}
	GetSessionsAtTimeArgs.Filter = flt
	GetSessionsAtTimeArgs.Time = grpcapi.TimeFromGRPC(req.GetTime())
	GetSessionsAtTimeArgs.Limit = req.GetPageSize()
	GetSessionsAtTimeArgs.ColumnFilter = SessionColumnFilterFromGRPC(req.GetColumnFilter())
	GetSessionsAtTimeArgs.OrderBy = SessionAtTimeOrderByFromGRPC(req.GetOrderBy())
	return GetSessionsAtTimeArgs, nil
}

func GetSessionsStatsFromGRPC(req *pgv1.GetSessionsStatsRequest) (pgmodels.GetSessionsStatsOptions, error) {
	var sessionStatsArgs pgmodels.GetSessionsStatsOptions

	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return pgmodels.GetSessionsStatsOptions{}, err
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

func GetStatementsAtTimeFromGRPC(req *pgv1.GetStatementsAtTimeRequest) (pgmodels.GetStatementsAtTimeOptions, error) {
	var GetStatementsAtTimeArgs pgmodels.GetStatementsAtTimeOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return pgmodels.GetStatementsAtTimeOptions{}, err
	}
	GetStatementsAtTimeArgs.Filter = flt
	GetStatementsAtTimeArgs.Time = grpcapi.TimeFromGRPC(req.GetTime())
	GetStatementsAtTimeArgs.Limit = req.GetPageSize()
	GetStatementsAtTimeArgs.ColumnFilter = StatementsColumnFilterFromGRPC(req.GetColumnFilter())
	GetStatementsAtTimeArgs.OrderBy = StatementsAtTimeOrderByFromGRPC(req.GetOrderBy())
	return GetStatementsAtTimeArgs, nil
}

func GetStatementsStatsFromGRPC(req *pgv1.GetStatementsStatsRequest) (pgmodels.GetStatementsStatsOptions, error) {
	var statementsStatsArgs pgmodels.GetStatementsStatsOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return pgmodels.GetStatementsStatsOptions{}, err
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

func GetStatementStatsFromGRPC(req *pgv1.GetStatementStatsRequest) (pgmodels.GetStatementStatsOptions, error) {
	var statementStatsArgs pgmodels.GetStatementStatsOptions
	statementStatsArgs.Queryid = req.GetQueryId()
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return pgmodels.GetStatementStatsOptions{}, err
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

func GetStatementsDiffFromGRPC(req *pgv1.GetStatementsDiffRequest) (pgmodels.GetStatementsDiffOptions, error) {
	var GetStatementsDiffOptions pgmodels.GetStatementsDiffOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return pgmodels.GetStatementsDiffOptions{}, err
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

func GetStatementsIntervalFromGRPC(req *pgv1.GetStatementsIntervalRequest) (pgmodels.GetStatementsIntervalOptions, error) {
	var GetStatementsIntervalArgs pgmodels.GetStatementsIntervalOptions
	flt, err := grpcapi.FilterFromGRPC(req.GetFilter())
	if err != nil {
		return pgmodels.GetStatementsIntervalOptions{}, err
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
