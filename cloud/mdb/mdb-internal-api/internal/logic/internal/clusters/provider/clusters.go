package provider

import (
	"context"
	"reflect"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Clusters) CreateCluster(ctx context.Context, args models.CreateClusterArgs) (clmodels.Cluster, []byte, error) {
	if err := args.Validate(); err != nil {
		return clmodels.Cluster{}, nil, err
	}

	reqid := requestid.MustFromContext(ctx)

	// Generate cluster id if needed
	if args.ClusterID == "" {
		id, err := c.clusterIDGenerator.Generate()
		if err != nil {
			return clmodels.Cluster{}, nil, xerrors.Errorf("cluster id not generated: %w", err)
		}

		args.ClusterID = id
	}

	// Generate cluster crypto key if it's empty
	var privateKey []byte
	if len(args.PublicKey) == 0 {
		privKey, pubKey, err := c.clusterSecretsGenerator.Generate()
		if err != nil {
			return clmodels.Cluster{}, nil, err
		}

		privateKey = privKey
		args.PublicKey = pubKey
	}

	cluster, err := c.metaDB.CreateCluster(ctx, reqid, args)
	if err != nil {
		return clmodels.Cluster{}, nil, wrapClusterAlreadyExists(err, args.Name)
	}

	if len(args.Labels) != 0 {
		if err = c.metaDB.SetLabelsOnCluster(ctx, args.ClusterID, args.Labels, cluster.Revision); err != nil {
			return clmodels.Cluster{}, nil, xerrors.Errorf("cluster labels not set: %w", err)
		}
	}

	return cluster.Cluster, privateKey, nil
}

func (c *Clusters) ClusterByClusterID(ctx context.Context, cid string, typ clmodels.Type, vis models.Visibility) (clusters.Cluster, error) {
	cluster, err := c.clusterByClusterID(ctx, cid, typ, vis)
	if err != nil {
		return clusters.Cluster{}, err
	}

	return clusters.NewClusterModel(cluster.Cluster, cluster.Pillar), nil
}

func (c *Clusters) ClusterExtendedByClusterID(ctx context.Context, cid string, typ clmodels.Type, vis models.Visibility, session sessions.Session) (clmodels.ClusterExtended, error) {
	cluster, err := c.clusterByClusterID(ctx, cid, typ, vis)
	if err != nil {
		return clmodels.ClusterExtended{}, err
	}

	health, _ := c.health.Cluster(ctx, cid)

	monitoring, err := c.cfg.Monitoring.New(typ, cid, session.FolderCoords.FolderExtID, c.cfg.Console.URI)
	if err != nil {
		c.l.Error("cannot retrieve cluster monitoring", log.String("cid", cid), log.Error(err))
		sentry.GlobalClient().CaptureError(ctx, err, nil)
	}

	maintenanceInfo, err := c.MaintenanceInfoByClusterID(ctx, cid)
	if err != nil {
		c.l.Error("cannot retrieve maintenance info", log.String("cid", cid), log.Error(err))
		sentry.GlobalClient().CaptureError(ctx, err, nil)
	}

	clext := clmodels.ClusterExtended{
		Cluster:         cluster.Cluster,
		FolderExtID:     session.FolderCoords.FolderExtID,
		Health:          health,
		Monitoring:      monitoring,
		Pillar:          cluster.Pillar,
		MaintenanceInfo: maintenanceInfo,
	}
	return clext, nil
}

func (c *Clusters) clusterByClusterID(ctx context.Context, cid string, typ clmodels.Type, vis models.Visibility) (metadb.Cluster, error) {
	cluster, err := c.metaDB.ClusterByClusterID(ctx, cid, vis)
	if err != nil {
		return metadb.Cluster{}, wrapWithClusterNotFoundIfSQLErrNotFound(err, cid)
	}

	if cluster.Type != typ {
		return metadb.Cluster{}, wrapWithClusterNotFound(xerrors.Errorf("found invalid cluster type: requested %q but found %q", typ, cluster.Type), cid)
	}

	return cluster, nil
}

func (c *Clusters) ClusterByClusterIDAtRevision(ctx context.Context, cid string, typ clmodels.Type, rev int64) (clusters.Cluster, error) {
	cluster, err := c.clusterByClusterIDAtRevision(ctx, cid, typ, rev)
	if err != nil {
		return clusters.Cluster{}, err
	}

	return clusters.NewClusterModel(cluster.Cluster, cluster.Pillar), nil
}

