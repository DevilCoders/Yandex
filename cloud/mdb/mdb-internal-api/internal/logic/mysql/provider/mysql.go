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
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/library/go/core/log"
)

type MySQL struct {
	L        log.Logger
	cfg      logic.Config
	operator clusters.Operator
	backups  backups.Backups
	events   events.Events
	tasks    tasks.Tasks
	compute  compute.Compute
	console  console.Console
}

var _ mysql.MySQL = &MySQL{}

func New(cfg logic.Config,
	logger log.Logger,
	operator clusters.Operator,
	backups backups.Backups,
	events events.Events,
	tasks tasks.Tasks,
	compute compute.Compute,
	console console.Console,
) *MySQL {
	return &MySQL{
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

func (m MySQL) GetDefaultVersions(ctx context.Context) ([]consolemodels.DefaultVersion, error) {
	// FIXME: why SaltEnvs.Production??
	return m.console.GetDefaultVersions(ctx, clustermodels.TypeMySQL, m.cfg.SaltEnvs.Production, mymodels.MySQLMetaDBComponent)
}

func (m MySQL) GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute2.HostGroupHostType, error) {
	return m.compute.GetHostGroupHostType(ctx, hostGroupIds)
}
