package provider

import (
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider/hostname"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/health"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/library/go/core/log"
)

type Clusters struct {
	metaDB                  metadb.Backend
	clusterIDGenerator      generator.IDGenerator
	subClusterIDGenerator   generator.IDGenerator
	shardIDGenerator        generator.IDGenerator
	pillarIDGenerator       generator.IDGenerator
	hostnameGenerator       hostname.ClusterHostnameGenerator
	clusterSecretsGenerator ClusterSecrets
	sessions                sessions.Sessions
	health                  health.Health
	cfg                     logic.Config
	l                       log.Logger
}

var (
	_ clusters.Operator = &Operator{}
	_ clusters.Creator  = &Clusters{}
	_ clusters.Restorer = &Clusters{}
	_ clusters.Reader   = &Clusters{}
	_ clusters.Modifier = &Clusters{}
)

func NewClusters(
	metaDB metadb.Backend,
	clusterIDGen generator.IDGenerator,
	subClusterIDGen generator.IDGenerator,
	shardIDGen generator.IDGenerator,
	pillarIDGen generator.IDGenerator,
	hostnameGen hostname.ClusterHostnameGenerator,
	clusterSecretsGen ClusterSecrets,
	sessions sessions.Sessions,
	health health.Health,
	cfg logic.Config,
	l log.Logger,
) *Clusters {
	return &Clusters{
		metaDB:                  metaDB,
		clusterIDGenerator:      clusterIDGen,
		subClusterIDGenerator:   subClusterIDGen,
		shardIDGenerator:        shardIDGen,
		pillarIDGenerator:       pillarIDGen,
		hostnameGenerator:       hostnameGen,
		clusterSecretsGenerator: clusterSecretsGen,
		sessions:                sessions,
		health:                  health,
		cfg:                     cfg,
		l:                       l,
	}
}

func NewOperator(
	sessions sessions.Sessions,
	metadb metadb.Backend,
	creator clusters.Creator,
	restorer clusters.Restorer,
	reader clusters.Reader,
	modifier clusters.Modifier,
	resourceModel environment.ResourceModel,
	logicConfig *logic.Config,
	l log.Logger,
) *Operator {
	return &Operator{
		sessions:      sessions,
		metadb:        metadb,
		creator:       creator,
		restorer:      restorer,
		reader:        reader,
		modifier:      modifier,
		resourceModel: resourceModel,
		logicConfig:   logicConfig,
		l:             l,
	}
}
