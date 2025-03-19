package kikimr

import (
	"bytes"
	"database/sql"
	"database/sql/driver"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"

	otlog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"github.com/prometheus/client_golang/prometheus"
	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type kikimrReplacer func(context.Context, string, []interface{}) (string, []interface{})

var _ kikimrDB = sqlDB{}
var _ kikimrTx = sqlTx{}

// kikimrTx is used instead of sql.Tx wrapper for table name ($snapshots) substitution
type kikimrTx interface {
	Querier
	Commit() error
	Rollback() error
}

// Querier interface
type Querier interface {
	ExecContext(ctx context.Context, query string, args ...interface{}) (sql.Result, error)
	QueryContext(ctx context.Context, query string, args ...interface{}) (SQLRows, error)
	QueryRowContext(ctx context.Context, query string, args ...interface{}) SQLRow
}

type kikimrDB interface {
	BeginTx(ctx context.Context, opts *sql.TxOptions) (kikimrTx, error)
	Close() error
	PingContext(ctx context.Context) error
}

type SQLRows interface {
	Close() error
	Next() bool
	Scan(dest ...interface{}) error
	Err() error
}

type SQLRow interface {
	Scan(dest ...interface{}) error
	Err() error
}

type sqlTx struct {
	*sql.Tx
	queryReplace kikimrReplacer
	timer        *prometheus.Timer
	traceCtx     context.Context
}

func (tx sqlTx) ExecContext(ctx context.Context, query string, args ...interface{}) (res sql.Result, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	span.LogFields(otlog.String("query", query))

	t := misc.KikimrQueryTimer.Start()
	defer t.ObserveDuration()
	query, args = tx.queryReplace(ctx, query, args)
	res, err = tx.Tx.ExecContext(ctx, query, args...)
	errorToMetric(err)
	return res, err
}

func (tx sqlTx) QueryContext(ctx context.Context, query string, args ...interface{}) (res SQLRows, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	span.LogFields(otlog.String("query", query))

	t := misc.KikimrQueryTimer.Start()
	defer t.ObserveDuration()
	query, args = tx.queryReplace(ctx, query, args)
	res, err = tx.Tx.QueryContext(ctx, query, args...)
	errorToMetric(err)
	return res, err
}

func (tx sqlTx) QueryRowContext(ctx context.Context, query string, args ...interface{}) (row SQLRow) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, nil) }()

	span.LogFields(otlog.String("query", query))

	t := misc.KikimrQueryTimer.Start()
	defer t.ObserveDuration()
	query, args = tx.queryReplace(ctx, query, args)
	return tx.Tx.QueryRowContext(ctx, query, args...)
}

func (tx sqlTx) Commit() (err error) {
	span, _ := tracing.StartSpanFromContextFunc(tx.traceCtx)
	defer func() { tracing.Finish(span, err) }()

	t := misc.KikimrCommitQueryTimer.Start()
	err = tx.Tx.Commit()
	errorToMetric(err)
	tx.timer.ObserveDuration()
	misc.KikimrTransactions.Dec()
	if err == nil {
		t.ObserveDuration()
	}

	return
}

func (tx sqlTx) Rollback() (err error) {
	span, _ := tracing.StartSpanFromContextFunc(tx.traceCtx)
	defer func() { tracing.Finish(span, err) }()

	err = tx.Tx.Rollback()
	tx.timer.ObserveDuration()
	misc.KikimrTransactions.Dec()
	return
}

type sqlDB struct {
	*sql.DB
	queryReplace kikimrReplacer
}

func (db sqlDB) BeginTx(ctx context.Context, opts *sql.TxOptions) (kikimrTx kikimrTx, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()
	if opts != nil {
		span.LogFields(otlog.Bool("readonly", opts.ReadOnly), otlog.String("isolation_level", opts.Isolation.String()))
	}
	misc.KikimrTransactions.Inc()

	tx, err := db.DB.BeginTx(ctx, opts)
	return sqlTx{tx, db.queryReplace, misc.KikimrTransactionTimer.Start(), ctx}, err
}

func (db sqlDB) PingContext(ctx context.Context) error {
	return db.DB.PingContext(ctx)
}

func timeToUint64(t time.Time) uint64 {
	return uint64(t.UnixNano() / 1000)
}

type kikimrTime struct {
	v *time.Time
}

