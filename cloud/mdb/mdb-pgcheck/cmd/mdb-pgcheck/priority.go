package main

import (
	"database/sql"

	l "a.yandex-team.ru/library/go/core/log"
)

type priority uint

const (
	alivePrimaryPrio priority = 0
	aliveStandbyPrio priority = 10
	deadHostPrio     priority = 100
)

type hostPrio struct {
	CurrentPrio priority
	NeededPrio  priority
}

func prioIsNear(currentPrio, newPrio priority, config Config) bool {
	magic := config.NearPrioMagic
	lower := int(currentPrio) - magic
	upper := int(currentPrio) + magic
	if int(newPrio) >= lower && int(newPrio) <= upper {
		return true
	}
	return false
}

func stateToPrio(host *host, state *hostState, config Config) priority {
	if !state.IsAlive {
		return deadHostPrio
	} else if state.IsPrimary {
		return alivePrimaryPrio
	}

	prio := aliveStandbyPrio
	if host.DC != config.DC {
		prio += config.OtherDCPrioIncrease
	}
	prio += priority(float32(state.ReplicationLag) * config.ReplicationLagMultiplier)
	prio += priority(state.SessionsRatio * 100.0 / 2)

	// prioDiff may be negative so we cast everything to int first
	// and then back to priority type
	p := int(prio) + host.prioDiff
	if p < int(alivePrimaryPrio) {
		p = int(deadHostPrio) - p
	} else if p > int(alivePrimaryPrio) && p < int(aliveStandbyPrio) {
		p = int(aliveStandbyPrio)
	}
	prio = priority(p)

	if prio > 2*deadHostPrio {
		prio = 2 * deadHostPrio
	}

	return prio
}

func updateHostPriority(logger l.Logger, db *sql.DB, hostname string, prio priority) {
	_, err := db.Exec(
		`UPDATE plproxy.priorities
		    SET priority = $1
		  WHERE host_id = (
			  SELECT host_id FROM plproxy.hosts WHERE host_name = $2
			)`, prio, hostname)
	if err != nil {
		logger.Error("setting priority in db failed", []l.Field{
			l.String("func", "updateHostPriority"),
			l.String("hostname", hostname),
			l.NamedError("error", err),
		}...)
	}
}
