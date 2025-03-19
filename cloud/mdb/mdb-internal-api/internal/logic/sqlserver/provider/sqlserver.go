package provider

import (
	marketplace "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/library/go/core/log"
)

type SQLServer struct {
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
	license        marketplace.LicenseService
}

var _ sqlserver.SQLServer = &SQLServer{}

func New(cfg logic.Config,
	logger log.Logger,
	operator clusters.Operator,
	backups backups.Backups,
	events events.Events,
	search search.Docs,
	tasks tasks.Tasks,
	cryptoProvider crypto.Crypto,
	compute compute.Compute,
	consoleProvider console.Console,
	licenseClient marketplace.LicenseService) *SQLServer {
	return &SQLServer{
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
		license:        licenseClient,
	}
}