func toNewYDBArg(arg interface{}) (argVal interface{}, argType string, err error) {
	switch v := arg.(type) {
	case string:
		if strings.HasPrefix(v, "{") {
			argType = "Json"
			argVal = ydb.JSONValue(v)
		} else {
			argType = "Utf8"
			argVal = v
		}
	case []byte:
		argType = "Utf8"
		argVal = v
	case int, int64:
		argType = "Int64"
		argVal = v
	case uint64, uint:
		argType = "Uint64"
		argVal = v
	case bool:
		argType = "Bool"
		argVal = v
	case time.Time:
		argType = "Uint64"
		argVal = timeToUint64(v)
	case kikimrTime:
		argType = "Uint64"
		argVal = timeToUint64(*v.v)
	case sql.NamedArg:
		return toNewYDBArg(v.Value)
	default:
		return nil, "", fmt.Errorf("unsupported value %v found in statement", arg)
	}
	return
}

func (t kikimrTime) Scan(src interface{}) error {
	if src == nil {
		*t.v = time.Time{}
		return nil
	}

	switch src := src.(type) {
	case string:
		v, err := strconv.ParseInt(src, 10, 64)
		if err != nil {
			return fmt.Errorf("invalid time: %v", src)
		}
		*t.v = time.Unix(v/1000000, (v%1000000)*1000)
	case int64:
		*t.v = time.Unix(src/1000000, (src%1000000)*1000)
	case uint64:
		*t.v = time.Unix(int64(src/1000000), int64((src%1000000)*1000))
	default:
		return fmt.Errorf("time src is %T, not int64 or string", src)
	}

	return nil
}

func (t kikimrTime) Value() (driver.Value, error) {
	if t.v == nil {
		return nil, nil
	}
	return t.v.UnixNano() / 1000, nil
}

type kikimrChunkFormat struct {
	v *storage.ChunkFormat
}

func (t kikimrChunkFormat) Scan(src interface{}) error {
	var cf storage.ChunkFormat
	switch v := src.(type) {
	case string:
		cf = storage.ChunkFormat(v)
	case []byte:
		cf = storage.ChunkFormat(v)
	default:
		return fmt.Errorf("chunkFormat src is invalid: %T", src)
	}

	if !storage.IsValidFormat(cf) {
		return fmt.Errorf("chunkFormat value is invalid: %v", cf)
	}

	*t.v = cf
	return nil
}

type nullString struct {
	v *string
}

func (s nullString) Scan(src interface{}) error {
	if src == nil {
		*s.v = ""
		return nil
	}

	switch v := src.(type) {
	case string:
		*s.v = v
	case []byte:
		*s.v = string(v)
	default:
		return fmt.Errorf("nullString src is invalid: %T", src)
	}
	return nil
}

type queryBuilder struct {
	args    []interface{}
	clauses []string
}

func newQueryBuilder(startArgs ...interface{}) *queryBuilder {
	return &queryBuilder{
		args:    startArgs,
		clauses: make([]string, 0),
	}
}

func (qb *queryBuilder) AppendOne(value interface{}) {
	qb.args = append(qb.args, value)
	qb.clauses = append(qb.clauses, fmt.Sprintf("$%v", len(qb.args)))
}

func (qb *queryBuilder) Append(values ...interface{}) {
	for i := 0; i < len(values); i++ {
		qb.clauses = append(qb.clauses, fmt.Sprintf("$%v", len(qb.args)+i+1))
	}
	qb.args = append(qb.args, values...)
}

func (qb *queryBuilder) RenderArgs() (string, error) {
	newArgs := make([]string, len(qb.args))
	for i, e := range qb.args {
		prepared := fmt.Sprintf("%v", e)
		if strings.ContainsAny(prepared, `'"`) {
			return "", errors.New("can't prepare argument with quote")
		}
		newArgs[i] = fmt.Sprintf(`'%s'`, prepared)
	}

	return fmt.Sprintf("(%v)", strings.Join(newArgs, ",")), nil
}

func (qb *queryBuilder) AppendInBraces(values ...interface{}) {
	var s bytes.Buffer
	s.WriteString("(")
	for i := 0; i < len(values); i++ {
		if i > 0 {
			s.WriteString(", ")
		}
		s.WriteString("$")
		s.WriteString(strconv.Itoa(len(qb.args) + i + 1))
	}
	s.WriteString(")")
	qb.clauses = append(qb.clauses, s.String())
	qb.args = append(qb.args, values...)
}

func errorToMetric(err error) {
	if err == nil {
		return
	}
	if ydb.IsOpError(err, ydb.StatusAborted) {
		misc.KikimrTransactionErrors.Inc()
	} else {
		misc.KikimrQueryErrors.Inc()
	}
}
