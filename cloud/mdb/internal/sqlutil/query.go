package sqlutil

import (
	"context"
	"fmt"
	"reflect"
	"strings"
	"time"

	"github.com/jmoiron/sqlx"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type parsingError struct {
	Err string
}

func (e *parsingError) Error() string {
	return e.Err
}

// NewParsingError constructs error that should be returned from parser when it failed to handle result but query should continue.
func NewParsingError(text string) error {
	return &parsingError{Err: text}
}

// NewParsingErrorf constructs error that should be returned from parser when it failed to handle result but query should continue.
// Does NOT use wrapped errors, only formats text string.
func NewParsingErrorf(format string, a ...interface{}) error {
	return NewParsingError(fmt.Sprintf(format, a...))
}

// RowParser must be able to parse sqlx.Rows and store results wherever it needs them to be
type RowParser func(*sqlx.Rows) error

// Stmt defines SQL query
type Stmt struct {
	// Name of the query (will appear in logs)
	Name string
	// Query itself (SQL code)
	Query string
}

// Format query string of statement returning new statement
func (s Stmt) Format(a ...interface{}) Stmt {
	return Stmt{
		Name:  s.Name,
		Query: fmt.Sprintf(s.Query, a...),
	}
}

type checkedConn struct {
	*sqlx.Conn
	driverName string
}

func (c checkedConn) DriverName() string {
	return c.driverName
}

func (c checkedConn) BindNamed(_ string, _ interface{}) (string, []interface{}, error) {
	return "", nil, nil
}

// QueryContext retrieves binding from context.
// If binding does not exist, it acquires node from NodeChooser.
// Then performs query on tx from binding or acquired node.
// Read queryExtContext for more.
func QueryContext(ctx context.Context, chooser NodeChooser, stmt Stmt, args map[string]interface{}, parser RowParser, l log.Logger, opts ...QueryOption) (int64, error) {
	binding, ok := TxBindingFrom(ctx)
	if !ok {
		node := chooser(ctx)
		if node == nil {
			return 0, semerr.Unavailable("unavailable")
		}

		return QueryNode(ctx, node, stmt, args, parser, l)
	}

	return QueryTxBinding(ctx, binding, stmt, args, parser, l, opts...)
}

// Query retrieves binding from context.
// If binding does not exist, it fails.
// Then performs query on tx.
// Read queryExtContext for more.
func QueryTx(ctx context.Context, stmt Stmt, args map[string]interface{}, parser RowParser, l log.Logger, opts ...QueryOption) (int64, error) {
	binding, ok := TxBindingFrom(ctx)
	if !ok {
		return 0, xerrors.New("no tx binding in context")
	}

	return QueryTxBinding(ctx, binding, stmt, args, parser, l, opts...)
}

// QueryBinding executes query on Node. Read queryExtContext for more.
func QueryNode(ctx context.Context, node Node, stmt Stmt, args map[string]interface{}, parser RowParser, lg log.Logger, opts ...QueryOption) (int64, error) {
	if node == nil {
		return 0, semerr.Unavailable("unavailable")
	}

	return queryExtContext(ctx, node.DBx(), node.Addr(), stmt, args, parser, lg, opts...)
}

// QueryBinding executes query on transaction stored in TxBinding. Read queryExtContext for more.
func QueryTxBinding(ctx context.Context, b TxBinding, stmt Stmt, args map[string]interface{}, parser RowParser, lg log.Logger, opts ...QueryOption) (int64, error) {
	if b.node == nil {
		return 0, semerr.Unavailable("unavailable")
	}

	var ectx sqlx.ExtContext
	if b.tx != nil {
		ectx = b.tx
	} else {
		ectx = b.node.DBx()
	}

	return queryExtContext(ctx, ectx, b.node.Addr(), stmt, args, parser, lg, opts...)
}

// QueryConn executes query on connection. Read queryExtContext for more.
// This is not for general purposes. Because a checkedConn wrapper is implemented without maybe useful func.
func QueryConn(ctx context.Context, c *sqlx.Conn, dn, addr string, stmt Stmt, args map[string]interface{}, parser RowParser, lg log.Logger, opts ...QueryOption) (int64, error) {
	if c == nil {
		return 0, semerr.Unavailable("unavailable")
	}
	return queryExtContext(ctx, checkedConn{Conn: c, driverName: dn}, addr, stmt, args, parser, lg, opts...)
}

// queryExtContext executes named query on sql context and handles results via provided parser
func queryExtContext(ctx context.Context, ectx sqlx.ExtContext, addr string, stmt Stmt, args map[string]interface{}, parser RowParser, lg log.Logger, opts ...QueryOption) (int64, error) {
	if ectx == nil {
		return 0, semerr.Unavailable("unavailable")
	}

	var qctx queryContext
	for _, opt := range opts {
		opt(qctx)
	}

	if qctx.ErrorWrapper == nil {
		qctx.ErrorWrapper = DefaultErrorWrapper
	}

	startTS := time.Now()
	var execDuration time.Duration
	handleError := func(logMsg, errMsg string, err error) error {
		ctxlog.Error(
			ctx,
			lg,
			logMsg,
			log.String("query_name", stmt.Name),
			log.String("query_body", stmt.Query),
			log.Reflect("query_args", args),
			log.String("db_instance", addr),
			log.Duration("exec_duration", execDuration),
			log.Duration("total_duration", time.Since(startTS)),
			log.Error(err),
		)

		err = xerrors.Errorf(errMsg+": %w", err)
		return qctx.ErrorWrapper(err)
	}

	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBQuery,
		tags.DBType.Tag("sql"),
		tags.DBInstance.Tag(addr),
		tags.DBStatementName.Tag(stmt.Name),
		tags.DBStatement.Tag(stmt.Query),
	)
	defer span.Finish()

	ctxlog.Debug(
		ctx,
		lg,
		"executing query",
		log.String("query_name", stmt.Name),
		log.String("db_instance", addr),
	)

	rows, err := namedQueryContext(ctx, qctx, ectx, addr, stmt, args)
	execDuration = time.Since(startTS)
	if err != nil {
		return 0, handleError("error while executing statement", "sql query", err)
	}
	defer func() { _ = rows.Close() }()

	count, err := parseRows(ctx, qctx, rows, addr, stmt, parser, handleError, lg)
	if err != nil {
		return 0, err
	}

	tags.DBQueryResultCount.Set(span, count)

	ctxlog.Debug(
		ctx,
		lg,
		"executed query",
		log.String("query_name", stmt.Name),
		log.String("db_instance", addr),
		log.Duration("exec_duration", execDuration),
		log.Duration("total_duration", time.Since(startTS)),
		log.Int64("rows_count", count),
	)

	return count, nil
}

