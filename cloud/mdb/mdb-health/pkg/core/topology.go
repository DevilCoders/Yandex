package core

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (gw *GeneralWard) updateTopology(ctx context.Context, now time.Time) error {
	// TODO: some tests for this logic
	ctxlog.Infof(ctx, gw.logger, "update topology check")
	clustersRevs, err := gw.mdb.ClustersRevs(ctx)
	if err != nil {
		return err
	}

	obsolete, err := gw.ds.RetouchTopology(ctx, clustersRevs, gw.cfg.TopologyTimeout)
	if err != nil {
		return err
	}
	if len(obsolete) == 0 {
		gw.topologyUpd = now
		return nil
	}

	// update cycle for a topology
	// it gets a maximum of half a cycle for collecting topologies and half a cycle for Redis update
	// with a very low probability, that can occur only with a few topology updates
	// we can lose the lead and update after the next leader has finished updating to next revision (race)
	// in this case, the topology will be updated in the next topology check cycle
	now = time.Now()
	requestTopologyTimeout := 3 * gw.cfg.LeadTimeout / 4
	requestTopologyUntil := now.Add(requestTopologyTimeout)
	clusterTopologies := make([]datastore.ClusterTopology, 0, len(obsolete))
	// first part for collecting clusters topology
	for _, ci := range obsolete {
		clusterTopology, err := gw.loadClusterTopology(ctx, ci.ClusterID, ci.Rev)
		if err != nil {
			ctxlog.Warnf(ctx, gw.logger, "failed to load %q cluster topology: %s", ci.ClusterID, err)
			break
		}
		clusterTopologies = append(clusterTopologies, clusterTopology)

		if time.Now().After(requestTopologyUntil) {
			ctxlog.Warnf(ctx, gw.logger, "stopping updating topologies as the timeout (%s) allocated for it has exceed", requestTopologyTimeout)
			break
		}
	}
	if len(clusterTopologies) == 0 {
		return xerrors.Errorf("not load topology of obsolete clusters for update, total %d clusters, obsolete %d", len(clustersRevs), len(obsolete))
	}

	// second part for Redis update
	ctxlog.Infof(ctx, gw.logger, "set new cluster topologies %d of total %d clusters", len(clusterTopologies), len(obsolete))
	err = gw.ds.SetClustersTopology(ctx, clusterTopologies, gw.cfg.TopologyTimeout, gw.mdb)
	if err != nil {
		return err
	}

	if len(clusterTopologies) == len(obsolete) {
		ctxlog.Infof(ctx, gw.logger, "updateTopology finished")
		gw.topologyUpd = now
	} else {
		ctxlog.Infof(ctx, gw.logger, "save partial %d of total %d clusters", len(clusterTopologies), len(obsolete))
	}
	return nil
}

func (gw *GeneralWard) loadClusterTopology(ctx context.Context, cid string, rev int64) (datastore.ClusterTopology, error) {
	cluster, err := gw.mdb.ClusterAtRev(ctx, cid, rev)
	if err != nil {
		return datastore.ClusterTopology{}, xerrors.Errorf("failed to get cluster %s at revision %d: %s", cid, rev, err)
	}

	clusterTopology := datastore.ClusterTopology{
		Cluster: cluster,
		CID:     cid,
		Rev:     rev,
	}

	if cluster.Visible && types.SupportTopology(cluster.Type) {
		clusterTopology.Hosts, err = gw.mdb.ClusterHostsAtRev(ctx, cid, rev)
		if err != nil {
			return datastore.ClusterTopology{}, xerrors.Errorf("failed to get for cluster %s hosts at revision %d: %s", cid, rev, err)
		}
		nonaggregatable, err := gw.mdb.ClusterHealthNonaggregatable(ctx, cid, rev)
		if err != nil {
			return datastore.ClusterTopology{}, xerrors.Errorf(
				"failed to get for cluster %s health nonaggregatable flag at revision %d: %s",
				cid,
				rev,
				err,
			)
		}
		clusterTopology.Nonaggregatable = nonaggregatable
	}
	return clusterTopology, nil
}
