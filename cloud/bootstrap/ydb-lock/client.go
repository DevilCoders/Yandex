package ydblock

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"text/template"
	"time"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
	spanlog "github.com/opentracing/opentracing-go/log"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-sdk/v3/table"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydbtypes "github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	"go.uber.org/zap"
)

type getLockedQueryResult struct {
	count       uint64
	description string
}

type templateConfig struct {
	TablePathPrefix string
	TableName       string
}

// Client Bootstrap locks implementation using ydb
//
// Based on https://bb.yandex-team.ru/projects/cloud/repos/ci/browse/ci/yc_ci/common/clients/kikimr/__init__.py?at=451fdf7b3c19d31bf3df373c6046fba587a9a5c1
type Client struct {
	DB              ydb.Connection
	Root            string
	Logger          *zap.Logger
	MetricsRegistry prometheus.Registerer
}

// NewClient Convenience function for instantiating new client
func NewClient(DB ydb.Connection, root string) *Client {
	return &Client{DB: DB, Root: root}
}

var checkRoot = registerQuery(`
	SELECT 1 FROM {{ .TableName }}
	`)

func (c *Client) Init(ctx context.Context) error {
	if c.Logger == nil {
		c.Logger = zap.NewNop()
	}
	if c.MetricsRegistry == nil {
		c.MetricsRegistry = prometheus.NewRegistry()
	}
	registerMetrics(c.MetricsRegistry)

	// Render queries.
	queries := defaultQueryRegistry.getQueries()
	for _, query := range queries {
		q, err := c.renderQuery(query.rawQuery)
		if err != nil {
			return fmt.Errorf("cannot render query '%s'", query.rawQuery)
		}
		query.renderedQuery = q
	}

	err := c.DB.Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) error {
			_, _, err := s.Execute(ctx, table.DefaultTxControl(), query(checkRoot), table.NewQueryParameters())
			return err
		},
	)
	if err != nil {
		return fmt.Errorf("failed to validate root: %w", err)
	}

	return nil
}

// CreateLock Tries to create new lock with specified params and returns error on failure or if specified hosts are already locked.
// Returns Lock instance on success
func (c *Client) CreateLock(
	ctx context.Context,
	description string,
	hosts []string,
	hbTimeout uint64,
	lockType HostLockType,
) (*Lock, error) {
	timer := prometheus.NewTimer(createLockTimings)
	defer timer.ObserveDuration()
	span, ctx := opentracing.StartSpanFromContext(ctx, "ydblock.create_lock")
	defer span.Finish()
	span.LogFields(spanlog.Int("hosts_len", len(hosts)))
	span.LogFields(spanlog.String("type", lockType.String()))
	span.LogFields(spanlog.String("description", description))
	now := time.Now().Unix()
	deadline := uint64(now) + hbTimeout
	span.LogFields(spanlog.Uint64("deadline", deadline))
	lockTypes := lockTypesFor(lockType)
	locked := &getLockedQueryResult{}
	err := c.DB.Table().DoTx(
		ctx,
		func(ctx context.Context, tx table.TransactionActor) (err error) {
			locked, err = c.getLocked(ctx, tx, now, hosts, lockTypes)
			if err != nil {
				return fmt.Errorf("getLocked failed: %w", err)
			}
			if locked.count > 0 {
				return fmt.Errorf("some hosts (%d) already locked. Description: \n%s",
					locked.count, locked.description)
			}
			return c.createLock(ctx, tx, deadline, hosts, description, lockType)
		})
	if err != nil {
		return nil, spanReportError(span, err)
	}
	return &Lock{Hosts: hosts, Deadline: deadline, Description: description, Timeout: hbTimeout, Type: lockType}, nil
}

func lockTypesFor(lockType HostLockType) []HostLockType {
	if lockType == HostLockTypeOther {
		return []HostLockType{HostLockTypeOther}
	}
	return HostLockTypeAll
}

// CheckHostLock checks if specified host is locked with specified lock and return error if lock is not found or invalidated
// by add-hosts workflow for example
func (c *Client) CheckHostLock(ctx context.Context, hostname string, lock *Lock) error {
	timer := prometheus.NewTimer(checkHostLockTimings)
	defer timer.ObserveDuration()
	span, ctx := opentracing.StartSpanFromContext(ctx, "ydblock.check_host_lock")
	defer span.Finish()
	span.SetTag("hostname", hostname)
	span.LogFields(spanlog.Int("hosts_len", len(lock.Hosts)))
	span.LogFields(spanlog.String("type", lock.Type.String()))
	span.LogFields(spanlog.Uint64("deadline", lock.Deadline))
	span.LogFields(spanlog.String("description", lock.Description))
	now := time.Now().Unix()
	err := c.DB.Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) error {
			description, err := c.readHostLock(ctx, s, now, hostname)
			if err != nil {
				return fmt.Errorf("readHostLock failed: %w", err)
			}
			if description != lock.Description {
				return fmt.Errorf("lock for hostname %s is invalidated by lock with description: %s", hostname, description)
			}
			return nil
		})
	return spanReportError(span, err)
}

