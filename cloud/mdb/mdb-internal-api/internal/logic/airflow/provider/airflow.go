package provider

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets"
	"a.yandex-team.ru/library/go/core/log"
)

type Airflow struct {
	cfg            logic.Config
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

var _ airflow.Airflow = &Airflow{}

func New(
	cfg logic.Config,
	operator clusters.Operator,
	backups backups.Backups,
	events events.Events,
	search search.Docs,
	tasks tasks.Tasks,
	cryptoProvider crypto.Crypto,
	compute compute.Compute,
	pillarSecrets pillarsecrets.PillarSecretsClient,
	l log.Logger,
) *Airflow {
	return &Airflow{
		cfg:            cfg,
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
