package kikimr

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"

	"github.com/gofrs/uuid"
	otlog "github.com/opentracing/opentracing-go/log"
	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/serializedlock"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

const (
	lockActionTimeout = 30 * time.Second
)

func (st *kikimrstorage) getTableName(name string) string {
	return st.p.GetDatabase(defaultDatabase).GetTable(name).GetName()
}

func (st *kikimrstorage) replacer() kikimrReplacer {
	// new driver does not support PRAGMA (related to isolation) and COMMIT/ROLLBACK statements in query
	cutExpressions := regexp.MustCompile(`(PRAGMA.*?;)|(ROLLBACK;?)`)
	lastCharRegexp := regexp.MustCompile("[^a-zA-Z_]")
	digitArgumentRegexp := regexp.MustCompile(`\$\d+`)

	tableNameReplacer := func(query string) string {
		return queryRegexp.ReplaceAllStringFunc(query, func(match string) string {
			if match[0] != '$' {
				return " "
			}
			if lastCharRegexp.MatchString(match[len(match)-1:]) {
				// Strip '$' before and restore the random symbol after.
				return st.getTableName(match[1:len(match)-1]) + match[len(match)-1:]
			}
			// End-of-line
			return st.getTableName(match[1:])
		})
	}

	replacer := func(ctx context.Context, query string, args []interface{}) (string, []interface{}) {

		query = cutExpressions.ReplaceAllString(query, " ")
		newArgs := make([]interface{}, len(args))

		if strings.HasPrefix(query, "#PREPARE") {
			query = strings.TrimPrefix(query, "#PREPARE")
			for i, arg := range args {
				a, ok := arg.(sql.NamedArg)
				if !ok {
					log.G(ctx).Panic("Non named argument found in #PREPARE statement")
				}
				// ensure no unsupported type in arguments
				argVal, _, err := toNewYDBArg(a.Value)
				if err != nil {
					log.G(ctx).Panic("unknown type found in #PREPARE statement"+err.Error(),
						zap.String("type_path", reflect.TypeOf(arg).PkgPath()),
						zap.String("type_name", reflect.TypeOf(arg).Name()),
						zap.Any("type_value", arg), zap.Error(err))
				}
				newArgs[i] = sql.Named(a.Name, argVal)
			}
			// add real table names
			return tableNameReplacer(query), newArgs
		}
		declare := make([]string, len(args))

		// new driver does not support not named arguments
		for index, arg := range args {
			argName := "a" + strconv.Itoa(index+1)
			argVal, argType, err := toNewYDBArg(arg)
			if err != nil {
				log.G(ctx).Panic("unknown type found in statement",
					zap.String("type_path", reflect.TypeOf(arg).PkgPath()),
					zap.String("type_name", reflect.TypeOf(arg).Name()),
					zap.Any("type_value", arg), zap.Error(err))
			}
			newArgs[index] = sql.Named(argName, argVal)
			declare[index] = fmt.Sprintf(`DECLARE $%s AS %s;`, argName, argType)
		}

		// replace arguments from numeric to named
		query = digitArgumentRegexp.ReplaceAllStringFunc(query, func(s string) string {
			return "$a" + s[1:]
		})

		if len(declare) > 0 {
			query = strings.Join(declare, " ") + " " + query
		}

		// add real table names
		return tableNameReplacer(query), newArgs
	}

	return func(ctx context.Context, query string, args []interface{}) (string, []interface{}) {
		newQuery, newArgs := replacer(ctx, query, args)

		// force YQLv1 syntax
		newQuery = "--!syntax_v1\n" + strings.TrimSpace(newQuery)
		return newQuery, newArgs
	}
}

