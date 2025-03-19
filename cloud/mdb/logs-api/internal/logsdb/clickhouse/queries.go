package clickhouse

import (
	"fmt"

	"cuelang.org/go/pkg/strings"
	"golang.org/x/exp/maps"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter/chencoder"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
)

var (
	selectClauseTemplate = `
SELECT
  :source_type AS source_type,
  %[1]s AS resource_id,
  %[2]s AS instance,
  toUnixTimestamp(%[3]s) AS seconds,
  %[4]s AS ms,
  %[6]s AS severity,
  %[7]s AS message
FROM %[5]s`
	baseWhereClauseTemplate = `
WHERE
  resource_id = :resource_id
  AND (seconds > :from_time OR (seconds = :from_time AND ms >= :from_ms) )
  AND (seconds < :to_time   OR (seconds = :to_time   AND ms <  :to_ms) )
`
	severityWhereClauseTemplate = `
AND %[1]s IN ( %[2]s )
`
	orderClauseTemplate = "ORDER BY resource_id ASC, seconds %[1]s, ms %[1]s "
	limitClause         = "LIMIT :offset, :limit"

	logsQuery = sqlutil.Stmt{
		Name:  "querySelectLogs",
		Query: "%s\n %s \n %s \n %s",
	}
)

var (
	severityModifiersMap = map[SeverityModifier]func(models.LogLevel) (string, error){
		DefaultModifier: func(level models.LogLevel) (string, error) {
			return string(level), nil
		},
		DataTransferModifier: func(level models.LogLevel) (string, error) {
			var modified string
			switch level {
			case models.LogLevelInfo:
				modified = "INFO"
			case models.LogLevelWarning:
				modified = "WARN"
			case models.LogLevelError:
				modified = "ERROR"
			case models.LogLevelDebug:
				modified = "DEBUG"
			default:
				return string(level), nil
			}
			return modified, nil
		},
	}
)

func BuildQuery(config DataOptions, criteria logsdb.Criteria) (sqlutil.Stmt, map[string]interface{}, error) {
	if len(criteria.Sources) != 1 {
		return sqlutil.Stmt{}, nil, semerr.Unavailable("multiple sources not implemented")
	}

	selectClause, selectArgs, err := buildSelectClause(config, criteria.Sources[0].Type)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	whereClause, whereArgs, err := buildWhereClause(config, criteria)
	if err != nil {
		return sqlutil.Stmt{}, nil, err
	}

	limitClause, limitArgs := buildLimitClause(criteria)

	queryArgs := map[string]interface{}{}
	for _, m := range []map[string]interface{}{selectArgs, whereArgs, limitArgs} {
		for k, v := range m {
			queryArgs[k] = v
		}
	}

	return logsQuery.Format(
		selectClause,
		whereClause,
		buildOrderClause(criteria.Order),
		limitClause,
	), queryArgs, nil
}

func buildSelectClause(config DataOptions, source models.LogSourceType) (string, map[string]interface{}, error) {
	queryParams, err := config.ParamsBySourceType(source)
	if err != nil {
		return "", nil, err
	}

	args := map[string]interface{}{
		"source_type": int32(source),
	}
	return fmt.Sprintf(selectClauseTemplate, queryParams...), args, nil
}

func buildWhereClause(config DataOptions, criteria logsdb.Criteria) (string, map[string]interface{}, error) {
	options := config.Resource[criteria.Sources[0].Type.Stringify()]

	whereClauseTemplate := baseWhereClauseTemplate
	args := map[string]interface{}{
		"resource_id": criteria.Sources[0].ID,
		"from_time":   criteria.FromSeconds,
		"to_time":     criteria.ToSeconds,
		"from_ms":     criteria.FromMS,
		"to_ms":       criteria.ToMS,
	}

	if len(criteria.Levels) > 0 {
		modifier := options.SeverityFilter.Modifier
		if modifier == "" {
			modifier = DefaultModifier
		}

		severityArgumentNames := make([]string, len(criteria.Levels))
		for i, level := range criteria.Levels {
			modifiedLevel, err := severityModifiersMap[modifier](level)
			if err != nil {
				return "", nil, err
			}
			severityValueArgumentName := fmt.Sprintf("severity_value_%d", i)
			severityArgumentNames[i] = fmt.Sprintf(":%s", severityValueArgumentName)
			args[severityValueArgumentName] = modifiedLevel
		}
		whereClauseTemplate += fmt.Sprintf(severityWhereClauseTemplate, options.SeverityFilter.Column, strings.Join(severityArgumentNames, " , "))
	}

	if len(criteria.Filters) > 0 {
		filterColumnMap := make(map[string]struct{})
		var dummy struct{}
		for _, column := range options.FilterColumns {
			filterColumnMap[column] = dummy
		}
		conditions, params, err := chencoder.EncodeFilterToClickhouseConditions("", filterColumnMap, criteria.Filters)
		if err != nil {
			return "", nil, err
		}
		maps.Copy(args, params)
		whereClauseTemplate += conditions
	}

	return whereClauseTemplate, args, nil
}

func buildOrderClause(order models.SortOrder) string {
	return fmt.Sprintf(orderClauseTemplate, order)
}

func buildLimitClause(criteria logsdb.Criteria) (string, map[string]interface{}) {
	args := map[string]interface{}{
		"limit":  criteria.Limit + 1, // Add 1 to limit to check if there is more data
		"offset": criteria.Offset,
	}
	return limitClause, args
}
