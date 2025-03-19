// Package monitoring provides monrun check for mdb-event-producer
package monitoring

import (
	"context"
	"fmt"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	monrunrunner "a.yandex-team.ru/cloud/mdb/internal/monrun/runner"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/app"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type Monitoring struct {
	Cfg    app.Config
	logger log.Logger
}

func New(logger log.Logger) *Monitoring {
	cfg := app.LoadConfig()
	return NewFromAppConfig(logger, cfg)
}

func NewFromAppConfig(logger log.Logger, cfg app.Config) *Monitoring {
	return &Monitoring{Cfg: cfg, logger: logger}
}

func (m *Monitoring) setupMDB(ctx context.Context) (metadb.MetaDB, error) {
	m.logger.Debugf("metadb config is %+v", m.Cfg.MetaDB)

	cluster, err := pgutil.NewCluster(m.Cfg.MetaDB, sqlutil.WithTracer(tracers.Log(m.logger)))
	if err != nil {
		return nil, xerrors.Errorf("Unable to construct mdb: %w", err)
	}

	if _, err = cluster.WaitForAlive(ctx); err != nil {
		_ = cluster.Close()
		return nil, xerrors.Errorf("unable to connect to mdb: %w", err)
	}

	mdb := mdbpg.NewWithCluster(cluster, m.logger)
	return mdb, nil
}

func (m *Monitoring) CheckEventsAge(ctx context.Context, warn time.Duration, crit time.Duration) monrun.Result {
	mdb, err := m.setupMDB(ctx)
	if err != nil {
		return monrun.Warnf("MetaDB initialization fail: %s", err)
	}

	ev, err := mdb.OldestUnsetStartEvent(ctx)
	if err != nil {
		code := monrun.CRIT
		if pgerrors.IsTemporary(err) {
			code = monrun.WARN
		}
		return monrun.Result{
			Code:    code,
			Message: err.Error(),
		}
	}
	if ev == (metadb.WorkerQueueEvent{}) {
		return monrun.Result{}
	}
	eventAge := time.Since(ev.CreatedAt)
	if eventAge > warn {
		code := monrun.WARN
		message := fmt.Sprintf("Exists unsent start event %d. With age: %s", ev.ID, eventAge)
		if eventAge > crit {
			code = monrun.CRIT
		}
		return monrun.Result{
			Code:    code,
			Message: message,
		}
	}
	return monrun.Result{}
}

var (
	warnSince time.Duration
	critSince time.Duration
)

func init() {
	flags := pflag.NewFlagSet("EventsAge", pflag.ExitOnError)
	flags.DurationVar(&warnSince, "warn", time.Minute*3, "Warn level")
	flags.DurationVar(&critSince, "crit", time.Minute*15, "Crit level")
	pflag.CommandLine.AddFlagSet(flags)
	_ = pflag.CommandLine.MarkHidden("mdb-loglevel")
}

func CheckEventsAge() {
	pflag.Parse()
	monrunrunner.RunCheck(func(ctx context.Context, logger log.Logger) monrun.Result {
		monitor := New(logger)
		return monitor.CheckEventsAge(ctx, warnSince, critSince)
	})
}
