package logic

import (
	"context"
	"strings"
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	TailWaitPeriod time.Duration `json:"tail_wait_period" yaml:"tail_wait_period"`
	Retry          retry.Config  `json:"retry" yaml:"retry"`
	BatchSize      int64         `json:"batch_size" yaml:"batch_size"`
}

func DefaultConfig() Config {
	return Config{
		TailWaitPeriod: time.Millisecond * 100,
		Retry:          retry.DefaultConfig(),
		BatchSize:      250,
	}
}

type Logs struct {
	config  Config
	auth    auth.Authenticator
	logsdb  logsdb.Backend
	clock   clockwork.Clock
	backoff *retry.BackOff
}

func NewLogs(config Config, auth auth.Authenticator, logsdb logsdb.Backend, clock clockwork.Clock) Logs {
	return Logs{
		config:  config,
		auth:    auth,
		logsdb:  logsdb,
		clock:   clock,
		backoff: retry.New(config.Retry),
	}
}

type LogsBatch struct {
	Logs []models.Log
	Err  error
}

func (l *Logs) ListLogs(ctx context.Context, criteria models.Criteria) ([]models.Log, bool, int64, error) {
	if err := l.auth.Authorize(ctx, criteria.Sources); err != nil {
		return nil, false, 0, err
	}

	logs, more, err := l.logs(ctx, criteria)

	var offset = criteria.Offset.Int64
	if len(logs) > 0 {
		offset = logs[len(logs)-1].Offset
	}

	return logs, more, offset, err
}

func (l *Logs) StreamLogs(ctx context.Context, criteria models.Criteria) (chan LogsBatch, error) {
	if err := l.auth.Authorize(ctx, criteria.Sources); err != nil {
		return nil, err
	}

	if !criteria.Limit.Valid {
		criteria.Limit = optional.NewInt64(l.config.BatchSize)
	}

	logChannel := make(chan LogsBatch)

	go func() {
		defer close(logChannel)

		for {
			// We rely on logs retrieval code to handle context for us
			res, more, err := l.logs(ctx, criteria)
			if err != nil {
				sendLogsBatch(ctx, logChannel, LogsBatch{Err: err})
				return
			}

			if len(res) > 0 {
				sendLogsBatch(ctx, logChannel, LogsBatch{Logs: res})
				// Set offset equal to the last message 'next token'
				criteria.Offset.Set(res[len(res)-1].Offset)
			}

			// Is there any more data?
			if !more {
				// ending timestamp was set, bail out
				if criteria.To.Valid {
					return
				}

				// We are in 'tail -f' mode, wait some time and repeat the request with new offset
				select {
				case <-ctx.Done():
					return
				case <-l.clock.After(l.config.TailWaitPeriod):
				}
			}
		}
	}()

	return logChannel, nil
}

func sendLogsBatch(ctx context.Context, ch chan<- LogsBatch, lb LogsBatch) {
	// Our reader might be 'dead' so either write to channel or wait for context
	select {
	case <-ctx.Done():
		return
	case ch <- lb:
	}
}

func (l *Logs) logs(ctx context.Context, criteria models.Criteria) (logs []models.Log, more bool, err error) {
	// Sanitize logs on exit
	defer func() {
		for i, m := range logs {
			logs[i].Message = strings.ToValidUTF8(m.Message, "?")
		}
	}()

	// LogsDB might be missing
	if l.logsdb == nil {
		return nil, false, semerr.NotImplemented("not implemented")
	}

	dbCriteria := l.criteriaToDB(criteria)

	err = l.backoff.Retry(ctx, func() error {
		logs, more, err = l.logsdb.Logs(ctx, dbCriteria)
		if err != nil {
			// Unavailable is retryable, everything else is not
			if semerr.IsUnavailable(err) {
				return err
			}

			return retry.Permanent(err)
		}

		return nil
	})
	if err != nil {
		return nil, false, xerrors.Errorf("logs for sources %v: %w", criteria.Sources, err)
	}

	// We have the data - return it!
	if len(logs) != 0 {
		return
	}

	// Return whatever the last iteration produced
	return
}

func (l *Logs) criteriaToDB(criteria models.Criteria) logsdb.Criteria {
	// Set defaults
	if !criteria.Limit.Valid || criteria.Limit.Int64 <= 0 {
		criteria.Limit = optional.NewInt64(100)
	} else if !criteria.Offset.Valid || criteria.Offset.Int64 < 0 {
		criteria.Offset = optional.NewInt64(0)
	}
	if !criteria.From.Valid {
		criteria.From = models.DefaultFrom(l.clock)
	}
	if !criteria.To.Valid {
		criteria.To = models.DefaultTo(l.clock)
	}

	return logsdb.Criteria{
		Sources:     criteria.Sources,
		FromSeconds: criteria.From.Time.Unix(),
		FromMS:      int64(criteria.From.Time.Nanosecond() / 1e6),
		ToSeconds:   criteria.To.Time.Unix(),
		ToMS:        int64(criteria.To.Time.Nanosecond() / 1e6),
		Order:       criteria.Order,
		Offset:      criteria.Offset.Int64,
		Limit:       criteria.Limit.Int64,
		Levels:      criteria.Levels,
		Filters:     criteria.Filters,
	}
}
