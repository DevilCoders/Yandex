package redis

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbsupport"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (b *backend) RetouchTopology(ctx context.Context, clusters []metadb.ClusterRev, ttl time.Duration) ([]metadb.ClusterRev, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "RetouchTopology")
	defer span.Finish()

	topRev := make([]topologyRev, len(clusters))

	pl := b.client.Pipeline()
	defer func() { _ = pl.Close() }()
	cids := make([]string, len(clusters))
	for i, c := range clusters {
		topRev[i].revReq = pl.HGet(ctx, marshalTopologyKey(c.ClusterID), "rev")
		topRev[i].ttlReq = pl.TTL(ctx, marshalTopologyKey(c.ClusterID))
		cids[i] = c.ClusterID
	}
	setInvisibleReq := pl.Eval(ctx, luaScriptSetInvisible, cids)
	_, err := pl.Exec(ctx)
	setInvisible, ierr := setInvisibleReq.Result()
	if ierr != nil {
		ctxlog.Warnf(ctx, b.logger, "set invisible error: %s", ierr)
	}
	setInvisibleCount, ok := setInvisible.(int64)
	if !ok {
		ctxlog.Warnf(ctx, b.logger, "return of luaScriptSetInvisible unexpected value: %v", setInvisible)
	}
	if setInvisibleCount != 0 {
		ctxlog.Infof(ctx, b.logger, "set %d invisible clusters", setInvisibleCount)
	}
	if err != nil {
		// not a error because we can missed topology of cluster
		ctxlog.Debugf(ctx, b.logger, "pipeline get topology error: %s", err)
	}

	cnt := 0
	needUpdateTTL := 0
	for i := range topRev {
		r := &topRev[i]
		c := clusters[i]
		// key may not exist, it's mean obsolete cluster
		ok, err := r.parse()
		if err != nil {
			if !ok {
				ctxlog.Warnf(ctx, b.logger, "failed parse topology, cid %s, expect rev %d, set rev %s: %s", c.ClusterID, c.Rev, r.rawRev, err)
			}
			cnt++
			continue
		}
		if !r.isActual(c.Rev) {
			cnt++
			continue
		}
		if r.needUpdateTTL(ttl / 4) {
			needUpdateTTL++
			pl.Eval(ctx, luaScriptUpdateTTL, []string{c.ClusterID}, int(ttl.Seconds()))
		}
	}

	if needUpdateTTL > 0 {
		ctxlog.Warnf(ctx, b.logger, "going to update %d topology ttls", needUpdateTTL)
		_, err := pl.Exec(ctx)
		if err != nil {
			return nil, semerr.WrapWithInternal(err, redisRequestFailedErrText)
		}
	}

	changedClusters := make([]metadb.ClusterRev, 0, cnt)
	for i := range topRev {
		if topRev[i].isActual(clusters[i].Rev) {
			continue
		}
		changedClusters = append(changedClusters, clusters[i])
	}

	return changedClusters, nil
}

func (b *backend) SetClustersTopology(ctx context.Context, topologies []datastore.ClusterTopology, ttl time.Duration, mdb metadb.MetaDB) error {
	span, ctx := opentracing.StartSpanFromContext(ctx, "SetClustersTopology")
	defer span.Finish()

	// get custom roles
	for _, t := range topologies {
		customRoles, err := mdb.GetClusterCustomRolesAtRev(ctx, t.Cluster.Type, t.CID, t.Rev)
		if err != nil {
			ctxlog.Errorf(ctx, b.logger, "failed to get custom roles for cluster %s for rev %d", t.CID, t.Rev)
			continue
		}
		for i, h := range t.Hosts {
			role, ok := customRoles[h.FQDN]
			if ok {
				// host has custom role
				t.Hosts[i].Roles = append(t.Hosts[i].Roles, string(role))
			}
		}
	}

	pl := b.client.Pipeline()
	defer func() { _ = pl.Close() }()
	for _, t := range topologies {
		tk := marshalTopologyKey(t.CID)
		pl.Del(ctx, tk)
		pl.HSet(ctx, tk, "rev", t.Rev)
		pl.HSet(ctx, tk, "name", t.Cluster.Name)
		pl.HSet(ctx, tk, "type", string(t.Cluster.Type))
		pl.HSet(ctx, tk, "env", t.Cluster.Environment)
		pl.HSet(ctx, tk, "status", t.Cluster.Status)
		pl.HSet(ctx, tk, "visible", t.Cluster.Visible)
		if t.Nonaggregatable {
			pl.HSet(ctx, tk, "nonaggregatable", t.Nonaggregatable)
		}

		extractor, okExtractor := dbsupport.DBspec[t.Cluster.Type]
		if okExtractor {
			slaCluster, slaShards := extractor.GetSLAGuaranties(t)
			pl.HSet(ctx, tk, "sla", slaCluster)
			var slaListShards string
			for s, sla := range slaShards {
				if !sla {
					continue
				}
				if slaListShards == "" {
					slaListShards = s
				} else {
					slaListShards = fmt.Sprintf("%s %s", slaListShards, s)
				}
			}
			pl.HSet(ctx, tk, "slashards", slaListShards)
		}
		roles := make(map[string]string)
		shards := make(map[string]string)
		geos := make(map[string]string)
		notsharded := make([]string, 0)
		fqdns := make([]string, len(t.Hosts))
		for i, h := range t.Hosts {
			fqdns[i] = h.FQDN
			fk := marshalFQDNKey(h.FQDN)
			pl.Del(ctx, fk)
			pl.HSet(ctx, fk, "cid", t.CID)
			if sid, err := h.ShardID.Get(); err == nil {
				pl.HSet(ctx, fk, "sid", sid)
				cur, ok := shards[sid]
				if !ok {
					shards[sid] = h.FQDN
				} else {
					shards[sid] = fmt.Sprintf("%s %s", cur, h.FQDN)
				}
			} else {
				notsharded = append(notsharded, h.FQDN)
			}
			pl.HSet(ctx, fk, "subcid", h.SubClusterID)
			pl.HSet(ctx, fk, "geo", h.Geo)
			{
				cur, ok := geos[h.Geo]
				if !ok {
					geos[h.Geo] = h.FQDN
				} else {
					geos[h.Geo] = fmt.Sprintf("%s %s", cur, h.FQDN)
				}
			}
			pl.HSet(ctx, fk, "roles", strings.Join(h.Roles, " "))
			for _, hr := range h.Roles {
				cur, ok := roles[hr]
				if !ok {
					roles[hr] = h.FQDN
				} else {
					roles[hr] = fmt.Sprintf("%s %s", cur, h.FQDN)
				}
			}
			pl.HSet(ctx, fk, "created", h.CreatedAt.Unix())
			pl.Expire(ctx, fk, ttl)
		}
		for shard, v := range shards {
			pl.HSet(ctx, tk, marshalTopologyField(shardidPrefix, shard), v)
		}
		for k, v := range roles {
			pl.HSet(ctx, tk, marshalTopologyField(rolePrefix, k), v)
		}
		for geo, v := range geos {
			pl.HSet(ctx, tk, marshalTopologyField(geoPrefix, geo), v)
		}
		if len(fqdns) > 0 {
			pl.HSet(ctx, tk, "fqdns", strings.Join(fqdns, " "))
		}
		if len(notsharded) > 0 {
			pl.HSet(ctx, tk, "notsharded", strings.Join(notsharded, " "))
		}
		pl.Expire(ctx, tk, ttl)
	}
	_, err := pl.Exec(ctx)
	if err != nil {
		return semerr.WrapWithInternal(err, datastore.NotAllUpdatedErrText)
	}
	return nil
}
