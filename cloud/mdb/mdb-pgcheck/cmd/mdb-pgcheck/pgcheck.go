package main

import (
	"database/sql"
	"fmt"
	"os"
	"time"

	"github.com/spf13/pflag"

	l "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	appName         = "MDB PGCheck"
	binName         = "mdb-pgcheck"
	appDesc         = "Tool for monitoring backend databases from PL/Proxy hosts"
	maxChannelsSize = 100
)

type pgcheck struct {
	logger l.Logger
	config Config
}

func main() {
	pflag.Usage = func() {
		_, _ = fmt.Fprintf(os.Stderr, "%s Application\n%s\n\n", appName, appDesc)
		_, _ = fmt.Fprintf(os.Stderr, "Usage:\n  %s [OPTIONS]\n\n", binName)
		_, _ = fmt.Fprintln(os.Stderr, pflag.CommandLine.FlagUsages())
	}
	pflag.Parse()

	config := parseConfig()
	logger, err := zap.New(zap.KVConfig(config.LogLevel))
	if err != nil {
		fmt.Printf("failed to initialize logger: %s\n", err)
		os.Exit(1)
	}

	logger.Debugf("%v", config)

	pgc := &pgcheck{
		logger: logger,
		config: config,
	}

	ss := newStatsServer(config, logger)
	statsChan := make(chan statsState, maxChannelsSize)
	go ss.startStatsServer(statsChan)

	for db := range config.Databases {
		go pgc.processDB(db, statsChan)
	}

	handleSignals(logger)
}

func (pgc *pgcheck) processDB(dbname string, statsChan chan statsState) {
	var db database
	db.name = dbname
	db.config = pgc.config.Databases[dbname]
	db.pool, _ = createPool(pgc.logger, &db.config.LocalConnString, true)
	defer func() { _ = db.pool.Close() }()

	hosts := pgc.buildInitialHostsInfo(&db)
	shards := buildShardsInfo(hosts)

	for {
		pgc.updateHostsState(hosts, dbname)
		pgc.correctPrioForHostsInShard(dbname, shards, hosts)
		pgc.updatePriorities(db.pool, hosts)
		pgc.sendStats(statsChan, dbname, hosts, shards)
		time.Sleep(time.Second)
	}
}

func (pgc *pgcheck) buildInitialHostsInfo(db *database) *map[string]*host {
	query := "SELECT distinct(host_name) FROM plproxy.hosts"
	bf := []l.Field{
		l.String("func", "buildInitialHostsInfo"),
		l.String("query", query),
		l.String("dbname", db.name),
	}
	rows, err := db.pool.Query(query)
	if err != nil {
		pgc.logger.Fatal("query failed", append(bf[:],
			l.NamedError("error", err),
		)...)
	}
	defer func() { _ = rows.Close() }()

	hosts := make(map[string]*host)

	for rows.Next() {
		var hostname string
		if err := rows.Scan(&hostname); err != nil {
			pgc.logger.Error("reading row failed", append(bf[:],
				l.NamedError("error", err),
			)...)
		}

		hosts[hostname] = pgc.buildHostInfo(db, hostname)
	}

	if err := rows.Err(); err != nil {
		pgc.logger.Error("reading rows failed", append(bf[:],
			l.NamedError("error", err),
		)...)
	}

	return &hosts
}

func buildShardsInfo(hostsInfo *map[string]*host) *map[int][]string {
	hosts := *hostsInfo
	shards := make(map[int][]string)
	for hostname := range hosts {
		shard := hosts[hostname].partID
		shards[shard] = append(shards[shard], hostname)
	}
	return &shards
}

func (pgc *pgcheck) updateHostsState(hostsInfo *map[string]*host, dbname string) {
	dbc := pgc.config.Databases[dbname]
	maxStatesCount := int(dbc.Quorum + dbc.Hysterisis)

	hosts := *hostsInfo
	for hostname := range hosts {
		host := hosts[hostname]
		state := pgc.getHostState(host, dbname)
		hosts[hostname].LastStates = *pgc.updateLastStates(host, state, maxStatesCount)
		updateState(&hosts, hostname, state, dbc.Quorum, pgc.config)
	}
}

func (pgc *pgcheck) correctPrioForHostsInShard(dbname string, shardsInfo *map[int][]string, hostsInfo *map[string]*host) {
	shards := *shardsInfo
	hosts := *hostsInfo

	bf := []l.Field{
		l.String("func", "correctPrioForHostsInShard"),
		l.String("dbname", dbname),
	}
	for partID, hostsList := range shards {
		var masters []string
		for _, h := range hostsList {
			if hosts[h].IsAlive && hosts[h].IsPrimary {
				masters = append(masters, h)
			}
		}
		bfp := append(bf[:],
			l.Int("shard_id", partID),
			l.Int("master_count", len(masters)),
			l.String("masters", fmt.Sprintf("%v", masters)),
		)
		if len(masters) > 1 {
			pgc.logger.Warn("marking all masters in shard as dead", bfp...)

			for _, h := range masters {
				hosts[h].NeededPrio = deadHostPrio
			}
		} else if len(masters) == 1 {
			pgc.logger.Debug("single master in shard", append(bfp[:],
				l.String("master", masters[0]),
			)...)
		} else {
			pgc.logger.Warn("no master in shard", bfp...)
		}

		if len(masters) != 1 {
			for _, h := range hostsList {
				if !hosts[h].IsPrimary && hosts[h].IsAlive {
					pgc.logger.Warn("not accounting replication lag for host since master is dead",
						append(bfp[:], l.String("host", hosts[h].Name))...)
					hosts[h].NeededPrio =
						hosts[h].NeededPrio - priority(hosts[h].ReplicationLag)
				}
			}
		}
	}
}

func (pgc *pgcheck) updatePriorities(db *sql.DB, hostsInfo *map[string]*host) {
	hosts := *hostsInfo
	for hostname := range hosts {
		s := hosts[hostname]
		bf := []l.Field{
			l.String("func", "updatePriorities"),
			l.UInt("old_priority", uint(s.CurrentPrio)),
			l.UInt("new_priority", uint(s.NeededPrio)),
			l.String("hostname", hostname),
		}
		if s.CurrentPrio != s.NeededPrio {
			hosts[hostname].CurrentPrio = s.NeededPrio
			updateHostPriority(pgc.logger, db, hostname, s.NeededPrio)
			pgc.logger.Info("priority of host has been changed", bf...)
		}
	}
}

func (pgc *pgcheck) sendStats(ch chan statsState, dbname string, hosts *map[string]*host, shards *map[int][]string) {
	if len(ch) == maxChannelsSize {
		pgc.logger.Error("could not send stats for DB since channel is full", []l.Field{
			l.String("func", "sendStats"),
			l.String("dbname", dbname),
		}...)
		return
	}
	ch <- statsState{dbname, *hosts, *shards}
}
