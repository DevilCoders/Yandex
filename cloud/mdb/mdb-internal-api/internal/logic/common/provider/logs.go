package provider

import (
	"context"
	"strings"
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Logs struct {
	cfg      logic.Config
	sessions sessions.Sessions
	reader   clusters.Reader
	ldb      logsdb.Backend
	clock    clockwork.Clock
	backoff  *retry.BackOff
}

var _ common.Logs = &Logs{}

func NewLogs(cfg logic.Config, sessions sessions.Sessions, reader clusters.Reader, ldb logsdb.Backend, clock clockwork.Clock) *Logs {
	return &Logs{cfg: cfg, sessions: sessions, reader: reader, ldb: ldb, clock: clock, backoff: retry.New(cfg.Logs.Retry)}
}

// Logs implements generalized handling of logs retrieval procedure
func (ls *Logs) Logs(
	ctx context.Context,
	cid string,
	st logs.ServiceType,
	limit int64,
	opts common.LogsOptions,
) ([]logs.Message, bool, error) {
	ctx, _, err := ls.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBClustersGetLogs))
	if err != nil {
		return nil, false, err
	}
	defer ls.sessions.Rollback(ctx)

	if _, err = ls.reader.ClusterByClusterID(ctx, cid, st.ClusterType(), models.VisibilityVisible); err != nil {
		return nil, false, err
	}

	return ls.logs(ctx, cid, st, limit, opts)
}

const (
	defaultFromTSModifier = -time.Hour
)

func defaultFromTS(clock clockwork.Clock) time.Time {
	return clock.Now().Add(defaultFromTSModifier).UTC()
}

func defaultToTS(clock clockwork.Clock) time.Time {
	return clock.Now().UTC()
}

func (ls *Logs) Stream(
	ctx context.Context,
	cid string,
	st logs.ServiceType,
	opts common.LogsOptions,
) (<-chan common.LogsBatch, error) {
	ctx, _, err := ls.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBClustersGetLogs))
	if err != nil {
		return nil, err
	}
	// Session will likely be closed before we retrieve our first log.
	// It is correct.
	// We cannot hold session while streaming logs because it can take us a long while.
	defer ls.sessions.Rollback(ctx)

	if _, err = ls.reader.ClusterByClusterID(ctx, cid, st.ClusterType(), models.VisibilityVisible); err != nil {
		return nil, err
	}

	// Set default values for 'from' if needed
	// This value MUST be set here, otherwise it will be changed on every call to logsdb and pagination
	// code will fail
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(ls.clock))
	}

	ch := make(chan common.LogsBatch)
	go func() {
		defer close(ch)

		for {
			// We rely on logs retrieval code to handle context for us
			res, more, err := ls.logs(ctx, cid, st, ls.cfg.Logs.BatchSize, opts)
			if err != nil {
				sendLogsBatch(ctx, ch, common.LogsBatch{Err: err})
				return
			}

			if len(res) > 0 {
				sendLogsBatch(ctx, ch, common.LogsBatch{Logs: res})
				// Set offset equal to the last message 'next token'
				opts.Offset.Set(res[len(res)-1].NextMessageToken)
			}

			// Is there any more data?
			if !more {
				// ending timestamp was set, bail out
				if opts.ToTS.Valid {
					return
				}

				// We are in 'tail -f' mode, wait some time and repeat the request with new offset
				select {
				case <-ctx.Done():
					return
				case <-ls.clock.After(ls.cfg.Logs.TailWaitPeriod):
				}
			}
		}
	}()

	return ch, nil
}

func sendLogsBatch(ctx context.Context, ch chan<- common.LogsBatch, lb common.LogsBatch) {
	// Our reader might be 'dead' so either write to channel or wait for context
	select {
	case <-ctx.Done():
		return
	case ch <- lb:
	}
}

// Logs implements generalized handling of logs retrieval procedure
func (ls *Logs) logs(
	ctx context.Context,
	cid string,
	st logs.ServiceType,
	limit int64,
	opts common.LogsOptions,
) (logs []logs.Message, more bool, err error) {
	// Sanitize logs on exit
	defer func() {
		for i := range logs {
			msg := logs[i].Message
			for k, v := range msg {
				msg[k] = strings.ToValidUTF8(v, "?")
			}
		}
	}()

	// LogsDB might be missing
	if ls.ldb == nil {
		return nil, false, semerr.NotImplemented("not implemented")
	}

	// Set defaults
	// TODO: rework pagination into separate object/package
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(ls.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(ls.clock))
	}

	// This 'magic' is for postgresql's pooler only - service type mapper will return odyssey and bouncer (in that order)
	// Other service types map to corresponding logs service types one to one
	for _, lst := range logsdb.LogsServiceTypeToLogType(st) {
		err = ls.backoff.Retry(ctx, func() error {
			logs, more, err = ls.ldb.Logs(
				ctx,
				cid,
				lst,
				opts.ColumnFilter,
				opts.FromTS.Time,
				opts.ToTS.Time,
				limit,
				opts.Offset.Int64,
				opts.Filter,
			)
			if err != nil {
				// Unavailable is retryable, everything else is not
				if semerr.IsUnavailable(err) {
					return err
				}

				return retry.Permanent(err)
			}

			return nil
		})
		// Fail on any error, doesn't matter if we have any logs service types left or not
		if err != nil {
			return nil, false, xerrors.Errorf("logs for service type %q: %w", lst, err)
		}

		// We have the data - return it!
		if len(logs) != 0 {
			return
		}
	}

	// Return whatever the last iteration produced
	return
}