func (st *kikimrstorage) retryWithTxOpts(ctx context.Context, name string, f func(tx Querier) error, options *sql.TxOptions) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	span.LogFields(otlog.String("name", name))
	defer func() { tracing.Finish(span, resErr) }()

	return misc.Retry(ctx, name, func() error {
		ctx, cancel := context.WithTimeout(ctx, st.kikimrGlobalTimeout)
		span, ctx := tracing.StartSpanFromContext(ctx, "attempt")
		defer func() {
			tracing.Finish(span, resErr)
			cancel()
		}()

		tx, err := st.db.BeginTx(ctx, options)
		if err != nil {
			log.G(ctx).Error(name+": failed to begin transaction", zap.Error(err))
			return kikimrError(err)
		}

		if err = f(tx); err != nil {
			t := misc.KikimrRollbackQueryTimer.Start()
			defer t.ObserveDuration()
			switch err {
			case misc.ErrNoCommit:
				_ = tx.Rollback()
				return nil
			case context.Canceled, context.DeadlineExceeded:
				_ = tx.Rollback()
				return err
			}

			if err1 := tx.Rollback(); err1 != nil && !ydb.IsOpError(err1, ydb.StatusNotFound) {
				log.G(ctx).Error(name+": failed to rollback transaction", zap.Error(err1))
			}
			return convertRetryableError(err)
		}

		if err = tx.Commit(); err != nil {
			log.G(ctx).Error(name+": failed to commit transaction", zap.Error(err))
			return kikimrError(err)
		}

		return nil
	})
}

func (st *kikimrstorage) retryWithTx(ctx context.Context, name string, f func(tx Querier) error) error {
	return st.retryWithTxOpts(ctx, name, f, &sql.TxOptions{})
}

func (st *kikimrstorage) createTransactionPerQueryTx(txOpts *sql.TxOptions) Querier {
	return sqlTxPerQuery{
		db:     st.db,
		txOpts: txOpts,
	}
}

func (st *kikimrstorage) selectPartial(
	ctx context.Context, tx Querier,
	query string, args []interface{}, n int64,
	cb func(rows SQLRows) error,
	nextIterCb func() (query string, args []interface{}),
) error {
	var total int
	maxSelectRows := st.p.GetDatabase(defaultDatabase).GetConfig().MaxSelectRows
	for cnt := maxSelectRows; cnt == maxSelectRows || nextIterCb != nil; total += cnt {
		cnt = 0
		var q string
		var a []interface{}
		if nextIterCb == nil {
			q = query + " LIMIT " + strconv.Itoa(maxSelectRows) + " OFFSET " + strconv.Itoa(total)
			a = args
		} else {
			q, a = nextIterCb()
			if q == "" {
				break
			}
			q += " LIMIT " + strconv.Itoa(maxSelectRows)
		}
		rows, err := tx.QueryContext(ctx, q, a...)
		if err != nil {
			log.G(ctx).Error("selectPartial: unknown error", zap.Error(err), zap.String("query", q),
				zap.Reflect("args", a))
			return kikimrError(err)
		}

		for ; rows.Next(); cnt++ {
			if n > 0 && int64(cnt)+int64(total) >= n {
				_ = rows.Close()
				return nil
			}
			if err = cb(rows); err != nil {
				_ = rows.Close()
				return err
			}
		}

		if err = rows.Err(); err != nil {
			log.G(ctx).Error("selectPartial: rows.Err()", zap.Error(err))
			return kikimrError(err)
		}
		_ = rows.Close()
	}
	return nil
}

type LockHolder struct {
	// closed when critical section is done and lock has to be unlocked
	done chan struct{}
	// closed when lock actually unlocked (we need to wait to lock to unlock before exiting critical section, else lock can remain in invalid state if program exits)
	unlocked chan struct{}
	// marks holder as truly closed, to allow multiple Close calls
	closed atomic.Value
}

func (l *LockHolder) Close(ctx context.Context) {
	if l.closed.Load() != nil {
		// already closed
		return
	}
	span, _ := tracing.StartSpanFromContextFunc(ctx)
	defer span.Finish()

	t := misc.UnlockTimer.Start()
	defer t.ObserveDuration()

	// mark LockHolder as closed so no double close occure
	l.closed.Store(struct{}{})

	// start unlocking
	close(l.done)

	// blocks until actually unlocked
	<-l.unlocked
}

