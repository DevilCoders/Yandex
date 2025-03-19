package provider

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	validators "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets"
	"a.yandex-team.ru/library/go/core/log"
)

type ClickHouse struct {
	cfg            logic.Config
	operator       clusterslogic.Operator
	backups        backups.Backups
	events         events.Events
	search         search.Docs
	tasks          tasks.Tasks
	compute        compute.Compute
	cryptoProvider crypto.Crypto
	uriValidator   validators.URIValidator
	pillarSecrets  pillarsecrets.PillarSecretsClient
	versions       chmodels.CHVersions
}

var _ clickhouse.ClickHouse = &ClickHouse{}

func New(
	cfg logic.Config,
	operator clusterslogic.Operator,
	backups backups.Backups,
	events events.Events,
	search search.Docs,
	tasks tasks.Tasks,
	compute compute.Compute,
	cryptoProvider crypto.Crypto,
	pillarSecrets pillarsecrets.PillarSecretsClient,
	l log.Logger,
) *ClickHouse {

	var uriValidator validators.URIValidator
	if cfg.ClickHouse.ExternalURIValidation.UseHTTPClient {
		uriValidator = validators.MustNewHTTPURIValidator(cfg.ClickHouse.ExternalURIValidation, l)
	} else {
		uriValidator = validators.DummyURIValidator{}
	}

	var versions chmodels.CHVersions
	if len(cfg.ClickHouse.Versions) > 0 {
		versions = chmodels.VersionsFromConfig(cfg.ClickHouse)
	}

	return &ClickHouse{
		cfg:            cfg,
		operator:       operator,
		backups:        backups,
		events:         events,
		search:         search,
		tasks:          tasks,
		compute:        compute,
		cryptoProvider: cryptoProvider,
		uriValidator:   uriValidator,
		pillarSecrets:  pillarSecrets,
		versions:       versions,
	}
}

type cluster struct {
	clusters.Cluster
	Pillar *chpillars.ClusterCH
}

type subCluster struct {
	clusters.SubCluster
	Pillar *chpillars.SubClusterCH
}

type zkSubCluster struct {
	clusters.SubCluster
	Pillar *chpillars.SubClusterZK
}

type shard struct {
	clusters.Shard
	Pillar *chpillars.ShardCH
}
