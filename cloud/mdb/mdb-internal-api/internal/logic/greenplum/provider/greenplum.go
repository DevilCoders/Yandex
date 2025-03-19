package provider

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/library/go/core/log"
)

type Greenplum struct {
	L              log.Logger
	cfg            logic.Config
	operator       clusters.Operator
	backups        backups.Backups
	events         events.Events
	search         search.Docs
	tasks          tasks.Tasks
	cryptoProvider crypto.Crypto
	compute        compute.Compute
	console        console.Console
}

var _ greenplum.Greenplum = &Greenplum{}

func New(cfg logic.Config,
	logger log.Logger,
	operator clusters.Operator,
	backups backups.Backups,
	events events.Events,
	search search.Docs,
	tasks tasks.Tasks,
	cryptoProvider crypto.Crypto,
	compute compute.Compute,
	consoleProvider console.Console) *Greenplum {
	return &Greenplum{
		cfg:            cfg,
		L:              logger,
		operator:       operator,
		backups:        backups,
		events:         events,
		search:         search,
		tasks:          tasks,
		cryptoProvider: cryptoProvider,
		compute:        compute,
		console:        consoleProvider,
	}
}