func (st *kikimrstorage) pollLock(ctx context.Context, snapshotID string, lockID string, done chan struct{}, cancelOP context.CancelFunc) {
	cfg, _ := config.GetConfig()
	tracingLimiter := tracing.SpanRateLimiter{
		InitialInterval: cfg.Tracing.LoopRateLimiterInitialInterval.Duration,
		MaxInterval:     cfg.Tracing.LoopRateLimiterMaxInterval.Duration,
	}

	// always unlock in the end
	defer func() {
		log.G(ctx).Debug("Canceling lockedCTX due to end of lockPolling in lock", zap.String("snapshot-id", snapshotID))
		cancelOP()
		unlockCTX := misc.WithNoCancelIn(ctx, lockActionTimeout)
		if err := st.retryWithTx(unlockCTX, fmt.Sprintf("unlockPoller-%s-%s", snapshotID, lockID), func(tx Querier) error {
			now := time.Now()
			lock, err := lockLoad(unlockCTX, tx, snapshotID, now)
			if err != nil {
				log.G(unlockCTX).Error("pollLock: failed to load lock", zap.String("snapshot-id", snapshotID), zap.Error(err))
				return err
			}
			if err := lock.Unlock(serializedlock.LockIDType(lockID), now); err != nil {
				log.G(unlockCTX).Error("pollLock: failed to update lock", zap.String("snapshot-id", snapshotID), zap.String("lock-id", lockID), zap.Error(err))
				return err
			}
			return lockStore(unlockCTX, tx, snapshotID, now, lock)
		}); err == nil {
			log.G(unlockCTX).Info("pollLock: snapshot unlocked", zap.String("lock-id", lockID), zap.String("snapshot-id", snapshotID))
		} else {
			log.G(unlockCTX).Error("pollLock: failed to unlock snapshot", zap.Error(err), zap.String("snapshot-id", snapshotID), zap.String("lock-id", lockID))
		}
	}()
	log.G(ctx).Info("lockpoll started", zap.String("lock-id", lockID))

	for {
		span, loopCTX := tracingLimiter.StartSpanFromContext(ctx, "pollLockCycle")
		t := misc.PollUpdateLockTimer.Start()
		select {
		case <-done:
			log.G(ctx).Info("pollLock: lock closed from lockHolder", zap.String("lock-id", lockID))
			return
		case <-ctx.Done():
			log.G(ctx).Info("pollLock: lock closed due to ctx.Done()", zap.String("lock-id", lockID))
			return
		case <-time.After(st.lockPollInterval):
		}

		err := st.updateLock(loopCTX, snapshotID, lockID)
		t.ObserveDuration()
		tracing.Finish(span, err)
		if err != nil {
			return
		}
	}
}

func (st *kikimrstorage) updateLock(ctx context.Context, snapshotID, lockID string) error {
	now := time.Now()
	until := now.Add(st.lockDuration)

	updateCTX, cancel := context.WithTimeout(ctx, lockActionTimeout)
	defer func() {
		log.G(ctx).Debug("Canceling updateCTX due to end of kikimrstorage.updateLock")
		cancel()
	}()
	if err := st.retryWithTx(updateCTX, fmt.Sprintf("lockUpdatePoller-%s-%s", snapshotID, lockID), func(tx Querier) error {
		lock, err := lockLoad(updateCTX, tx, snapshotID, now)
		if err != nil {
			log.G(updateCTX).Error("pollLock: failed to load lock", zap.String("snapshot-id", snapshotID), zap.Error(err))
			return err
		}
		if err := lock.Update(serializedlock.LockIDType(lockID), now, until); err != nil {
			log.G(updateCTX).Error("pollLock: failed to update lock", zap.String("snapshot-id", snapshotID), zap.String("lock-id", lockID), zap.Error(err))
			return err
		}

		return lockStore(updateCTX, tx, snapshotID, now, lock)
	}); err != nil {
		return err
	}
	log.G(ctx).Info("pollLock: lock update success", zap.Time("now", now), zap.Time("until", until))
	return nil
}