// ExtendLock Validates and extends specified lock and returns updated lock. Will fail if locks are expired or invalidated
// by add-hosts workflow for example
func (c *Client) ExtendLock(ctx context.Context, lock *Lock) (*Lock, error) {
	timer := prometheus.NewTimer(extendLockTimings)
	defer timer.ObserveDuration()
	span, ctx := opentracing.StartSpanFromContext(ctx, "ydblock.extend_lock")
	defer span.Finish()
	span.LogFields(spanlog.Int("hosts_len", len(lock.Hosts)))
	span.LogFields(spanlog.String("type", lock.Type.String()))
	span.LogFields(spanlog.Uint64("deadline", lock.Deadline))
	span.LogFields(spanlog.String("description", lock.Description))
	deadline := uint64(time.Now().Unix()) + lock.Timeout
	err := c.DB.Table().DoTx(ctx,
		func(ctx context.Context, tx table.TransactionActor) error {
			if err := c.checkLocksAreValid(ctx, tx, lock); err != nil {
				return fmt.Errorf("checkLocksAreValid failed: %w", err)
			}
			return c.extendLock(ctx, tx, deadline, lock.Hosts, lock.Type)
		})
	if err != nil {
		return nil, spanReportError(span, err)
	}
	lock.Deadline = deadline
	span.LogFields(spanlog.Uint64("new_deadline", lock.Deadline))
	return lock, nil
}

// ReleaseLock Validates and releases specified lock. Will fail if locks are expired or overtaken
// by add-hosts workflow for example
func (c *Client) ReleaseLock(ctx context.Context, lock *Lock) error {
	timer := prometheus.NewTimer(releaseLockTimings)
	defer timer.ObserveDuration()
	span, ctx := opentracing.StartSpanFromContext(ctx, "ydblock.release_lock")
	defer span.Finish()
	span.LogFields(spanlog.Int("hosts_len", len(lock.Hosts)))
	span.LogFields(spanlog.String("type", lock.Type.String()))
	span.LogFields(spanlog.Uint64("deadline", lock.Deadline))
	span.LogFields(spanlog.String("description", lock.Description))
	err := c.DB.Table().DoTx(
		ctx,
		func(ctx context.Context, tx table.TransactionActor) (err error) {
			if err = c.checkLocksAreValid(ctx, tx, lock); err != nil {
				return fmt.Errorf("checkLocksAreValid failed: %w", err)
			}
			if err = c.releaseLock(ctx, tx, lock.Hosts, lock.Type); err != nil {
				return fmt.Errorf("releaseLock failed: %w", err)
			}
			return nil
		})
	return spanReportError(span, err)
}

var getLockedQuery = registerQuery(`
	DECLARE $now AS Int64;
	DECLARE $hosts AS List<Utf8>;
	DECLARE $types AS List<Utf8>;
	SELECT COUNT(*) AS count,
		Unicode::JoinFromList(CAST(AGGREGATE_LIST_DISTINCT(description, 10) AS List<Utf8>), "\n") AS description
	FROM {{ .TableName }}
	WHERE hostname IN $hosts AND deadline > $now AND type IN $types;
	`)

func (c *Client) getLocked(ctx context.Context, tx table.TransactionActor, now int64, hosts []string, types []HostLockType) (res *getLockedQueryResult, err error) {
	timer := prometheus.NewTimer(internalGetLockedTimings)
	defer timer.ObserveDuration()
	span, ctx := opentracing.StartSpanFromContext(ctx, "ydblock.internal_get_locked_query")
	defer span.Finish()
	span.LogFields(spanlog.Int("hosts_len", len(hosts)))
	var typesList []ydbtypes.Value
	for _, t := range types {
		typesList = append(typesList, ydbtypes.UTF8Value(t.String()))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$now", ydbtypes.Int64Value(now)),
		table.ValueParam("$hosts", ydbtypes.ListValue(stringsToYDBList(hosts)...)),
		table.ValueParam("$types", ydbtypes.ListValue(typesList...)),
	)
	queryRes, err := tx.Execute(ctx, query(getLockedQuery), params)
	if err != nil {
		return nil, spanReportError(span, err)
	}
	defer func() {
		err = spanReportError(span, queryRes.Close())
		if err != nil {
			c.Logger.Sugar().Errorf("error closing getLocked() query: %v", err)
		}
	}()
	if err = queryRes.NextResultSetErr(ctx); err != nil {
		return nil, spanReportError(span, err)
	}
	if queryRes.NextRow() {
		res = &getLockedQueryResult{}
		err = queryRes.ScanNamed(named.Required("count", &res.count), named.Required("description", &res.description))
		if err != nil {
			return nil, spanReportError(span, err)
		}
		if queryRes.Err() != nil {
			return nil, spanReportError(span, queryRes.Err())
		}
		return res, nil
	}
	return nil, spanReportError(span, errors.New("got empty result while getting locked hosts"))
}

