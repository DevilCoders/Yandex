package provider

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
)

type MongoDB struct {
	cfg            logic.Config
	operator       clusters.Operator
	backups        backups.Backups
	events         events.Events
	tasks          tasks.Tasks
	cryptoProvider crypto.Crypto
}

var _ mongodb.MongoDB = &MongoDB{}

func New(cfg logic.Config, operator clusters.Operator, backups backups.Backups, events events.Events, tasks tasks.Tasks, cryptoProvider crypto.Crypto) *MongoDB {
	return &MongoDB{cfg: cfg, operator: operator, backups: backups, events: events, tasks: tasks, cryptoProvider: cryptoProvider}
}
