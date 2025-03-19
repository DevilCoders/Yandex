package api

import (
	"encoding/base64"
	"encoding/json"
	"time"

	"google.golang.org/protobuf/types/known/timestamppb"

	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/logs/v1"
	commonv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
)

var (
	mapSourceTypeFromGRPC = map[apiv1.LogSourceType]models.LogSourceType{
		apiv1.LogSourceType_LOG_SOURCE_TYPE_CLICKHOUSE: models.LogSourceTypeClickhouse,
		apiv1.LogSourceType_LOG_SOURCE_TYPE_KAFKA:      models.LogSourceTypeKafka,
		apiv1.LogSourceType_LOG_SOURCE_TYPE_TRANSFER:   models.LogSourceTypeTransfer,
	}
	mapSourceTypeToGRPC = reflectutil.ReverseMap(mapSourceTypeFromGRPC).(map[models.LogSourceType]apiv1.LogSourceType)
)

func logSourcesFromGRPC(sources []*apiv1.LogSource) ([]models.LogSource, error) {
	res := make([]models.LogSource, 0, len(sources))
	for _, s := range sources {
		t, ok := mapSourceTypeFromGRPC[s.Type]
		if !ok {
			return nil, semerr.InvalidInputf("unsupported log source type: %q", s.Type.String())
		}

		res = append(res, models.LogSource{
			Type: t,
			ID:   s.GetId(),
		})
	}

	return res, nil
}

func optionalTimeFromGRPC(t *timestamppb.Timestamp) optional.Time {
	if t == nil {
		return optional.Time{}
	}

	return optional.NewTime(t.AsTime())
}

var (
	mapLevelFromGRPC = map[apiv1.LogLevel]models.LogLevel{
		apiv1.LogLevel_LOG_LEVEL_TRACE: models.LogLevelTrace,
		apiv1.LogLevel_LOG_LEVEL_DEBUG: models.LogLevelDebug,
		apiv1.LogLevel_LOG_LEVEL_INFO:  models.LogLevelInfo,
		apiv1.LogLevel_LOG_LEVEL_WARN:  models.LogLevelWarning,
		apiv1.LogLevel_LOG_LEVEL_ERROR: models.LogLevelError,
		apiv1.LogLevel_LOG_LEVEL_FATAL: models.LogLevelFatal,
	}
	mapLevelToGRPC = reflectutil.ReverseMap(mapLevelFromGRPC).(map[models.LogLevel]apiv1.LogLevel)
)

func levelsFromGRPC(levels []apiv1.LogLevel) ([]models.LogLevel, error) {
	res := make([]models.LogLevel, 0, len(levels))
	for _, l := range levels {
		t, ok := mapLevelFromGRPC[l]
		if !ok {
			return nil, semerr.InvalidInputf("unsupported log level: %q", l.String())
		}

		res = append(res, t)
	}

	return res, nil
}

func filtersFromGRPC(filter string) ([]sqlfilter.Term, error) {
	if filter != "" {
		terms, err := sqlfilter.Parse(filter)
		if err != nil {
			return nil, err
		}
		return terms, nil
	}
	return []sqlfilter.Term{}, nil
}

var mapOrderFromGRPC = map[apiv1.SortOrder]models.SortOrder{
	apiv1.SortOrder_SORT_ORDER_INVALID:    models.SortOrderAscending,
	apiv1.SortOrder_SORT_ORDER_ASCENDING:  models.SortOrderAscending,
	apiv1.SortOrder_SORT_ORDER_DESCENDING: models.SortOrderDescending,
}

func sortOrderFromGRPC(order apiv1.SortOrder) models.SortOrder {
	res, ok := mapOrderFromGRPC[order]
	if !ok {
		return models.SortOrderAscending
	}

	return res
}

func criteriaFromGRPC(criteria *apiv1.Criteria) (models.Criteria, string, error) {
	sources, err := logSourcesFromGRPC(criteria.Sources)
	if err != nil {
		return models.Criteria{}, "", err
	}

	levels, err := levelsFromGRPC(criteria.Levels)
	if err != nil {
		return models.Criteria{}, "", err
	}

	filters, err := filtersFromGRPC(criteria.Filter)
	if err != nil {
		return models.Criteria{}, "", err
	}

	return models.Criteria{
		Sources: sources,
		From:    optionalTimeFromGRPC(criteria.GetFromTime()),
		To:      optionalTimeFromGRPC(criteria.GetToTime()),
		Levels:  levels,
		Filters: filters,
		Order:   sortOrderFromGRPC(criteria.SortOrder),
		// TODO pagination
	}, criteria.Filter, nil
}

func logToGRPC(log models.Log) *apiv1.LogRecord {
	return &apiv1.LogRecord{
		Source: &apiv1.LogSource{
			Type: mapSourceTypeToGRPC[log.Source.Type],
			Id:   log.Source.ID,
		},
		Instance:   log.Instance.String,
		RecordTime: timestamppb.New(log.Timestamp),
		Level:      mapLevelToGRPC[log.Severity],
		Message:    log.Message,
	}
}

func logsToGRPC(logs []models.Log) []*apiv1.LogRecord {
	res := make([]*apiv1.LogRecord, 0, len(logs))
	for _, l := range logs {
		res = append(res, logToGRPC(l))
	}

	return res
}

type pageToken struct {
	Sources []models.LogSource `json:"sources"`
	From    *time.Time         `json:"from,omitempty"`
	To      *time.Time         `json:"to,omitempty"`
	Levels  []models.LogLevel  `json:"levels,omitempty"`
	Filter  string             `json:"filter,omitempty"`
	Order   models.SortOrder   `json:"order"`
	Limit   *int64             `json:"limit,omitempty"`
	Offset  int64              `json:"offset"`
}

func buildNextPage(criteria models.Criteria, filter string, offset int64) (*commonv1.NextPage, error) {
	token := pageToken{
		Sources: criteria.Sources,
		Levels:  criteria.Levels,
		Filter:  filter,
		Order:   criteria.Order,
		Offset:  offset,
	}

	if criteria.From.Valid {
		token.From = &criteria.From.Time
	}

	if criteria.To.Valid {
		token.To = &criteria.To.Time
	}

	if criteria.Limit.Valid {
		token.Limit = &criteria.Limit.Int64
	}

	data, err := json.Marshal(token)
	if err != nil {
		return nil, err
	}

	return &commonv1.NextPage{
		Token: base64.StdEncoding.EncodeToString(data),
	}, nil
}

func criteriaFromPaging(paging *commonv1.NextPage) (models.Criteria, string, error) {
	raw, err := base64.StdEncoding.DecodeString(paging.Token)
	if err != nil {
		return models.Criteria{}, "", err
	}

	var token pageToken
	if err := json.Unmarshal(raw, &token); err != nil {
		return models.Criteria{}, "", err
	}

	filters, err := filtersFromGRPC(token.Filter)
	if err != nil {
		return models.Criteria{}, "", err
	}
	result := models.Criteria{
		Sources: token.Sources,
		Levels:  token.Levels,
		Filters: filters,
		Order:   token.Order,
		Offset:  optional.NewInt64(token.Offset),
	}

	if token.Limit != nil {
		result.Limit = optional.NewInt64(*token.Limit)
	}

	if token.From != nil {
		result.From = optional.NewTime(*token.From)
	}

	if token.To != nil {
		result.To = optional.NewTime(*token.To)
	}

	return result, token.Filter, nil
}
