package provider

import (
	"context"

	compute2 "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/library/go/core/log"
)

type PostgreSQL struct {
	L        log.Logger
	cfg      logic.Config
	operator clusters.Operator
	backups  backups.Backups
	events   events.Events
	tasks    tasks.Tasks
	compute  compute.Compute
	console  console.Console
}

var _ postgresql.PostgreSQL = &PostgreSQL{}

func New(cfg logic.Config,
	logger log.Logger,
	operator clusters.Operator,
	backups backups.Backups,
	events events.Events,
	tasks tasks.Tasks,
	compute compute.Compute,
	console console.Console,
) *PostgreSQL {
	return &PostgreSQL{
		cfg:      cfg,
		L:        logger,
		operator: operator,
		backups:  backups,
		events:   events,
		tasks:    tasks,
		compute:  compute,
		console:  console,
	}
}

func (pg *PostgreSQL) GetDefaultVersions(ctx context.Context) ([]consolemodels.DefaultVersion, error) {
	// FIXME: why SaltEnvs.Production??
	return pg.console.GetDefaultVersions(ctx, clustermodels.TypePostgreSQL, pg.cfg.SaltEnvs.Production, pgmodels.PostgreSQLMetaDBComponent)

}

func (pg *PostgreSQL) GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute2.HostGroupHostType, error) {
	return pg.compute.GetHostGroupHostType(ctx, hostGroupIds)
}
