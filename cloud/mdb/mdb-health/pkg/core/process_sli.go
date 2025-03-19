package core

import (
	"context"
	"fmt"
	"time"

	"github.com/opentracing/opentracing-go"
	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (gw *GeneralWard) getProcessSLIFunc(ct metadb.ClusterType) func(ctx context.Context, now time.Time) {
	return func(ctx context.Context, now time.Time) {
		gw.processSLI(ctx, ct, now)
	}
}

func (gw *GeneralWard) processSLI(ctx context.Context, ct metadb.ClusterType, now time.Time) {
	if !gw.leaderElector.IsLeader(ctx) {
		gw.stat[ct].processDur.Set(0)
		return
	}

	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		"Process SLI",
		opentracing.Tag{Key: "cluster.type", Value: ct},
	)
	defer span.Finish()

	aggEnvs := make(map[string]*datastore.AggregatedInfo)
	ensureAggEnvs := func(env string, aggType types.AggType, sla, userfaultBroken bool) *datastore.AggregatedInfo {
		aggField := fmt.Sprintf("%s_%s", createStatCategory(aggType, sla, userfaultBroken), env)
		agg, ok := aggEnvs[aggField]
		if !ok {
			agg = &datastore.AggregatedInfo{
				Timestamp:       now,
				CType:           ct,
				AggType:         aggType,
				Env:             env,
				SLA:             sla,
				UserFaultBroken: userfaultBroken,
			}
			aggEnvs[aggField] = agg
		}
		return agg
	}
	uaLogger := unhealthy.NewLogger(gw.logger, ct)
	ua := unhealthy.NewAggregator(uaLogger, ct)
	updateRWStats := func(agg *datastore.AggregatedInfo, info *types.DBRWInfo) {
		agg.RWInfo.HostsTotal += info.HostsTotal
		agg.RWInfo.HostsRead += info.HostsRead
		agg.RWInfo.HostsWrite += info.HostsWrite
		agg.RWInfo.DBTotal += info.DBTotal
		agg.RWInfo.DBRead += info.DBRead
		agg.RWInfo.DBWrite += info.DBWrite
		agg.RWInfo.HostsBrokenByUser += info.HostsBrokenByUser
		agg.RWInfo.DBBroken += info.DBBroken
	}
	start := prometheus.NewTimer(prometheus.ObserverFunc(gw.stat[ct].processDur.Set))
	var loadFewTryNumber = 0
	for cursor := ""; cursor != datastore.EndCursor; {
		fewClusterInfo, err := gw.ds.LoadFewClustersHealth(ctx, ct, cursor)
		if err != nil {
			ctxlog.Errorf(ctx, gw.logger, "failed to process LoadFewClustersHealth for cluster type %s: %s", ct, err)
			loadFewTryNumber++
			if loadFewTryNumber == loadFewClustersMaxTries {
				ctxlog.Errorf(ctx, gw.logger, "LoadFewClustersHealth fails too many times: %d. Give up", loadFewClustersMaxTries)
				ctxlog.Errorf(ctx, gw.logger, "LoadFewClustersHealth fails too many times: %d. Give up", loadFewClustersMaxTries)
				sentry.GlobalClient().CaptureError(
					ctx,
					xerrors.Errorf("LoadFewClustersHealth fails (%d retries didn't help): %w", loadFewClustersMaxTries, err),
					map[string]string{
						"cluster_type": string(ct),
						"method":       "processSLI",
					},
				)
				return
			}
			continue
		}
		loadFewTryNumber = 0
		cursor = fewClusterInfo.NextCursor
		err = gw.ds.SaveClustersHealth(ctx, fewClusterInfo.Clusters, gw.cfg.ClusterHealthTimeout)
		if err != nil {
			ctxlog.Errorf(ctx, gw.logger, "failed to SaveClustersHealth for cluster type %s for %d clusters: %s", ct, len(fewClusterInfo.Clusters), err)
		}
		for _, h := range fewClusterInfo.Clusters {
			if h.Nonaggregatable {
				continue
			}
			agg := ensureAggEnvs(h.Env, types.AggClusters, h.SLA, h.UserFaultBroken)
			switch h.Status {
			case types.ClusterStatusAlive:
				agg.Alive++
			case types.ClusterStatusDegraded:
				agg.Degraded++
			case types.ClusterStatusDead:
				agg.Dead++
			case types.ClusterStatusUnknown:
				agg.Unknown++
			}
			agg.Total++
			info, ok := fewClusterInfo.ClusterInfo[h.Cid]
			if ok && info.HostsTotal > 0 {
				updateRWStats(agg, &info)
				ua.AddCluster(h.SLA, h.Status, h.Env, h.Cid, &info, fewClusterInfo.GeoInfo[h.Cid])
			} else {
				ua.AddCluster(h.SLA, h.Status, h.Env, h.Cid, nil, nil)
			}
		}
		for sid, info := range fewClusterInfo.SLAShards {
			if info.HostsTotal == 0 {
				continue
			}
			env, ok := fewClusterInfo.ShardEnv[sid]
			if !ok {
				continue
			}
			agg := ensureAggEnvs(env, types.AggShards, true, info.DBBroken > 0)
			updateRWStats(agg, &info)
			ua.AddShard(true, env, sid, &info)
		}
		for sid, info := range fewClusterInfo.NoSLAShards {
			if info.HostsTotal == 0 {
				continue
			}
			env, ok := fewClusterInfo.ShardEnv[sid]
			if !ok {
				continue
			}
			agg := ensureAggEnvs(env, types.AggShards, false, info.DBBroken > 0)
			updateRWStats(agg, &info)
			ua.AddShard(false, env, sid, &info)
		}
		for _, info := range fewClusterInfo.HostInfo {
			// use geo as env, if required split to env may use both
			agg := ensureAggEnvs(info.Geo, types.AggGeoHosts, info.SLA, info.UserFaultBroken)
			switch info.Status {
			case types.HostStatusAlive:
				agg.Alive++
			case types.HostStatusDegraded:
				agg.Degraded++
			case types.HostStatusDead:
				agg.Dead++
			case types.HostStatusUnknown:
				agg.Unknown++
			}
			agg.Total++
		}
	}

	if err := gw.ds.SetUnhealthyAggregatedInfo(ctx, ct, ua.Info(), gw.cfg.ClusterHealthTimeout); err != nil {
		ctxlog.Errorf(ctx, gw.logger, "failed to SetUnhealthyAggregatedInfo for cluster type %s: %s", ct, err)
	}
	ua.Log(ctx)
	aggregateTime := start.ObserveDuration()
	msg := fmt.Sprintf("process SLI metrics for cluster type %s take %s", ct, aggregateTime)
	if aggregateTime > gw.cfg.LeadTimeout/2 {
		ctxlog.Warn(ctx, gw.logger, msg)
	} else if aggregateTime > gw.cfg.LeadTimeout/8 {
		ctxlog.Info(ctx, gw.logger, msg)
	} else {
		ctxlog.Debug(ctx, gw.logger, msg)
	}
	for _, agg := range aggEnvs {
		err := gw.ds.SetAggregateInfo(ctx, *agg, gw.cfg.ClusterHealthTimeout)
		if err != nil {
			ctxlog.Errorf(ctx, gw.logger, "failed to SetAggregateInfo for cluster type %s and env %s clusters: %s", agg.CType, agg.Env, err)
		}
	}
}
