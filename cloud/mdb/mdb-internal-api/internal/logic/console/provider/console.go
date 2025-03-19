package provider

import (
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/health"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type Console struct {
	cfg      logic.Config
	auth     auth.Authenticator
	sessions sessions.Sessions
	metaDB   metadb.Backend
	health   health.Health
	network  networkProvider.Client

	clusterSpecific map[clusters.Type]console.ClusterSpecific
}

var _ console.Console = &Console{}

func New(cfg logic.Config, auth auth.Authenticator, sessions sessions.Sessions, metaDB metadb.Backend, health health.Health,
	network networkProvider.Client, clusterSpecific map[clusters.Type]console.ClusterSpecific) *Console {
	return &Console{
		cfg:             cfg,
		auth:            auth,
		metaDB:          metaDB,
		sessions:        sessions,
		health:          health,
		network:         network,
		clusterSpecific: clusterSpecific,
	}
}