func (c *Clusters) clusterByClusterIDAtRevision(ctx context.Context, cid string, typ clmodels.Type, rev int64) (metadb.Cluster, error) {
	cluster, err := c.metaDB.ClusterByClusterIDAtRevision(ctx, cid, rev)
	if err != nil {
		return metadb.Cluster{}, wrapWithClusterNotFoundIfSQLErrNotFound(err, cid)
	}

	if cluster.Type != typ {
		return metadb.Cluster{}, wrapWithClusterNotFound(xerrors.Errorf("found invalid cluster type: requested %q but found %q", typ, cluster.Type), cid)
	}

	return cluster, nil
}

func (c *Clusters) ClusterAtTime(ctx context.Context, cid string, time time.Time, typ clmodels.Type) (clusters.Cluster, error) {
	rev, err := c.RevisionByClusterIDAtTime(ctx, cid, time)
	if err != nil {
		return clusters.Cluster{}, err
	}

	return c.ClusterByClusterIDAtRevision(ctx, cid, typ, rev)
}

func (c *Clusters) Clusters(ctx context.Context, args models.ListClusterArgs) ([]clusters.Cluster, error) {
	cls, err := c.metaDB.Clusters(ctx, args)
	if err != nil {
		return nil, err
	}

	res := make([]clusters.Cluster, 0, len(cls))
	for _, cl := range cls {
		res = append(res, clusters.NewClusterModel(cl.Cluster, cl.Pillar))
	}

	return res, err
}

func (c *Clusters) ClustersExtended(ctx context.Context, args models.ListClusterArgs, session sessions.Session) ([]clmodels.ClusterExtended, error) {
	cls, err := c.metaDB.Clusters(ctx, args)
	if err != nil {
		return nil, err
	}

	res := make([]clmodels.ClusterExtended, 0, len(cls))
	for _, cl := range cls {
		health, _ := c.health.Cluster(ctx, cl.ClusterID)
		maintenanceInfo, err := c.MaintenanceInfoByClusterID(ctx, cl.ClusterID)
		if err != nil {
			return nil, err
		}

		res = append(res, clmodels.ClusterExtended{
			Cluster:         cl.Cluster,
			FolderExtID:     session.FolderCoords.FolderExtID,
			Health:          health,
			Pillar:          cl.Pillar,
			MaintenanceInfo: maintenanceInfo,
		})
	}

	return res, err
}

func (c *Clusters) ClusterVersions(ctx context.Context, cid string) ([]console.Version, error) {
	return c.metaDB.GetClusterVersions(ctx, cid)
}

func (c *Clusters) ClusterVersionsAtRevision(ctx context.Context, cid string, rev int64) ([]console.Version, error) {
	return c.metaDB.GetClusterVersionsAtRevision(ctx, cid, rev)
}

func (c *Clusters) ClusterUsesBackupService(ctx context.Context, cid string) (bool, error) {
	return c.metaDB.ClusterUsesBackupService(ctx, cid)
}

func (c *Clusters) UpdateClusterName(ctx context.Context, cluster clusters.Cluster, name string) error {
	err := c.metaDB.UpdateClusterName(ctx, cluster.ClusterID, name, cluster.Revision)
	if err != nil {
		return wrapClusterAlreadyExists(err, name)
	}
	return nil
}

func (c *Clusters) UpdateClusterFolder(ctx context.Context, cluster clusters.Cluster, folderExtID string) error {
	folderCoords, err := c.metaDB.FolderCoordsByFolderExtID(ctx, folderExtID)
	if err != nil {
		return err
	}
	err = c.metaDB.UpdateClusterFolder(ctx, cluster.ClusterID, folderCoords.FolderID, cluster.Revision)
	if err != nil {
		return err
	}
	return nil
}