func (st *kikimrstorage) lockSnapshotGuard(ctx context.Context, snapshotID string, lockType serializedlock.LockType, reason string, maxActiveLocks int) (context.Context, *LockHolder, error) {
	lockID, err := st.lockSnapshot(ctx, snapshotID, lockType, reason, maxActiveLocks)
	if err != nil {
		log.G(ctx).Error("failed to lock snapshot", zap.String("snapshot-id", snapshotID), zap.Error(err))
		return ctx, nil, err
	}
	log.G(ctx).Info("snapshot is locked now", zap.String("lock-id", lockID), zap.String("snapshot-id", snapshotID))
	c, holder := st.startPollLock(ctx, snapshotID, lockID, reason)
	return c, holder, nil
}

func (st *kikimrstorage) startPollLock(ctx context.Context, snapshotID string, lockID string, reason string) (context.Context, *LockHolder) {
	c, cancel := context.WithCancel(ctx)
	ctx = log.WithLogger(ctx, log.G(ctx).Named(fmt.Sprintf("pollLock-%s", reason)))
	done := make(chan struct{})
	unlocked := make(chan struct{})
	go func() {
		/*
			1. Lock must be locked when startPollLock is called
			2. Lock is updated until LockHolder.Close() or any error during lock update
			3. Lock is released when either lock update fails or LockHolder.Close() called
			4. When unlocking, we ignore that context can be already canceled to guarantee unlock
			5. LockHolder.Close() returns only when lock unlocked
		*/
		defer close(unlocked)
		st.pollLock(ctx, snapshotID, lockID, done, cancel)
	}()
	return c, &LockHolder{done: done, unlocked: unlocked}
}

// LockSnapshot take exclusive lock
func (st *kikimrstorage) LockSnapshot(ctx context.Context, id string, reason string) (lockedCTX context.Context, holder storage.LockHolder, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	return st.lockSnapshotGuard(ctx, id, serializedlock.Exclusive, reason, -1)
}

func (st *kikimrstorage) LockSnapshotShared(ctx context.Context, lockID string, reason string) (c context.Context, holder storage.LockHolder, resErr error) {
	return st.LockSnapshotSharedMax(ctx, lockID, reason, -1)
}

// Take shared lock to snapshot.
// It can have many shared lock or one exclusive lock
// If number of active locks more then maxActiveLocks - return ErrMaxShareLockExceeded
// maxActiveLocks = -1 mean unlimited shared locks
func (st *kikimrstorage) LockSnapshotSharedMax(ctx context.Context, snapshotID string, reason string, maxActiveLocks int) (c context.Context, holder storage.LockHolder, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	return st.lockSnapshotGuard(ctx, snapshotID, serializedlock.Shared, reason, maxActiveLocks)
}

// Try to lock snapshot
// Drops all locks and locks snapshot for itself if snapshot locked by someone
// wait until all other users see new lock condition
func (st *kikimrstorage) LockSnapshotForce(ctx context.Context, snapshotID string, reason string) (lockedCTX context.Context, holder storage.LockHolder, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	c, holder, err := st.lockSnapshotGuard(ctx, snapshotID, serializedlock.Exclusive, reason, -1)
	switch err {
	case nil:
		return c, holder, nil
	case misc.ErrAlreadyLocked:
		var id serializedlock.LockIDType
		if e := st.retryWithTx(ctx, "lockSnapshotForceUpdate", func(tx Querier) error {
			var lock serializedlock.Lock
			now := time.Now()
			if id, err = lock.Lock(serializedlock.Exclusive, now, now.Add(st.lockDuration)); err == nil {
				return lockStore(ctx, tx, snapshotID, now, lock)
			} else {
				return err
			}
		}); e == nil {
			c, holder := st.startPollLock(ctx, snapshotID, string(id), reason)
			// wait until everyone saw lock is locked by us now
			time.Sleep(st.lockPollInterval * 3)
			return c, holder, nil
		} else {
			return ctx, nil, e
		}
	default:
		return ctx, nil, err
	}
}

