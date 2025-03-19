package clickhouse

import (
	"context"
	"math"
	"sort"
	"time"

	// ClickHouse driver
	_ "github.com/ClickHouse/clickhouse-go"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func DefaultConfig() chutil.Config {
	return chutil.Config{
		DB:       "mdb",
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

var ErrGroupByRequired = xerrors.NewSentinel("orderBy should contain at least one field")

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

func (b *Backend) SessionsStats(ctx context.Context,
	cid string, limit, offset int64,
	fromTS, toTS time.Time, rollupPeriod int64,
	groupBy []mymodels.MySessionsColumn, orderBy []mymodels.OrderBy, filter []sqlfilter.Term,
) (res []mymodels.SessionsStats, more bool, err error) {
	if len(groupBy) == 0 {
		return nil, false, ErrGroupByRequired
	}
	query, whereParams, err := BuildSessionsStatsQuery(filter, groupBy, orderBy, rollupPeriod)
	if err != nil {
		return nil, false, err
	}

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
	for _, group := range groupBy {
		if group == mymodels.MySessionsTime {
			queryParams["rollup_period"] = rollupPeriod
		}
	}
	for k, v := range whereParams {
		queryParams[k] = v
	}
	var resps []mymodels.SessionsStats

	nextMessageToken := offset

	parser := func(rows *sqlx.Rows) error {
		var ss SessionStats
		err = rows.StructScan(&ss)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		ts := optional.Time{}
		if ss.Time.Valid {
			ts = optional.Time{Valid: true, Time: time.Unix(ss.Time.Int64, 0)}
		}

		nextMessageToken++
		resps = append(resps, mymodels.SessionsStats{
			Timestamp:        ts,
			Dimensions:       ss.makeDimensionsMap(),
			SessionsCount:    ss.SessionCount,
			NextMessageToken: nextMessageToken,
		})
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

func (b *Backend) StatementsStats(ctx context.Context,
	cid string, limit, offset int64,
	fromTS, toTS time.Time,
	groupBy []mymodels.StatementsStatsGroupBy,
	filter []sqlfilter.Term,
	columnFilter []mymodels.MyStatementsColumn,
) (stats mymodels.StatementsStats, more bool, err error) {

	query, whereParams, err := BuildStatementsStatsQuery(filter, columnFilter, groupBy)
	if err != nil {
		return mymodels.StatementsStats{}, false, err
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

	nextMessageToken := offset
	var statements []mymodels.Statements
	parser := func(rows *sqlx.Rows) error {
		var st Statements
		err := rows.StructScan(&st)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		statements = append(statements, st.toExt())
		nextMessageToken++
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
	if err != nil {
		return mymodels.StatementsStats{}, false, xerrors.Errorf("failed to execute query %s: %s", query.Name, err)
	}

	more, nextMessageToken = checkMore(len(statements), nextMessageToken, limit)

	if more {
		// Remove additional data if there is any
		statements = statements[:len(statements)-1]
	}

	return mymodels.StatementsStats{
		Statements:       statements,
		NextMessageToken: nextMessageToken,
	}, more, nil
}

func (b *Backend) StatementStats(ctx context.Context,
	cid string, digest string, limit, offset int64,
	fromTS, toTS time.Time,
	groupBy []mymodels.StatementsStatsGroupBy,
	filter []sqlfilter.Term,
	columnFilter []mymodels.MyStatementsColumn,
) (stats mymodels.StatementStats, more bool, err error) {

	query, whereParams, err := BuildStatementStatsQuery(filter, columnFilter, groupBy)
	if err != nil {
		return mymodels.StatementStats{}, false, err
	}

	queryParams := map[string]interface{}{
		"cid":       cid,
		"digest":    digest,
		"from_time": fromTS.Unix(),
		"to_time":   toTS.Unix(),
		"offset":    offset,
		"limit":     limit + 1, // Add 1 to limit to check if there is more data
	}

	for k, v := range whereParams {
		queryParams[k] = v
	}

	nextMessageToken := offset
	var statements []mymodels.Statements
	var q string
	parser := func(rows *sqlx.Rows) error {
		var st Statements
		err := rows.StructScan(&st)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		q, st.Query = st.Query, ""
		statements = append(statements, st.toExt())
		nextMessageToken++
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
	if err != nil {
		return mymodels.StatementStats{}, false, xerrors.Errorf("failed to execute query %s: %s", query.Name, err)
	}

	more, nextMessageToken = checkMore(len(statements), nextMessageToken, limit)

	if more {
		// Remove additional data if there is any
		statements = statements[:len(statements)-1]
	}

	return mymodels.StatementStats{
		Query:            q,
		Statements:       statements,
		NextMessageToken: nextMessageToken,
	}, more, nil
}

func (b *Backend) StatementsAtTime(
	ctx context.Context,
	cid string,
	limit, offset int64,
	ts time.Time,
	columnFilter []mymodels.MyStatementsColumn,
	filter []sqlfilter.Term,
	orderBy []mymodels.OrderByStatementsAtTime,
) (stats mymodels.StatementsAtTime, more bool, err error) {
	fQuery, fWhereParams, err := BuildFindClosestDatesQuery("queryFindStatementsClosestDate", filter)
	if err != nil {
		return mymodels.StatementsAtTime{}, false, xerrors.Errorf("failed to prepare query %s: %w", fQuery.Name, err)
	}

	currentTS, nextTS, previousTS, err := b.findClosestTime(ctx, cid, ts, fQuery, fWhereParams)
	if err != nil {
		return mymodels.StatementsAtTime{}, false, xerrors.Errorf("failed to prepare query %s: %w", fQuery.Name, err)
	}

	if currentTS == 0 {
		return mymodels.StatementsAtTime{}, false, nil
	}

	query, whereParams, err := BuildStatementsAtTimeQuery(filter, columnFilter, orderBy)
	if err != nil {
		return mymodels.StatementsAtTime{}, false, err
	}

	queryParams := map[string]interface{}{
		"cid":    cid,
		"ts":     currentTS,
		"offset": offset,
		"limit":  limit + 1, // Add 1 to limit to check if there is more data
	}

	for k, v := range whereParams {
		queryParams[k] = v
	}

	nextMessageToken := offset
	var statements []mymodels.Statements
	parser := func(rows *sqlx.Rows) error {
		var st Statements
		err := rows.StructScan(&st)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		statements = append(statements, st.toExt())
		nextMessageToken++
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
	if err != nil {
		return mymodels.StatementsAtTime{}, false, xerrors.Errorf("failed to execute query %s: %s", query.Name, err)
	}

	more, nextMessageToken = checkMore(len(statements), nextMessageToken, limit)

	if more {
		// Remove additional data if there is any
		statements = statements[:len(statements)-1]
	}

	return mymodels.StatementsAtTime{
		Statements:          statements,
		NextCollectTime:     time.Unix(nextTS, 0),
		NextMessageToken:    nextMessageToken,
		PreviousCollectTime: time.Unix(previousTS, 0),
	}, more, nil
}

func (b *Backend) SessionsAtTime(
	ctx context.Context,
	cid string,
	limit, offset int64,
	ts time.Time,
	columnFilter []mymodels.MySessionsColumn,
	filter []sqlfilter.Term,
	orderBy []mymodels.OrderBySessionsAtTime,
) (stats mymodels.SessionsAtTime, more bool, err error) {
	fQuery, fWhereParams, err := BuildFindClosestDatesQuery("queryFindSessionsClosestDate", filter)
	if err != nil {
		return mymodels.SessionsAtTime{}, false, xerrors.Errorf("failed to build query %s: %w", fQuery.Name, err)
	}
	currentTS, nextTS, previousTS, err := b.findClosestTime(ctx, cid, ts, fQuery, fWhereParams)
	if err != nil {
		return mymodels.SessionsAtTime{}, false, xerrors.Errorf("failed to find closest time query %s: %w", fQuery.Name, err)
	}

	if currentTS == 0 {
		return mymodels.SessionsAtTime{}, false, nil
	}

	query, whereParams, err := BuildSessionsAtTimeQuery(filter, columnFilter, orderBy)
	if err != nil {
		return mymodels.SessionsAtTime{}, false, err
	}

	queryParams := map[string]interface{}{
		"cid":    cid,
		"ts":     currentTS,
		"offset": offset,
		"limit":  limit + 1, // Add 1 to limit to check if there is more data
	}

	for k, v := range whereParams {
		queryParams[k] = v
	}

	nextMessageToken := offset
	var sessions []mymodels.SessionState
	parser := func(rows *sqlx.Rows) error {
		var s SessionState
		err := rows.StructScan(&s)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		sessions = append(sessions, s.toExt())
		nextMessageToken++
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
	if err != nil {
		return mymodels.SessionsAtTime{}, false, err
	}
	more, nextMessageToken = checkMore(len(sessions), nextMessageToken, limit)
	if more {
		// Remove additional data if there is any
		sessions = sessions[:len(sessions)-1]
	}

	return mymodels.SessionsAtTime{
		Sessions:            sessions,
		NextCollectTime:     time.Unix(nextTS, 0),
		NextMessageToken:    nextMessageToken,
		PreviousCollectTime: time.Unix(previousTS, 0),
	}, more, nil
}

func (b *Backend) StatementsDiff(
	ctx context.Context,
	cid string,
	limit, offset int64,
	firstIntervalStart, secondIntervalStart time.Time,
	intervalsDuration int64,
	columnFilter []mymodels.MyStatementsColumn,
	filter []sqlfilter.Term,
	orderBy []mymodels.OrderByStatementsAtTime,
) (stats mymodels.DiffStatements, more bool, err error) {

	query, whereParams, err := BuildStatementsDiffQuery(filter, columnFilter, orderBy)
	if err != nil {
		return mymodels.DiffStatements{}, false, err
	}

	queryParams := map[string]interface{}{
		"cid":             cid,
		"first_ts_start":  firstIntervalStart.Unix(),
		"first_ts_end":    firstIntervalStart.Unix() + intervalsDuration,
		"second_ts_start": secondIntervalStart.Unix(),
		"second_ts_end":   secondIntervalStart.Unix() + intervalsDuration,
		"offset":          offset,
		"limit":           limit + 1, // Add 1 to limit to check if there is more data
	}

	for k, v := range whereParams {
		queryParams[k] = v
	}

	nextMessageToken := offset
	var diffStatements []mymodels.DiffStatement
	parser := func(rows *sqlx.Rows) error {
		var s DiffStatement
		err := rows.StructScan(&s)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		diffStatements = append(diffStatements, s.toExt())
		nextMessageToken++
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

	if err != nil {
		return mymodels.DiffStatements{}, false, err
	}

	more, nextMessageToken = checkMore(len(diffStatements), nextMessageToken, limit)

	if more {
		// Remove additional data if there is any
		diffStatements = diffStatements[:len(diffStatements)-1]
	}

	return mymodels.DiffStatements{
		DiffStatements:   diffStatements,
		NextMessageToken: nextMessageToken,
	}, more, nil
}

func (b *Backend) StatementsInterval(
	ctx context.Context,
	cid string,
	limit, offset int64,
	fromTS, toTS time.Time,
	columnFilter []mymodels.MyStatementsColumn,
	filter []sqlfilter.Term,
	orderBy []mymodels.OrderByStatementsAtTime,
) (stats mymodels.StatementsInterval, more bool, err error) {

	query, whereParams, err := BuildStatementsIntervalQuery(filter, columnFilter, orderBy)
	if err != nil {
		return mymodels.StatementsInterval{}, false, err
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

	nextMessageToken := offset
	var statements []mymodels.Statements
	parser := func(rows *sqlx.Rows) error {
		var st struct {
			Timestamp time.Time `db:"collect_time_max"`
			Statements
		}
		err := rows.StructScan(&st)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		st.Statements.Timestamp = st.Timestamp
		statements = append(statements, st.toExt())
		nextMessageToken++
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
	if err != nil {
		return mymodels.StatementsInterval{}, false, xerrors.Errorf("failed to execute query %s: %s", query.Name, err)
	}

	more, nextMessageToken = checkMore(len(statements), nextMessageToken, limit)

	if more {
		// Remove additional data if there is any
		statements = statements[:len(statements)-1]
	}

	return mymodels.StatementsInterval{
		Statements:       statements,
		NextMessageToken: nextMessageToken,
	}, more, nil
}

func (b *Backend) findClosestTime(ctx context.Context, cid string, ts time.Time, query sqlutil.Stmt, WhereParams map[string]interface{}) (int64, int64, int64, error) {
	QueryParams := map[string]interface{}{
		"cid": cid,
		"ts":  ts.Unix(),
	}
	for k, v := range WhereParams {
		QueryParams[k] = v
	}

	var timestamps []int64
	parser := func(rows *sqlx.Rows) error {
		var snapshotTS int64
		err := rows.Scan(&snapshotTS)
		if err != nil {
			return xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
		}
		timestamps = append(timestamps, snapshotTS)
		return nil
	}

	_, err := sqlutil.QueryNode(
		ctx,
		b.node,
		query,
		QueryParams,
		parser,
		b.lg,
	)
	if err != nil {
		return 0, 0, 0, xerrors.Errorf("failed to retrieve query %s result: %w", "", err)
	}

	if len(timestamps) == 0 {
		return 0, 0, 0, nil
	}
	sort.Slice(timestamps, func(i, j int) bool { return timestamps[i] < timestamps[j] })
	closest := 0
	expectedTS := ts.Unix()
	for i := range timestamps {
		if math.Abs(float64(expectedTS-timestamps[i])) < math.Abs(float64(expectedTS-timestamps[closest])) {
			closest = i
		}
	}

	var previous int
	if closest == 0 {
		previous = 0
	} else {
		previous = closest - 1
	}

	return timestamps[closest], timestamps[len(timestamps)-1], timestamps[previous], nil
}

func checkMore(l int, nextMessageToken, limit int64) (bool, int64) {
	more := false
	if int64(l) > limit {
		more = true
		nextMessageToken--
	}
	return more, nextMessageToken
}
