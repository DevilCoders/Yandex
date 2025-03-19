package clickhouse

import (
	"context"
	"time"

	// ClickHouse driver
	_ "github.com/ClickHouse/clickhouse-go"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/perfdiagdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func DefaultConfig() chutil.Config {
	return chutil.Config{
		DB:       "perfdiag",
		User:     "dbaas_reader",
		Secure:   true,
		Compress: true,
		CAFile:   "/opt/yandex/allCAs.pem",
	}
}

type Backend struct {
	lg   log.Logger
	node sqlutil.Node
}

var _ perfdiagdb.Backend = &Backend{}

func New(cfg chutil.Config, lg log.Logger) (*Backend, error) {
	db, err := sqlx.Open("clickhouse", cfg.URI())
	if err != nil {
		return nil, err
	}
	node := sqlutil.NewNode("some.ch.host", db)

	return NewWithDB(lg, node), nil
}

func NewWithDB(lg log.Logger, node sqlutil.Node) *Backend {
	return &Backend{
		lg:   lg,
		node: node,
	}
}

func (b *Backend) IsReady(ctx context.Context) error {
	if err := b.node.DBx().PingContext(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, "unavailable")
	}

	return nil
}

func (b *Backend) ProfilerStats(
	ctx context.Context,
	cid string,
	limit, offset int64,
	fromTS, toTS time.Time,
	rollupPeriod int64,
	aggregateBy mongomodels.ProfilerStatsColumn,
	aggregationFunction mongomodels.AggregationType,
	groupBy []mongomodels.ProfilerStatsGroupBy,
	topX int64,
	filter []sqlfilter.Term,
) ([]mongomodels.ProfilerStats, bool, error) {
	// Get TopX List
	query, whereParams, err := BuildProfilerTopXQuery(filter, aggregateBy, aggregationFunction, groupBy)
	if err != nil {
		return nil, false, xerrors.Errorf("failed to prepare query %s: %w", query.Name, err)
	}
	queryParams := map[string]interface{}{
		"cid":       cid,
		"from_time": fromTS.Unix(),
		"to_time":   toTS.Unix(),
		"limit":     topX,
	}
	for k, v := range whereParams {
		queryParams[k] = v
	}
	var topList []string

	topParser := func(rows *sqlx.Rows) error {
		var rec string
		err = rows.Scan(&rec)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", query.Name, err)
		}
		topList = append(topList, rec)
		return nil
	}
	_, err = sqlutil.QueryNode(
		ctx,
		b.node,
		query,
		queryParams,
		topParser,
		b.lg,
	)
	if len(topList) < 1 {
		// Ensure, that at least one element is present in topList
		topList = append(topList, "")
	}

	//Actial query
	query, whereParams, err = BuildProfilerStatsQuery(filter, aggregateBy, aggregationFunction, groupBy)
	if err != nil {
		return nil, false, xerrors.Errorf("failed to prepare query %s: %w", query.Name, err)
	}
	queryParams = map[string]interface{}{
		"rollup_period": rollupPeriod,
		"cid":           cid,
		"from_time":     fromTS.Unix(),
		"to_time":       toTS.Unix(),
		"offset":        offset,
		"limit":         limit + 1, // Add 1 to limit to check if there is more data
		"top_list":      topList,
	}
	for k, v := range whereParams {
		queryParams[k] = v
	}
	more := false
	var resps []mongomodels.ProfilerStats

	parser := func(rows *sqlx.Rows) error {
		var rec ProfilerStats
		err = rows.StructScan(&rec)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", query.Name, err)
		}
		resps = append(resps, rec.toExt())
		return nil
	}
	_, err = sqlutil.QueryNode(
		ctx,
		b.node,
		query,
		queryParams,
		parser,
		b.lg,
	)

	if int64(len(resps)) > limit {
		// Remove additional data if there is any
		more = true
		resps = resps[:len(resps)-1]
	}

	return resps, more, nil
}