func namedQueryContext(ctx context.Context, qctx queryContext, ectx sqlx.ExtContext, addr string, stmt Stmt, args map[string]interface{}) (*sqlx.Rows, error) {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBQueryExecute,
		tags.DBType.Tag("sql"),
		tags.DBInstance.Tag(addr),
		tags.DBStatementName.Tag(stmt.Name),
		tags.DBStatement.Tag(stmt.Query),
	)
	defer span.Finish()

	rows, err := sqlx.NamedQueryContext(ctx, ectx, stmt.Query, args)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		err = qctx.ErrorWrapper(err)
	}

	return rows, err
}

func parseRows(ctx context.Context, qctx queryContext, rows *sqlx.Rows, addr string, stmt Stmt, parser RowParser, handleError func(logMsg, errMsg string, err error) error, l log.Logger) (int64, error) {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBQueryResult,
		tags.DBType.Tag("sql"),
		tags.DBInstance.Tag(addr),
		tags.DBStatementName.Tag(stmt.Name),
	)
	defer span.Finish()

	var count int64
	for rows.Next() {
		count++
		if err := parser(rows); err != nil {
			var pErr *parsingError
			if xerrors.As(err, &pErr) {
				ctxlog.Error(
					ctx,
					l,
					"statement result parsing error (we will continue)",
					log.String("query_name", stmt.Name),
					log.String("db_instance", addr),
					log.Error(pErr),
				)
				continue
			}

			tracing.SetErrorOnSpan(span, err)
			err = qctx.ErrorWrapper(err)
			return count, handleError("unable to parse statement result", "scan sql result", err)
		}
	}
	if err := rows.Err(); err != nil {
		tracing.SetErrorOnSpan(span, err)
		err = qctx.ErrorWrapper(err)
		return count, handleError("error retrieving next row of result", "sql rows retrieval", err)
	}

	tags.DBQueryResultCount.Set(span, count)
	return count, nil
}

type ErrorWrapperFunc func(err error) error

func DefaultErrorWrapper(err error) error {
	return pgerrors.DefaultWrapper(err)
}

// NopParser is sql.Rows parser for Query func that does nothing
func NopParser(*sqlx.Rows) error { return nil }

// GetColumnsFromStruct get column names from struct. It panics if s not a struct.
func GetColumnsFromStruct(s interface{}) []string {
	st := reflect.TypeOf(s)
	if st.Kind() != reflect.Struct {
		panic(fmt.Sprintf("Got unexpected type. Should be a struct. Got %#v", s))
	}
	if st.NumField() == 0 {
		panic("Got struct without fields?")
	}
	columns := make([]string, st.NumField())
	for i := 0; i < st.NumField(); i++ {
		field := st.Field(i)
		dbTag := field.Tag.Get("db")
		if len(dbTag) == 0 {
			dbTag = field.Name
		}
		columns[i] = dbTag
	}
	return columns
}

// NewStmt wrap query columns and create new Stmt.
func NewStmt(name, query string, st interface{}) Stmt {
	columns := GetColumnsFromStruct(st)
	wrapped := fmt.Sprintf("SELECT %s FROM (%s) w", strings.Join(columns, ", "), query)
	return Stmt{
		Name:  name,
		Query: wrapped,
	}
}
