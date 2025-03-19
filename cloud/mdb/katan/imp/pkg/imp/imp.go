package imp

import (
	"context"
	"time"

	"github.com/google/go-cmp/cmp"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	TagsVersion = 2
)

type Imp struct {
	mdb metadb.MetaDB
	kdb katandb.KatanDB
	lg  log.Logger
}

// New create new Imp
func New(mdb metadb.MetaDB, kdb katandb.KatanDB, lg log.Logger) *Imp {
	return &Imp{
		mdb: mdb,
		kdb: kdb,
		lg:  lg,
	}
}

func (imp *Imp) IsReady(ctx context.Context) error {
	if err := imp.mdb.IsReady(ctx); err != nil {
		return xerrors.Errorf("metadb is not ready: %w", err)
	}

	if err := imp.kdb.IsReady(ctx); err != nil {
		return xerrors.Errorf("katandb is not ready: %w", err)
	}

	return nil
}

func (imp *Imp) getKatanMetaCluster(ctx context.Context) (map[string]tags.ClusterTags, error) {
	clusters, err := imp.kdb.ClustersByTagsQuery(ctx, tags.QueryTagsBySource(tags.MetaDBSource))
	if err != nil {
		return nil, xerrors.Errorf("fail to get hosts by query: %w", err)
	}
	imp.lg.Infof("got %d clusters from katandb", len(clusters))
	clusterMap, err := UnmarshalClusters(clusters)
	if err != nil {
		return nil, err
	}
	return clusterMap, nil
}

// FromMeta import clusters from metadb
func (imp *Imp) FromMeta(ctx context.Context) error {
	metaRevs, err := imp.mdb.ClustersRevs(ctx)
	if err != nil {
		return xerrors.Errorf("fail to get hosts revs: %w", err)
	}
	imp.lg.Infof("got %d hosts from meta", len(metaRevs))

	examinedClusters := make(map[string]bool)
	katanClustersMap, err := imp.getKatanMetaCluster(ctx)
	if err != nil {
		return err
	}

	for _, meta := range metaRevs {
		examinedClusters[meta.ClusterID] = true
		katanCluster, ok := katanClustersMap[meta.ClusterID]
		if !ok {
			imp.lg.Infof("we have new cluster %+v", meta)
			err = imp.importNewCluster(ctx, meta)
			if err != nil {
				return xerrors.Errorf("fail while try import new cluster: %w", err)
			}
			continue
		}

		if katanCluster.Version < TagsVersion {
			imp.lg.Infof("cluster %+v has old tags version: %d (current version is %d)", meta, katanCluster.Version, TagsVersion)
		} else {
			if katanCluster.Meta.Rev == meta.Rev {
				imp.lg.Debugf("cluster %+v at latest state", meta)
				continue
			}
			if katanCluster.Meta.Rev > meta.Rev {
				imp.lg.Warnf("cluster %+v in katandb has revision greater then in meta %+v. Read from stale replica?", katanCluster, meta)
				continue
			}
		}

		err = imp.updateCluster(ctx, meta, katanCluster)
		if err != nil {
			return xerrors.Errorf("fail while try update cluster hosts %+v: %w", meta, err)
		}
	}

	for clusterID := range katanClustersMap {
		if !examinedClusters[clusterID] {
			imp.lg.Infof("delete cluster %q, cause it not exist in metadb", clusterID)
			err = imp.deleteCluster(ctx, clusterID)
			if err != nil {
				return xerrors.Errorf("fail to delete cluster: %q: %w", clusterID, err)
			}
		}
	}

	return nil
}

func (imp *Imp) deleteCluster(ctx context.Context, clusterID string) error {
	return imp.kdb.DeleteCluster(ctx, clusterID)
}

func (imp *Imp) importNewCluster(ctx context.Context, meta metadb.ClusterRev) error {
	cluster, err := imp.mdb.ClusterAtRev(ctx, meta.ClusterID, meta.Rev)
	if err != nil {
		return xerrors.Errorf("fail while try get ClusterAtRev: %w", err)
	}

	clusterHosts, err := imp.mdb.ClusterHostsAtRev(ctx, meta.ClusterID, meta.Rev)
	if err != nil {
		return xerrors.Errorf("unable to get cluster hosts: %w", err)
	}

	clusterTags, err := ClusterToTags(cluster, meta).Marshal()
	if err != nil {
		return err
	}

	err = imp.kdb.AddCluster(ctx, katandb.Cluster{
		ID:   meta.ClusterID,
		Tags: clusterTags,
	})
	if err != nil {
		return xerrors.Errorf("fail to add cluster %w", err)
	}

	for _, h := range clusterHosts {
		err = imp.addHost(ctx, meta.ClusterID, h.FQDN, HostToTags(h), time.Now())
		if err != nil {
			return err
		}
	}
	return nil
}