func (b *Backend) ProfilerRecs(
	ctx context.Context,
	cid string,
	limit, offset int64,
	fromTS, toTS time.Time,
	requestForm string,
	hostname string,
) ([]mongomodels.ProfilerRecs, bool, error) {
	query, whereParams, err := BuildProfilerRecordsQuery()
	if err != nil {
		return nil, false, xerrors.Errorf("failed to prepare query %s: %w", query.Name, err)
	}
	queryParams := map[string]interface{}{
		"cid":          cid,
		"from_time":    fromTS.Unix(),
		"to_time":      toTS.Unix(),
		"hostname":     hostname,
		"request_form": requestForm,
		"offset":       offset,
		"limit":        limit + 1, // Add 1 to limit to check if there is more data
	}
	for k, v := range whereParams {
		queryParams[k] = v
	}
	more := false
	var resps []mongomodels.ProfilerRecs

	parser := func(rows *sqlx.Rows) error {
		var rec ProfilerRecord
		err = rows.StructScan(&rec)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		resps = append(resps, rec.toExt())
		return nil
	}
	_, err = sqlutil.QueryNode(
		ctx,
		b.node,
		query,
		queryParams,
		parser,
		b.lg,
	)

	if int64(len(resps)) > limit {
		// Remove additional data if there is any
		more = true
		resps = resps[:len(resps)-1]
	}

	return resps, more, nil
}

func (b *Backend) ProfilerTopForms(
	ctx context.Context,
	cid string,
	limit, offset int64,
	fromTS, toTS time.Time,
	aggregateBy mongomodels.ProfilerStatsColumn,
	aggregationFunction mongomodels.AggregationType,
	filter []sqlfilter.Term,
) ([]mongomodels.TopForms, bool, error) {
	query, whereParams, err := BuildTopFormsQuery(filter, aggregateBy, aggregationFunction)
	if err != nil {
		return nil, false, xerrors.Errorf("failed to prepare query %s: %w", query.Name, err)
	}
	queryParams := map[string]interface{}{
		"cid":       cid,
		"from_time": fromTS.Unix(),
		"to_time":   toTS.Unix(),
		"offset":    offset,
		"limit":     limit + 1, // Add 1 to limit to check if there is more data
	}
	for k, v := range whereParams {
		queryParams[k] = v
	}
	more := false
	var resps []mongomodels.TopForms

	parser := func(rows *sqlx.Rows) error {
		var rec TopForms
		err = rows.StructScan(&rec)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		resps = append(resps, rec.toExt())
		return nil
	}
	_, err = sqlutil.QueryNode(
		ctx,
		b.node,
		query,
		queryParams,
		parser,
		b.lg,
	)

	if int64(len(resps)) > limit {
		// Remove additional data if there is any
		more = true
		resps = resps[:len(resps)-1]
	}

	return resps, more, nil
}

func (b *Backend) PossibleIndexes(
	ctx context.Context,
	cid string,
	limit, offset int64,
	fromTS, toTS time.Time,
	filter []sqlfilter.Term,
) ([]mongomodels.PossibleIndexes, bool, error) {
	query, whereParams, err := BuildPossibleIndexesQuery(filter)
	if err != nil {
		return nil, false, xerrors.Errorf("failed to prepare query %s: %w", query.Name, err)
	}
	queryParams := map[string]interface{}{
		"cid":       cid,
		"from_time": fromTS.Unix(),
		"to_time":   toTS.Unix(),
		"offset":    offset,
		"limit":     limit + 1, // Add 1 to limit to check if there is more data
	}
	for k, v := range whereParams {
		queryParams[k] = v
	}
	more := false
	var resps []mongomodels.PossibleIndexes

	parser := func(rows *sqlx.Rows) error {
		var rec PossibleIndexes
		err = rows.StructScan(&rec)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		resps = append(resps, rec.toExt())
		return nil
	}
	_, err = sqlutil.QueryNode(
		ctx,
		b.node,
		query,
		queryParams,
		parser,
		b.lg,
	)

	if int64(len(resps)) > limit {
		// Remove additional data if there is any
		more = true
		resps = resps[:len(resps)-1]
	}

	return resps, more, nil
}
