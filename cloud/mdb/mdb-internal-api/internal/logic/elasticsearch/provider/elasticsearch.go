package provider

import (
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/library/go/core/log"
)

type ElasticSearch struct {
	cfg            logic.Config
	auth           auth.Authenticator
	operator       clusters.Operator
	backups        backups.Backups
	events         events.Events
	search         search.Docs
	tasks          tasks.Tasks
	compute        compute.Compute
	cryptoProvider crypto.Crypto
	extIDGen       generator.IDGenerator
	l              log.Logger

	supportedVersions esmodels.SupportedVersions
	allowedEditions   []esmodels.Edition
}

var _ elasticsearch.ElasticSearch = &ElasticSearch{}

func New(
	cfg logic.Config,
	auth auth.Authenticator,
	operator clusters.Operator,
	backups backups.Backups,
	events events.Events,
	search search.Docs,
	tasks tasks.Tasks,
	compute compute.Compute,
	cryptoProvider crypto.Crypto,
	extIDGen generator.IDGenerator,
	l log.Logger,
) *ElasticSearch {
	return &ElasticSearch{
		cfg:               cfg,
		auth:              auth,
		operator:          operator,
		backups:           backups,
		events:            events,
		search:            search,
		tasks:             tasks,
		compute:           compute,
		cryptoProvider:    cryptoProvider,
		extIDGen:          extIDGen,
		l:                 l,
		supportedVersions: esmodels.MustLoadVersions(cfg.Elasticsearch.Versions),
		allowedEditions:   esmodels.MustLoadAllowedEditions(cfg.Elasticsearch.AllowedEditions),
	}
}