func (imp *Imp) getClusterHosts(ctx context.Context, clusterID string) (map[string]tags.HostTags, error) {
	hosts, err := imp.kdb.ClusterHosts(ctx, clusterID)
	if err != nil {
		return nil, err
	}
	return UnmarshalHosts(hosts)
}

func (imp *Imp) updateCluster(ctx context.Context, meta metadb.ClusterRev, clusterTags tags.ClusterTags) error {
	cluster, err := imp.mdb.ClusterAtRev(ctx, meta.ClusterID, meta.Rev)
	if err != nil {
		return xerrors.Errorf("fail while try get ClusterAtRev: %w", err)
	}
	if !cluster.Visible {
		imp.lg.Infof("cluster %+v is not visible. Delete it", meta)
		return imp.deleteCluster(ctx, meta.ClusterID)
	}

	if newTags := ClusterToTags(cluster, meta); !cmp.Equal(newTags, clusterTags) {
		tagsStr, err := newTags.Marshal()
		if err != nil {
			return err
		}
		err = imp.kdb.UpdateClusterTags(ctx, meta.ClusterID, tagsStr)
		if err != nil {
			return xerrors.Errorf("fail to update cluster tags: %w", err)
		}
	}

	clusterHosts, err := imp.mdb.ClusterHostsAtRev(ctx, meta.ClusterID, meta.Rev)
	if err != nil {
		return xerrors.Errorf("unable to get cluster hosts: %w", err)
	}

	katanHosts, err := imp.getClusterHosts(ctx, meta.ClusterID)
	if err != nil {
		return err
	}

	importedFQDNs := make(map[string]bool, len(clusterHosts))

	// add, update host
	for _, h := range clusterHosts {
		importedFQDNs[h.FQDN] = true

		newTags := HostToTags(h)
		existedTags, ok := katanHosts[h.FQDN]
		if ok {
			if cmp.Equal(existedTags, newTags) {
				imp.lg.Debugf("host %q already imported and has same rev: %d", h.FQDN, meta.Rev)
				continue
			}
			imp.lg.Debugf("host %q should be updated", h.FQDN)
			err = imp.updateHostTags(ctx, h.FQDN, newTags)
			if err != nil {
				return err
			}
			continue
		}
		imp.lg.Debugf("host %q is new. Add it", h.FQDN)
		err = imp.addHost(
			ctx,
			meta.ClusterID,
			h.FQDN,
			newTags,
			h.CreatedAt,
		)
		if err != nil {
			return err
		}
	}

	var deletedFQDNs []string
	for fqdn := range katanHosts {
		if !importedFQDNs[fqdn] {
			deletedFQDNs = append(deletedFQDNs, fqdn)
		}
	}
	if len(deletedFQDNs) > 0 {
		imp.lg.Infof("delete %+v hosts", deletedFQDNs)
		return imp.kdb.DeleteHosts(ctx, deletedFQDNs)
	}

	return nil
}

func (imp *Imp) addHost(ctx context.Context, clusterID, fqdn string, hostTags tags.HostTags, updateAt time.Time) error {
	strTags, err := hostTags.Marshal()
	if err != nil {
		return xerrors.Errorf("add host %q failed on tags marshal: %w", fqdn, err)
	}
	return imp.kdb.AddHost(
		ctx,
		katandb.Host{
			ClusterID: clusterID,
			FQDN:      fqdn,
			Tags:      strTags,
		})
}

func (imp *Imp) updateHostTags(ctx context.Context, fqdn string, hostTags tags.HostTags) error {
	strTags, err := hostTags.Marshal()
	if err != nil {
		return xerrors.Errorf("update %q tags failed on tags marshal: %w", fqdn, err)
	}
	return imp.kdb.UpdateHostTags(
		ctx,
		fqdn,
		strTags,
	)
}