func (st *kikimrstorage) lockSnapshot(ctx context.Context, snapshotID string, lockType serializedlock.LockType, reason string, maxActiveLocks int) (string, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).Named(fmt.Sprintf("lockSnapshot-%s", reason)))
	var lockIDString string
	err := st.retryWithTx(ctx, "lockSnapshot", func(tx Querier) error {
		now := time.Now()
		lock, err := lockLoad(ctx, tx, snapshotID, now)
		if err != nil {
			return err
		}
		lock.SetSkew(st.timeSkewInterval)
		lockID, err := lock.Lock(lockType, now, now.Add(st.lockDuration))
		if err != nil {
			if err == serializedlock.ErrAlreadyLocked {
				err = misc.ErrAlreadyLocked
			}
			log.G(ctx).Info("Can't lock snapshot", zap.Error(err))
			return err
		}
		if maxActiveLocks != -1 && len(lock.Locks) > maxActiveLocks {
			return misc.ErrMaxShareLockExceeded
		}
		lockIDString = string(lockID)
		return lockStore(ctx, tx, snapshotID, now, lock)
	})
	if err != nil {
		lockIDString = ""
	}
	return lockIDString, err
}

func lockStore(ctx context.Context, tx Querier, snapshotID string, now time.Time, lock serializedlock.Lock) error {
	if lock.IsLocked(now) {
		log.G(ctx).Debug("Save lock state")
		jsonBytes, err := json.Marshal(lock)
		if err != nil {
			log.G(ctx).Error("Can't marshal lock object", zap.Error(err))
			return err
		}

		_, err = tx.ExecContext(ctx,
			"UPSERT INTO $snapshots_locks (id, `timestamp`, lock_state) "+
				"VALUES ($1, $2, $3)", snapshotID, kikimrTime{&now}, string(jsonBytes))
		if err != nil {
			log.G(ctx).Error("lockStore: insert failed", zap.Error(err))
			return kikimrError(err)
		}
	} else {
		log.G(ctx).Debug("Remove lock state")
		_, err := tx.ExecContext(ctx,
			"#PREPARE "+
				"DECLARE $id AS Utf8; "+
				"DELETE FROM $snapshots_locks WHERE id = $id", sql.Named("id", snapshotID))
		if err != nil {
			log.G(ctx).Error("lockStore: delete failed", zap.Error(err))
			return kikimrError(err)
		}
	}
	return nil
}

func lockLoad(ctx context.Context, tx Querier, snapshotID string, now time.Time) (serializedlock.Lock, error) {
	var lock serializedlock.Lock
	var timestamp time.Time
	var lockJSON *string
	err := tx.QueryRowContext(ctx,
		"SELECT `timestamp`, lock_state FROM $snapshots_locks WHERE id = $1",
		snapshotID).Scan(kikimrTime{&timestamp}, &lockJSON)
	switch err {
	case nil:
		// old lock
		if lockJSON == nil {
			return lock, err
		} else {
			// new lock state
			err := json.Unmarshal([]byte(*lockJSON), &lock)
			return lock, err
		}
	case sql.ErrNoRows:
		return lock, nil
	default:
		log.G(ctx).Error("lockLoad: select failed", zap.Error(err))
		return lock, kikimrError(err)
	}
}

func stringSliceContains(slice []string, value string) bool {
	for _, item := range slice {
		if item == value {
			return true
		}
	}
	return false
}

func newChunkID(snapshotID string, offset int64) string {
	return uuid.NewV5(uuid.Nil, fmt.Sprintf("%s-%d", snapshotID, offset)).String()
}