var createLockQuery = registerQuery(`
	DECLARE $values AS List<Struct<hostname: Utf8, deadline: Uint64, description: Utf8, type: Utf8>>;
	REPLACE INTO {{ .TableName }} (hostname, deadline, description, type)
	SELECT hostname, deadline, description, type from AS_TABLE($values);
	`)

func (c *Client) createLock(ctx context.Context, tx table.TransactionActor, deadline uint64, hosts []string, description string, lockType HostLockType) error {
	timer := prometheus.NewTimer(internalCreateLockTimings)
	defer timer.ObserveDuration()
	span, ctx := opentracing.StartSpanFromContext(ctx, "ydblock.internal_create_lock_query")
	defer span.Finish()
	span.LogFields(spanlog.Int("hosts_len", len(hosts)))
	span.LogFields(spanlog.String("type", lockType.String()))
	span.LogFields(spanlog.Uint64("deadline", deadline))
	span.LogFields(spanlog.String("description", description))
	var values []ydbtypes.Value
	for _, host := range hosts {
		values = append(values, ydbtypes.StructValue(
			ydbtypes.StructFieldValue("hostname", ydbtypes.UTF8Value(host)),
			ydbtypes.StructFieldValue("deadline", ydbtypes.Uint64Value(deadline)),
			ydbtypes.StructFieldValue("description", ydbtypes.UTF8Value(description)),
			ydbtypes.StructFieldValue("type", ydbtypes.UTF8Value(lockType.String())),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydbtypes.ListValue(values...)),
	)
	queryRes, err := tx.Execute(ctx, query(createLockQuery), params)
	if err != nil {
		return spanReportError(span, err)
	}
	return spanReportError(span, queryRes.Close())
}

var readHostLockQuery = registerQuery(`
	DECLARE $hostname AS Utf8;
	DECLARE $now AS Uint64;
	SELECT description
	FROM {{ .TableName }}
	WHERE hostname = $hostname AND deadline > $now;
	`)

func (c *Client) readHostLock(ctx context.Context, tx table.Session, now int64, hostname string) (description string, err error) {
	readTx := table.TxControl(
		table.BeginTx(
			table.WithOnlineReadOnly(),
		),
		table.CommitTx(),
	)
	params := table.NewQueryParameters(
		table.ValueParam("$hostname", ydbtypes.UTF8Value(hostname)),
		table.ValueParam("$now", ydbtypes.Uint64Value(uint64(now))),
	)
	_, queryRes, err := tx.Execute(ctx, readTx, query(readHostLockQuery), params)
	if err != nil {
		return "", err
	}
	defer func() {
		err = queryRes.Close()
		if err != nil {
			c.Logger.Sugar().Errorf("error closing readHostLock() query: %v", err)
		}
	}()
	if err = queryRes.NextResultSetErr(ctx); err != nil {
		return "", err
	}
	if queryRes.NextRow() {
		if err = queryRes.ScanNamed(named.OptionalWithDefault("description", &description)); err != nil {
			return "", err
		}
		return description, queryRes.Err()
	}
	return "", fmt.Errorf("unexpected problem: cannot find lock for hostname %s", hostname)
}

func (c *Client) renderQuery(tpl string) (string, error) {
	var buf bytes.Buffer
	prefix := `--!syntax_v1
	PRAGMA TablePathPrefix("{{ .TablePathPrefix }}");
	`
	t := template.Must(template.New("").Parse(prefix + tpl))
	err := t.Execute(&buf, templateConfig{
		TablePathPrefix: c.Root,
		TableName:       lockTable,
	})
	if err != nil {
		return "", err
	}
	return buf.String(), nil
}

func (c *Client) checkLocksAreValid(ctx context.Context, tx table.TransactionActor, lock *Lock) error {
	checkCount, err := c.getPreviouslySetLocksCount(ctx, tx, lock.Hosts, lock.Deadline, lock.Description)
	if err != nil {
		return err
	}
	if checkCount != uint64(len(lock.Hosts)) {
		return fmt.Errorf("unexpected problem with locks: we have set locks on %d hosts, but now there are %d in correct state", len(lock.Hosts), checkCount)
	}
	return nil
}

var getPreviouslySetLocksCountQuery = registerQuery(`
	DECLARE $deadline AS Uint64;
    DECLARE $hosts AS List<Utf8>;
	DECLARE $description AS Utf8;
	DECLARE $add_hosts_type AS Utf8;
	SELECT COUNT(*) AS count
	FROM {{ .TableName }}
	WHERE hostname IN $hosts AND (deadline = $deadline AND description = $description OR
	type = $add_hosts_type);
	`)

func (c *Client) getPreviouslySetLocksCount(ctx context.Context, tx table.TransactionActor, hosts []string, deadline uint64, description string) (count uint64, err error) {
	timer := prometheus.NewTimer(internalGetPreviouslySetLocksCountTimings)
	defer timer.ObserveDuration()
	span, ctx := opentracing.StartSpanFromContext(ctx, "ydblock.internal_get_previous_set_locks_count_query")
	defer span.Finish()
	span.LogFields(spanlog.Int("hosts_len", len(hosts)))
	span.LogFields(spanlog.Uint64("deadline", deadline))
	span.LogFields(spanlog.String("description", description))
	params := table.NewQueryParameters(
		table.ValueParam("$hosts", ydbtypes.ListValue(stringsToYDBList(hosts)...)),
		table.ValueParam("$deadline", ydbtypes.Uint64Value(deadline)),
		table.ValueParam("$description", ydbtypes.UTF8Value(description)),
		table.ValueParam("$add_hosts_type", ydbtypes.UTF8Value(HostLockTypeAddHosts.String())),
	)
	queryRes, err := tx.Execute(ctx, query(getPreviouslySetLocksCountQuery), params)
	if err != nil {
		return 0, spanReportError(span, err)
	}
	defer func() {
		err = spanReportError(span, queryRes.Close())
		if err != nil {
			c.Logger.Sugar().Errorf("error closing readHostLock() query: %v", err)
		}
	}()
	if err = queryRes.NextResultSetErr(ctx); err != nil {
		return 0, spanReportError(span, err)
	}
	if queryRes.NextRow() {
		if err = queryRes.ScanNamed(named.Required("count", &count)); err != nil {
			return 0, spanReportError(span, err)
		}
		return count, spanReportError(span, queryRes.Err())
	}
	return 0, spanReportError(span, errors.New("got empty result while getting previously set locks count"))
}

var extendLockQuery = registerQuery(`
	DECLARE $deadline AS Uint64;
	DECLARE $hosts AS List<Utf8>;
	DECLARE $type AS Utf8;
	UPDATE {{ .TableName }} SET deadline = $deadline
	WHERE hostname IN $hosts AND type = $type;
	`)

func (c *Client) extendLock(ctx context.Context, tx table.TransactionActor, deadline uint64, hosts []string, lockType HostLockType) error {
	params := table.NewQueryParameters(
		table.ValueParam("$deadline", ydbtypes.Uint64Value(deadline)),
		table.ValueParam("$hosts", ydbtypes.ListValue(stringsToYDBList(hosts)...)),
		table.ValueParam("$type", ydbtypes.UTF8Value(lockType.String())),
	)
	queryRes, err := tx.Execute(ctx, query(extendLockQuery), params)
	if err != nil {
		return err
	}
	return queryRes.Close()
}

var releaseLockQuery = registerQuery(`
	DECLARE $hosts AS List<Utf8>;
	DECLARE $type AS Utf8;
	UPDATE {{ .TableName }} SET deadline = 0, description = ""
	WHERE hostname IN $hosts AND type = $type;
	`)

func (c *Client) releaseLock(ctx context.Context, tx table.TransactionActor, hosts []string, lockType HostLockType) error {
	params := table.NewQueryParameters(
		table.ValueParam("$hosts", ydbtypes.ListValue(stringsToYDBList(hosts)...)),
		table.ValueParam("$type", ydbtypes.UTF8Value(lockType.String())),
	)
	queryRes, err := tx.Execute(ctx, query(releaseLockQuery), params)
	if err != nil {
		return err
	}
	return queryRes.Close()
}

func stringsToYDBList(list []string) []ydbtypes.Value {
	var ydbList []ydbtypes.Value
	for _, host := range list {
		ydbList = append(ydbList, ydbtypes.UTF8Value(host))
	}
	return ydbList
}

// spanReportError is a helper func to register errors in trace span.
// It is safe to use it with nil error.
func spanReportError(span opentracing.Span, err error) error {
	if err == nil || span == nil {
		return err
	}
	ext.Error.Set(span, true)
	span.LogFields(spanlog.String("event", "error"), spanlog.String("message", err.Error()))
	return err
}
