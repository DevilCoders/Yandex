package main

import (
	"encoding/json"
	"fmt"
	"net/http"

	l "a.yandex-team.ru/library/go/core/log"
)

type statsState struct {
	dbname    string
	hostsMap  map[string]*host
	shardsMap map[int][]string
}

// StatsState is a type for marshaling to JSON
type StatsState struct {
	HostsMap  map[string]host  `json:"hosts"`
	ShardsMap map[int][]string `json:"shards"`
}

// Statistics contains current statistics
type Statistics struct {
	TotalShards      uint `json:"total_shards"`
	NormalShards     uint `json:"normal_shards"`
	ReadOnlyShards   uint `json:"read_only_shards"`
	NoReplicShards   uint `json:"shards_without_replicas"`
	SplitBrainShards uint `json:"split_brain_shards"`
	FullyDeadShards  uint `json:"fully_dead_shards"`
	AliveHosts       uint `json:"alive_hosts"`
	DeadHosts        uint `json:"dead_hosts"`
}

type statServ struct {
	currentState map[string]StatsState
	logger       l.Logger
	config       Config
}

func (ss statServ) currentStateHandler(w http.ResponseWriter, r *http.Request) {
	state, err := json.Marshal(ss.currentState)
	if err != nil {
		ss.logger.Error("marshaling state failed", []l.Field{
			l.String("func", "currentStateHandler"),
			l.NamedError("error", err),
		}...)
		return
	}
	w.Header().Set("Content-type", "application/json")
	_, _ = fmt.Fprint(w, string(state))
}

func (ss statServ) statisticsHandler(w http.ResponseWriter, r *http.Request) {
	var statistic Statistics
	for _, state := range ss.currentState {
		s := state
		for shardID := range s.ShardsMap {
			countStatsForShard(&statistic, &s, shardID)
		}
	}

	stats, err := json.Marshal(statistic)
	if err != nil {
		ss.logger.Error("marshaling statistics failed", []l.Field{
			l.String("func", "statisticsHandler"),
			l.NamedError("error", err),
		}...)
		return
	}
	w.Header().Set("Content-type", "application/json")
	_, _ = fmt.Fprint(w, string(stats))
}

func newStatsServer(config Config, logger l.Logger) *statServ {
	return &statServ{
		config:       config,
		logger:       logger,
		currentState: make(map[string]StatsState),
	}
}

func (ss statServ) startStatsServer(ch chan statsState) {
	for db := range ss.config.Databases {
		ss.currentState[db] = StatsState{}
	}

	go ss.startUpdatingStats(ch)

	http.HandleFunc("/state", ss.currentStateHandler)
	http.HandleFunc("/stats", ss.statisticsHandler)
	err := http.ListenAndServe(fmt.Sprintf(":%d", ss.config.HTTPPort), nil)
	ss.logger.Warn("binding to address failed", []l.Field{
		l.String("func", "startStatsServer"),
		l.NamedError("error", err),
	}...)
}

func (ss statServ) startUpdatingStats(ch chan statsState) {
	for {
		x := <-ch
		ss.currentState[x.dbname] = state2State(x)
	}
}

func countStatsForShard(statistic *Statistics, state *StatsState, shardID int) {
	shardHosts := state.ShardsMap[shardID]
	aliveMasters := 0
	aliveReplics := 0
	notDelayedReplics := 0

	for _, fqdn := range shardHosts {
		hostState := state.HostsMap[fqdn]

		if hostState.IsAlive {
			statistic.AliveHosts++
			if hostState.IsPrimary {
				aliveMasters++
			} else {
				aliveReplics++
				if hostState.ReplicationLag < 90 {
					notDelayedReplics++
				}
			}
		} else {
			statistic.DeadHosts++
		}
	}

	statistic.TotalShards++
	if aliveMasters == 0 {
		if aliveReplics == 0 {
			statistic.FullyDeadShards++
		} else {
			statistic.ReadOnlyShards++
		}
	} else if aliveMasters > 1 {
		statistic.SplitBrainShards++
		// Below aliveMasters == 1
	} else if aliveReplics == 0 || notDelayedReplics == 0 {
		statistic.NoReplicShards++
	} else if aliveReplics > 0 {
		statistic.NormalShards++
	}
}

func state2State(in statsState) StatsState {
	tmpState := StatsState{}
	tmpState.ShardsMap = in.shardsMap

	tmpState.HostsMap = make(map[string]host)
	for k := range in.hostsMap {
		tmpState.HostsMap[k] = *in.hostsMap[k]
	}

	return tmpState
}