func (c *Clusters) ModifyClusterMetadata(ctx context.Context, cluster clusters.Cluster, name optional.String, labels modelsoptional.Labels) (bool, error) {
	hasChanges := false

	if name.Valid && name.String != cluster.Name {
		if err := c.UpdateClusterName(ctx, cluster, name.String); err != nil {
			return hasChanges, err
		}
		hasChanges = true
	}

	if labels.Valid && !reflect.DeepEqual(labels.Labels, cluster.Labels) {
		if err := c.metaDB.SetLabelsOnCluster(ctx, cluster.ClusterID, labels.Labels, cluster.Revision); err != nil {
			return hasChanges, err
		}
		hasChanges = true
	}

	return hasChanges, nil
}

func (c *Clusters) ModifyClusterMetadataParameters(ctx context.Context, cluster clusters.Cluster,
	description optional.String, labels modelsoptional.Labels, deletionProtection optional.Bool,
	maintenanceWindow modelsoptional.MaintenanceWindow) (bool, error) {
	hasChanges := false

	if description.Valid && description.String != cluster.Description {
		err := c.metaDB.UpdateClusterDescription(ctx, cluster.ClusterID, description.String, cluster.Revision)
		if err != nil {
			return hasChanges, err
		}
		hasChanges = true
	}

	if deletionProtection.Valid && deletionProtection.Bool != cluster.DeletionProtection {
		err := c.metaDB.UpdateDeletionProtection(ctx, cluster.ClusterID, deletionProtection.Bool, cluster.Revision)
		if err != nil {
			return hasChanges, err
		}
		hasChanges = true
	}

	if maintenanceWindow.Valid {
		changes, err := c.updateClusterMaintenanceWindow(ctx, cluster, maintenanceWindow.MaintenanceWindow)
		if err != nil {
			return hasChanges, err
		}
		hasChanges = hasChanges || changes
	}

	return hasChanges, nil
}

func (c *Clusters) updateClusterMaintenanceWindow(ctx context.Context, cluster clusters.Cluster, maintenanceWindow clmodels.MaintenanceWindow) (bool, error) {
	err := maintenanceWindow.Validate()
	if err != nil {
		return false, err
	}

	currentMaintenanceInfo, err := c.MaintenanceInfoByClusterID(ctx, cluster.ClusterID)
	if err != nil {
		return false, err
	}

	if !currentMaintenanceInfo.Window.Equal(maintenanceWindow) {
		err = c.metaDB.SetMaintenanceWindowSettings(ctx, cluster.ClusterID, cluster.Revision, maintenanceWindow)
		if err != nil {
			return false, err
		}

		return true, nil
	}

	return false, nil
}

func wrapWithClusterNotFoundIfSQLErrNotFound(err error, cid string) error {
	if xerrors.Is(err, sqlerrors.ErrNotFound) {
		return wrapWithClusterNotFound(err, cid)
	}

	return err
}

func wrapWithClusterNotFound(err error, cid string) error {
	return semerr.WrapWithNotFoundf(err, "cluster %q does not exist", cid)
}

func wrapClusterAlreadyExists(err error, name string) error {
	if xerrors.Is(err, sqlerrors.ErrAlreadyExists) {
		err = semerr.WrapWithAlreadyExistsf(err, "cluster %q already exists", name)
	}

	return err
}

func (c *Clusters) MaintenanceInfoByClusterID(ctx context.Context, cid string) (clmodels.MaintenanceInfo, error) {
	info, err := c.metaDB.MaintenanceInfoByClusterID(ctx, cid)
	if err != nil {
		return clmodels.MaintenanceInfo{}, err
	}

	info.Operation.NearestMaintenanceWindow = CalculateNearestMaintenanceWindow(info.Operation.DelayedUntil, info.Window)
	info.Operation.LatestMaintenanceTime = CalculateLatestMaintenanceTime(info.Operation.CreatedAt)

	return info, nil
}

func (c *Clusters) RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clmodels.RescheduleType, maintenanceTime optional.Time) (time.Time, error) {
	maintenanceInfo, err := c.MaintenanceInfoByClusterID(ctx, cid)
	if err != nil {
		return time.Time{}, err
	}

	newMaintenanceTime, err := GetNewMaintenanceTime(maintenanceInfo, rescheduleType, maintenanceTime)
	if err != nil {
		return time.Time{}, err
	}

	err = c.metaDB.RescheduleMaintenanceTask(ctx, cid, maintenanceInfo.Operation.ConfigID, newMaintenanceTime)
	if err != nil {
		return time.Time{}, err
	}

	return newMaintenanceTime, nil
}
