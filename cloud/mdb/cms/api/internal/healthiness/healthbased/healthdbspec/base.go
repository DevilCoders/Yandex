package healthdbspec

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

const ProdEnvName = "prod"

type RoleResolver struct {
	RoleMatch func(string) bool
	OkToLetGo func(types.HostNeighboursInfo) bool
}

type RoleSpecificResolvers []RoleResolver

func (r RoleSpecificResolvers) OkToLetGo(ni types.HostNeighboursInfo) bool {
	for _, role := range ni.Roles {
		gotMatch := false
		for _, item := range r {
			if item.RoleMatch(role) {
				gotMatch = true
				if !item.OkToLetGo(ni) {
					return false
				}
			}
		}
		if !gotMatch {
			return defaultHealthCondition(ni)
		}
	}
	// all roles OK to give away
	return true
}

func StaleCondition(ni types.HostNeighboursInfo, now time.Time) (string, bool) {
	if !ni.HACluster && !ni.HAShard {
		return "", false
	} else if ni.SameRolesTS.IsZero() {
		return "unknown time", true
	} else if now.Sub(ni.SameRolesTS) > time.Minute*2 {
		return now.Sub(ni.SameRolesTS).Round(10 * time.Second).String(), true
	}
	return "", false
}

func configurationBasedCondition(ni types.HostNeighboursInfo) bool {
	if ni.Env != ProdEnvName {
		return true
	}
	if ni.SameRolesTotal == 0 {
		return true // one legged
	}
	if !(ni.HAShard || ni.HACluster) {
		return true // non HA
	}
	return false
}

func defaultHealthCondition(ni types.HostNeighboursInfo) bool {
	if configurationBasedCondition(ni) {
		return true
	}
	return ni.SameRolesTotal == ni.SameRolesAlive && ni.SameRolesTotal >= 2
}

func defaultNoQuorum(ni types.HostNeighboursInfo) bool {
	if defaultHealthCondition(ni) {
		return true
	}
	return ni.SameRolesAlive == ni.SameRolesTotal
}

func NewRoleSpecificResolver() RoleSpecificResolvers {
	result := RoleSpecificResolvers{}
	result = append(result, registerZK()...)
	result = append(result, registerPG()...)
	result = append(result, registerMySQL()...)
	result = append(result, registerRedis()...)
	result = append(result, registerMongodb()...)
	result = append(result, registerElasticsearch()...)
	result = append(result, registerGreenplum()...)
	return result
}
