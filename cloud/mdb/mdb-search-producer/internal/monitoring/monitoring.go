// Package monitoring provides monrun check for mdb-search-producer
package monitoring

import (
	"context"
	"fmt"
	"time"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	monrunrunner "a.yandex-team.ru/cloud/mdb/internal/monrun/runner"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/app"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type Monitoring struct {
	Cfg    app.Config
	logger log.Logger
}

func New(logger log.Logger) (*Monitoring, error) {
	cfg := app.DefaultConfig()

	if err := config.Load(app.ConfigName, &cfg); err != nil {
		return nil, xerrors.Errorf("failed to load application config: %w", err)
	}
	return NewFromAppConfig(logger, cfg), nil
}

func NewFromAppConfig(logger log.Logger, cfg app.Config) *Monitoring {
	return &Monitoring{Cfg: cfg, logger: logger}
}

func (m *Monitoring) setupMDB(ctx context.Context) (metadb.MetaDB, error) {
	m.logger.Debugf("metadb config is %+v", m.Cfg.MetaDB)

	cluster, err := pgutil.NewCluster(m.Cfg.MetaDB, sqlutil.WithTracer(tracers.Log(m.logger)))
	if err != nil {
		return nil, xerrors.Errorf("construct cluster: %w", err)
	}

	if _, err = cluster.WaitForAlive(ctx); err != nil {
		_ = cluster.Close()
		return nil, xerrors.Errorf("connect to metadb: %w", err)
	}

	mdb := mdbpg.New(cluster, m.logger)
	return mdb, nil
}

func (m *Monitoring) CheckDocumentsAge(ctx context.Context, warn time.Duration, crit time.Duration) monrun.Result {
	mdb, setupErr := m.setupMDB(ctx)
	if setupErr != nil {
		return monrun.Warnf("Unable to setup mdb: %s", setupErr)
	}

	var lastAge time.Duration
	var lastMessage string

	nonEnumeratedDocs, err := mdb.NonEnumeratedSearchDocs(ctx, 1)
	if err != nil {
		return monrun.Critf("Unable to run NonEnumeratedSearchDocs query: %s", err)
	}
	for _, doc := range nonEnumeratedDocs {
		lastAge = time.Since(doc.CreatedAt)
		lastMessage = fmt.Sprintf("Exist not enumerated search_queue document id: %d with age: %s", doc.ID, lastAge)
	}

	unsentDocs, err := mdb.UnsentSearchDocs(ctx, 1)
	if err != nil {
		code := monrun.CRIT
		if pgerrors.IsTemporary(err) {
			code = monrun.WARN
		}
		return monrun.Result{
			Code:    code,
			Message: fmt.Sprintf("Unable to run UnsentSearchDocs query: %s", err),
		}
	}
	for _, doc := range unsentDocs {
		age := time.Since(doc.CreatedAt)
		if age > lastAge {
			lastAge = age
			lastMessage = fmt.Sprintf("Exist unsent search_queue document queue_id: %d with age: %s", doc.QueueID, age)
		}
	}

	if lastAge > crit {
		return monrun.Result{Code: monrun.CRIT, Message: lastMessage}
	}

	if lastAge > warn {
		return monrun.Result{Code: monrun.WARN, Message: lastMessage}
	}
	return monrun.Result{}
}

var (
	warnSince time.Duration
	critSince time.Duration
)

func init() {
	flags := pflag.NewFlagSet("SearchQueueAge", pflag.ExitOnError)
	flags.DurationVar(&warnSince, "warn", time.Minute*3, "Warn level")
	flags.DurationVar(&critSince, "crit", time.Minute*15, "Crit level")
	pflag.CommandLine.AddFlagSet(flags)
	_ = pflag.CommandLine.MarkHidden("mdb-loglevel")
}

func CheckSearchQueueDocumentsAge() {
	pflag.Parse()
	monrunrunner.RunCheck(func(ctx context.Context, logger log.Logger) monrun.Result {
		monitor, err := New(logger)
		if err != nil {
			return monrun.Warnf("initialization failed: %s", err)
		}
		return monitor.CheckDocumentsAge(ctx, warnSince, critSince)
	})
}
