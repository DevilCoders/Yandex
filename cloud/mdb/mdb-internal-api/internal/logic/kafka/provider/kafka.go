package provider

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets"
	"a.yandex-team.ru/library/go/core/log"
)

type Kafka struct {
	cfg            logic.Config
	domainConfig   api.DomainConfig
	operator       clusters.Operator
	backups        backups.Backups
	events         events.Events
	search         search.Docs
	tasks          tasks.Tasks
	cryptoProvider crypto.Crypto
	compute        compute.Compute
	pillarSecrets  pillarsecrets.PillarSecretsClient
	l              log.Logger
}

var _ kafka.Kafka = &Kafka{}

func New(
	cfg logic.Config,
	domainConfig api.DomainConfig,
	operator clusters.Operator,
	backups backups.Backups,
	events events.Events,
	search search.Docs,
	tasks tasks.Tasks,
	cryptoProvider crypto.Crypto,
	compute compute.Compute,
	pillarSecrets pillarsecrets.PillarSecretsClient,
	l log.Logger,
) *Kafka {
	return &Kafka{
		cfg:            cfg,
		domainConfig:   domainConfig,
		operator:       operator,
		backups:        backups,
		events:         events,
		search:         search,
		tasks:          tasks,
		cryptoProvider: cryptoProvider,
		compute:        compute,
		pillarSecrets:  pillarSecrets,
		l:              l,
	}
}
