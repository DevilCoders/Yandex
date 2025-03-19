package main

import (
	"database/sql"
	"fmt"
	"time"

	l "a.yandex-team.ru/library/go/core/log"
)

type hostInfo struct {
	Name     string `json:"name"`
	connStr  string
	DC       string `json:"dc"`
	prioDiff int
	partID   int
}

type hostState struct {
	IsAlive        bool    `json:"alive"`
	IsPrimary      bool    `json:"primary"`
	ReplicationLag uint    `json:"replication_lag"`
	SessionsRatio  float64 `json:"sessions_ratio"`
}

var defaultHostState = hostState{
	IsAlive:        false,
	IsPrimary:      false,
	ReplicationLag: 0,
	SessionsRatio:  0,
}

type hostAux struct {
	connectionPool *sql.DB
	statesChan     chan hostState
	LastStates     []hostState `json:"last_states"`
}

type host struct {
	hostInfo
	hostState
	hostPrio
	hostAux
}

func (pgc *pgcheck) buildHostInfo(db *database, hostname string) *host {
	bf := []l.Field{
		l.String("func", "buildHostInfo"),
		l.String("hostname", hostname),
		l.String("dbname", db.name),
	}

	var h host
	var (
		dc       sql.NullString
		prioDiff sql.NullInt64
	)
	// We assume here that one host may be strictly in one shard
	row := db.pool.QueryRow(
		`SELECT h.host_name, c.conn_string, h.dc, h.prio_diff, p.part_id, p.priority
		   FROM plproxy.priorities p JOIN
				plproxy.hosts h USING (host_id) JOIN
				plproxy.connections c USING (conn_id)
		  WHERE host_name = $1`, hostname)
	err := row.Scan(&h.Name, &h.connStr, &dc, &prioDiff, &h.partID, &h.CurrentPrio)
	if err != nil {
		pgc.logger.Error("invalid host", append(bf[:],
			l.NamedError("error", err),
		)...)
	}

	// We don't want to change priorities immediately after pgcheck restart,
	// especially to nil value (0).
	h.NeededPrio = h.CurrentPrio

	if dc.Valid {
		h.DC = dc.String
	}
	if prioDiff.Valid {
		h.prioDiff = int(prioDiff.Int64)
	}
	h.connStr = fmt.Sprintf("%s %s", h.connStr, db.config.AppendConnString)

	maxStatesCount := int(db.config.Quorum + db.config.Hysterisis)
	h.statesChan = make(chan hostState, maxStatesCount*3)
	h.LastStates = make([]hostState, 0, maxStatesCount)

	h.connectionPool, err = createPool(pgc.logger, &h.connStr, false)
	if err != nil {
		pgc.logger.Error("could not create connection pool", append(bf[:],
			l.NamedError("error", err),
		)...)
	}

	return &h
}

func (pgc *pgcheck) getHostState(host *host, dbname string) *hostState {
	dbConfig := pgc.config.Databases[dbname]
	maxStatesCount := int(dbConfig.Quorum + dbConfig.Hysterisis)
	go pgc.sendStateToStatesChan(host, dbname)

	if len(host.statesChan) > maxStatesCount {
		for i := maxStatesCount; i < len(host.statesChan); i++ {
			<-host.statesChan
		}
	}

	var state hostState
	timeout := time.Second*pgc.config.Timeout + 100*time.Millisecond

	bf := []l.Field{
		l.String("func", "getHostState"),
		l.String("hostname", host.Name),
		l.Duration("timeout", timeout),
		l.String("dbname", dbname),
	}

	select {
	case x := <-host.statesChan:
		state = x
	case <-time.After(timeout):
		pgc.logger.Warn("could not get status of host in timeout", bf...)
		state = defaultHostState
	}

	return &state
}

func (pgc *pgcheck) sendStateToStatesChan(host *host, dbname string) {
	c := host.statesChan

	db, err := getPool(pgc.logger, host)
	if err != nil {
		pgc.logger.Error("connection to host failed", []l.Field{
			l.String("func", "sendStateToStatesChan"),
			l.String("hostname", host.Name),
			l.String("dbname", dbname),
			l.NamedError("error", err),
		}...)

		c <- defaultHostState
		return
	}

	state := pgc.fillState(db, dbname, host)
	c <- state
}

func (pgc *pgcheck) fillState(db *sql.DB, dbname string, host *host) hostState {
	state := defaultHostState

	var isMaster bool
	var replicationLag uint
	var sessionsRatio float64
	row := db.QueryRow(`SELECT is_master, lag, sessions_ratio
		FROM public.pgcheck_poll()`)
	err := row.Scan(&isMaster, &replicationLag, &sessionsRatio)
	if err != nil {
		pgc.logger.Error("checking host failed", []l.Field{
			l.String("func", "fillState"),
			l.String("hostname", host.Name),
			l.String("dbname", dbname),
			l.NamedError("error", err),
		}...)
		return state
	}

	state.IsAlive = true
	state.IsPrimary = isMaster
	state.ReplicationLag = replicationLag
	state.SessionsRatio = sessionsRatio
	return state
}

func (pgc *pgcheck) updateLastStates(host *host, state *hostState, maxStatesCount int) *[]hostState {
	var startFrom int
	if len(host.LastStates) != maxStatesCount {
		startFrom = 0
	} else {
		startFrom = 1
	}
	result := append(host.LastStates[startFrom:], *state)
	pgc.logger.Debugf("last states of host %s are: %v", host.Name, result)
	return &result
}

func updateState(hostsInfo *map[string]*host, hostname string, state *hostState, quorum uint, config Config) {
	hosts := *hostsInfo
	host := hosts[hostname]
	neededPrio := stateToPrio(host, state, config)

	if prioIsNear(host.CurrentPrio, neededPrio, config) && neededPrio != alivePrimaryPrio {
		hosts[hostname].hostState = *state
		return
	}

	var cnt uint
	for i := range hosts[hostname].LastStates {
		prio := stateToPrio(host, &host.LastStates[i], config)
		if prioIsNear(prio, neededPrio, config) {
			cnt++
		}
	}
	if cnt >= quorum {
		hosts[hostname].NeededPrio = neededPrio
		hosts[hostname].hostState = *state
	} else {
		hosts[hostname].NeededPrio = hosts[hostname].CurrentPrio
	}
}
