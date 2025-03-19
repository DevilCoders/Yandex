package core

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbsupport"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type clusterEnvStat struct {
	total       prometheus.Gauge
	alive       prometheus.Gauge
	degraded    prometheus.Gauge
	unknown     prometheus.Gauge
	dead        prometheus.Gauge
	hoststotal  prometheus.Gauge
	hostsread   prometheus.Gauge
	hostswrite  prometheus.Gauge
	dbtotal     prometheus.Gauge
	dbread      prometheus.Gauge
	dbwrite     prometheus.Gauge
	hostsBroken prometheus.Gauge
	dbBroken    prometheus.Gauge
}

type clustersStatByEnv map[string]clusterEnvStat
type statByCtype map[metadb.ClusterType]statForCtype
type statByCategory map[string]clustersStatByEnv // category like clusters, sla_clusters, shards, sla_shards

type statForCtype struct {
	categoryStat statByCategory
	processDur   prometheus.Gauge
}

func createStatCategory(at types.AggType, sla, userfaultbroken bool) string {
	ret := string(at)
	if sla {
		ret = "sla_" + ret
	}
	if userfaultbroken {
		ret = "userfault_broken_" + ret
	}
	return ret
}

func newStat(ds datastore.Backend, emulate bool) statByCtype {
	stat := make(statByCtype)
	for _, ct := range dbsupport.DBsupp {
		categoryStat := make(statByCategory)
		for _, userfaultBroken := range []bool{false, true} {
			for _, sla := range []bool{false, true} {
				for _, at := range []types.AggType{types.AggClusters, types.AggShards, types.AggGeoHosts} {
					categoryStat[createStatCategory(at, sla, userfaultBroken)] = make(clustersStatByEnv)
				}
			}
		}
		stat[ct] = statForCtype{
			categoryStat: categoryStat,
			processDur: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: fmt.Sprintf("process_aggregate_health_dur_%s", string(ct)),
			}),
		}
		if !emulate {
			prometheus.MustRegister(stat[ct].processDur)
		}
	}
	return stat
}

func (gw *GeneralWard) updateSLI(ctx context.Context) error {
	if gw.stat == nil {
		return nil
	}
	aiList, err := gw.ds.LoadAggregateInfo(ctx)
	if err != nil {
		return nil
	}
	for _, agg := range aiList {
		stat := gw.ensureStatForEnv(agg.CType, agg.Env, agg.AggType, agg.SLA, agg.UserFaultBroken)
		timeSince := time.Since(agg.Timestamp)
		aggregateTTL := gw.cfg.LeadTimeout * 3
		if timeSince > aggregateTTL {
			ctxlog.Debugf(ctx, gw.logger, "%s and env %s timestamp is out of fresh aggregate TTL %s current diff %s",
				agg.CType, agg.Env, aggregateTTL, timeSince)
			agg = datastore.AggregatedInfo{}
		}
		stat.total.Set(float64(agg.Total))
		stat.alive.Set(float64(agg.Alive))
		stat.dead.Set(float64(agg.Dead))
		stat.degraded.Set(float64(agg.Degraded))
		stat.unknown.Set(float64(agg.Unknown))
		stat.hoststotal.Set(float64(agg.RWInfo.HostsTotal))
		stat.hostsread.Set(float64(agg.RWInfo.HostsRead))
		stat.hostswrite.Set(float64(agg.RWInfo.HostsWrite))
		stat.dbtotal.Set(float64(agg.RWInfo.DBTotal))
		stat.dbread.Set(float64(agg.RWInfo.DBRead))
		stat.dbwrite.Set(float64(agg.RWInfo.DBWrite))
		stat.hostsBroken.Set(float64(agg.RWInfo.HostsBrokenByUser))
		stat.dbBroken.Set(float64(agg.RWInfo.DBBroken))
	}
	return nil
}

func (gw *GeneralWard) ensureStatForEnv(ctype metadb.ClusterType, env string, agg types.AggType, sla, userfaultBroken bool) clusterEnvStat {
	cat := createStatCategory(agg, sla, userfaultBroken)
	statType := gw.stat[ctype].categoryStat[cat]
	env = strings.ReplaceAll(env, "-", "_")
	stat, ok := statType[env]
	sctype := strings.TrimSuffix(string(ctype), "_cluster")

	if !ok {
		s := clusterEnvStat{
			total: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "health", env, "total"}, "_"),
			}),
			alive: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "health", env, "alive"}, "_"),
			}),
			degraded: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "health", env, "degraded"}, "_"),
			}),
			unknown: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "health", env, "unknown"}, "_"),
			}),
			dead: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "health", env, "dead"}, "_"),
			}),
			hoststotal: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "hosts", "total"}, "_"),
			}),
			hostsread: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "hosts", "read"}, "_"),
			}),
			hostswrite: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "hosts", "write"}, "_"),
			}),
			dbtotal: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "dbtotal"}, "_"),
			}),
			dbread: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "dbread"}, "_"),
			}),
			dbwrite: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "dbwrite"}, "_"),
			}),
			hostsBroken: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "hosts", "broken"}, "_"),
			}),
			dbBroken: prometheus.NewGauge(prometheus.GaugeOpts{
				Name: strings.Join([]string{cat, sctype, "rw", env, "dbbroken"}, "_"),
			}),
		}
		prometheus.MustRegister(s.total, s.alive, s.degraded, s.unknown, s.dead,
			s.hoststotal, s.hostsread, s.hostswrite,
			s.dbtotal, s.dbread, s.dbwrite, s.hostsBroken, s.dbBroken)
		statType[env] = s
		return s
	}
	return stat
}
