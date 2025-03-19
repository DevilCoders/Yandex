package healthbased

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased/healthdbspec"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type HealthBasedHealthiness struct {
	health healthapi.MDBHealthClient
	rr     healthdbspec.RoleSpecificResolvers
	now    time.Time
}

const ProdEnvName = "prod"

func isNotProd(ni types.HostNeighboursInfo) bool {
	return ni.Env == ProdEnvName && ni.SameRolesTotal == 0 && !ni.HAShard && !ni.HACluster
}

func NewHealthBasedHealthiness(
	health healthapi.MDBHealthClient,
	rr healthdbspec.RoleSpecificResolvers,
	now time.Time,
) healthiness.Healthiness {
	return &HealthBasedHealthiness{
		health: health, rr: rr,
		now: now,
	}
}
