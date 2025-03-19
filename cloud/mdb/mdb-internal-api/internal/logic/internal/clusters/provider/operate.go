package provider

// TODO: move to separate type/interface (Operator)?

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Operator struct {
	l log.Logger

	resourceModel environment.ResourceModel
	sessions      sessions.Sessions
	metadb        metadb.Backend

	creator  clusters.Creator
	restorer clusters.Restorer
	reader   clusters.Reader
	modifier clusters.Modifier

	logicConfig *logic.Config
}

func (o *Operator) lockCluster(ctx context.Context, cid string, typ clustermodels.Type) (clusters.Cluster, error) {
	reqid := requestid.MustFromContext(ctx)
	cluster, err := o.metadb.LockCluster(ctx, cid, reqid)
	if err != nil {
		return clusters.Cluster{}, wrapWithClusterNotFoundIfSQLErrNotFound(xerrors.Errorf("failed to obtain cluster lock: %w", err), cid)
	}

	if cluster.Type != typ {
		return clusters.Cluster{}, wrapWithClusterNotFound(xerrors.Errorf("locked invalid cluster type: requested %q but locked %q", typ, cluster.Type), cid)
	}

	return clusters.NewClusterModel(cluster.Cluster, cluster.Pillar), nil
}

func (o *Operator) Create(ctx context.Context, folderExtID string, do clusters.CreateFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllCreate}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.create(ctx, folderExtID,
		opOpts.Permission,
		func(ctx context.Context, session sessions.Session) (clustermodels.Cluster, operations.Operation, error) {
			return do(ctx, session, o.creator)
		})
}

func (o *Operator) FakeCreate(ctx context.Context, folderExtID string, do clusters.FakeCreateFunc, opts ...clusters.OperatorOption) error {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllCreate}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.fakeCreate(ctx, folderExtID,
		opOpts.Permission,
		func(ctx context.Context, session sessions.Session) error {
			return do(ctx, session, o.creator)
		})
}

type createFunc func(ctx context.Context, session sessions.Session) (clustermodels.Cluster, operations.Operation, error)

func (o *Operator) create(ctx context.Context, folderExtID string, permission models.Permission, do createFunc) (operations.Operation, error) {
	return o.createImpl(
		ctx,
		sessions.ResolveByFolder(folderExtID, permission),
		do,
	)
}

type fakeCreateFunc func(ctx context.Context, session sessions.Session) error

func (o *Operator) fakeCreate(ctx context.Context, folderExtID string, permission models.Permission, do fakeCreateFunc) error {
	return o.fakeCreateImpl(
		ctx,
		sessions.ResolveByFolder(folderExtID, permission),
		do,
	)
}

func (o *Operator) createImpl(ctx context.Context, sessionResolver sessions.SessionResolver, do createFunc) (operations.Operation, error) {
	return o.operate(
		ctx,
		sessionResolver,
		func(ctx context.Context, session sessions.Session) (operations.Operation, error) {
			// Validation is handled inside user code so we must add cluster
			// TODO: move validation to common code?
			// TODO: remove Clusters from resources that can be manipulated by user
			session.Quota.RequestChange(quota.Resources{Clusters: 1})

			cl, op, err := do(ctx, session)
			if err != nil {
				return operations.Operation{}, err
			}

			// Update used quota if needed
			if !session.Quota.Changed() {
				panic("tried to create cluster but did not request quota change")
			}

			r := session.Quota.Diff()
			if r.Clusters != 1 {
				msg := fmt.Sprintf("invalid quota diff for clusters count: %d when must be 1", r.Clusters)
				ctxlog.Error(ctx, o.l, msg)
				sentry.GlobalClient().CaptureError(ctx, xerrors.New(msg), nil)
				r.Clusters = 1
			}

			reqid := requestid.MustFromContext(ctx)
			if _, err = o.metadb.UpdateCloudUsedQuota(ctx, session.FolderCoords.CloudID, metadb.Resources(r), reqid); err != nil {
				return operations.Operation{}, err
			}

			if err = o.metadb.CompleteClusterChange(ctx, cl.ClusterID, cl.Revision); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (o *Operator) fakeCreateImpl(ctx context.Context, sessionResolver sessions.SessionResolver, do fakeCreateFunc) error {
	err := o.fakeOperate(
		ctx,
		sessionResolver,
		func(ctx context.Context, session sessions.Session) error {
			return do(ctx, session)
		},
	)
	return err
}

func (o *Operator) Restore(ctx context.Context, folderExtID string, do clusters.RestoreFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllCreate}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.create(ctx, folderExtID,
		opOpts.Permission,
		func(ctx context.Context, session sessions.Session) (clustermodels.Cluster, operations.Operation, error) {
			return do(ctx, session, o.reader, o.restorer)
		})
}

func (o *Operator) RestoreWithoutBackupService(ctx context.Context, folderExtID optional.String, cid string, do clusters.RestoreFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	if folderExtID.Valid {
		return o.Restore(ctx, folderExtID.Must(), do, opts...)
	}

	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllCreate}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.createImpl(
		ctx,
		sessions.ResolveByDeletedCluster(cid, opOpts.Permission),
		func(ctx context.Context, session sessions.Session) (clustermodels.Cluster, operations.Operation, error) {
			return do(ctx, session, o.reader, o.restorer)
		},
	)
}

func (o *Operator) ReadOnFolder(ctx context.Context, folderExtID string, do clusters.ReadOnFolderFunc, opts ...clusters.OperatorOption) error {
	if folderExtID == "" {
		return semerr.InvalidInputf("%s id must be specified", o.resourceModel.Folder())
	}
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllRead}
	for _, opt := range opts {
		opt(opOpts)
	}

	ctx, session, err := o.sessions.Begin(ctx, sessions.ResolveByFolder(folderExtID, opOpts.Permission))
	if err != nil {
		return err
	}
	defer o.sessions.Rollback(ctx)

	return do(ctx, session, o.reader)
}

func (o *Operator) Delete(ctx context.Context, cid string, typ clustermodels.Type, do clusters.DeleteFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllDelete}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.modify(ctx, cid, typ, sessions.ResolveByCluster(cid, opOpts.Permission), true,
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			// Load hosts before executing user code because they will be gone after it completes
			hosts, _, _, err := o.metadb.ListHosts(ctx, cluster.ClusterID, 0, optional.Int64{})
			if err != nil {
				return operations.Operation{}, err
			}

			if cluster.DeletionProtection {
				// check 'forceDelete' permission:
				err = session.Authorize(
					ctx,
					opOpts.Permission.WithForce(),
					accessservice.ResourceFolder(session.FolderCoords.FolderExtID),
					accessservice.ResourceCloud(session.FolderCoords.CloudExtID),
				)
				if err != nil {
					return operations.Operation{}, semerr.FailedPrecondition("The operation was rejected because cluster has 'deletion_protection' = ON")
				}
			}

			op, err := do(ctx, session, cluster, o.reader)
			if err != nil {
				return operations.Operation{}, err
			}

			// Deduce quota
			dts, err := o.metadb.DiskTypes(ctx)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("disk types not loaded: %w", err)
			}

			// Count used resource presets, also count actual used space
			rpresets := make(map[string]int)
			freeQuota := quota.Resources{Clusters: 1}
			for _, host := range hosts {
				freeQuota, err = dts.Apply(freeQuota, host.SpaceLimit, host.DiskTypeExtID)
				if err != nil {
					return operations.Operation{}, err
				}

				rpresets[host.ResourcePresetExtID] = rpresets[host.ResourcePresetExtID] + 1
			}

			// Count actual used resources
			for rPresetExtID, count := range rpresets {
				flavor, err := o.metadb.ResourcePresetByExtID(ctx, rPresetExtID)
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("resource preset %q not loaded: %w", rPresetExtID, err)
				}

				freeQuota = freeQuota.Add(flavor.Resources().Mul(int64(count)))
			}

			// Invert
			freeQuota = quota.Resources{}.Sub(freeQuota)
			// Apply deduction
			session.Quota.RequestChange(freeQuota)
			return op, nil
		},
	)
}

func (o *Operator) CreateOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do clusters.CreateOnClusterFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllCreate}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.modify(ctx, cid, typ, sessions.ResolveByClusterDependency(cid, opOpts.Permission), true,
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			return do(ctx, session, o.reader, o.modifier, cluster)
		})
}

func (o *Operator) modifyOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do clusters.ModifyOnClusterFunc, changeRev bool, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllModify}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.modify(ctx, cid, typ, sessions.ResolveByClusterDependency(cid, opOpts.Permission), changeRev,
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			return do(ctx, session, o.reader, o.modifier, cluster)
		})
}

func (o *Operator) ModifyOnClusterWithoutRevChanging(ctx context.Context, cid string, typ clustermodels.Type, do clusters.ModifyOnClusterFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	return o.modifyOnCluster(ctx, cid, typ, do, false, opts...)
}

func (o *Operator) ModifyOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do clusters.ModifyOnClusterFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	return o.modifyOnCluster(ctx, cid, typ, do, true, opts...)
}

func (o *Operator) ModifyOnNotRunningCluster(ctx context.Context, cid string, typ clustermodels.Type, do clusters.ModifyOnNotRunningClusterFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllModify}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.modify(ctx, cid, typ, sessions.ResolveByClusterDependency(cid, opOpts.Permission), true,
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			if cluster.Status == clustermodels.StatusRunning {
				return operations.Operation{}, semerr.FailedPrecondition("operation is not allowed in current cluster status")
			}

			return do(ctx, session, o.reader, o.modifier, cluster)
		})
}

func (o *Operator) ModifyOnNotStoppedCluster(ctx context.Context, cid string, typ clustermodels.Type, do clusters.ModifyOnNotStoppedClusterFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllModify}
	if cid == "" {
		return operations.Operation{}, semerr.InvalidInput("cluster id is not set")
	}

	for _, opt := range opts {
		opt(opOpts)
	}

	return o.modify(ctx, cid, typ, sessions.ResolveByClusterDependency(cid, opOpts.Permission), true,
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			if cluster.Status == clustermodels.StatusStopped {
				return operations.Operation{}, semerr.FailedPrecondition("operation is not allowed in current cluster status")
			}

			return do(ctx, session, o.reader, o.modifier, cluster)
		})
}

type moveFunc func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error)

func (o *Operator) move(ctx context.Context, resolver sessions.SessionResolver, cid, dstFolderExtID string, typ clustermodels.Type, do moveFunc) (operations.Operation, error) {
	return o.operate(
		ctx,
		resolver,
		func(ctx context.Context, session sessions.Session) (operations.Operation, error) {
			if dstFolderExtID == session.FolderCoords.FolderExtID {
				return operations.Operation{}, semerr.FailedPreconditionf("cluster %q is already in folder %q", cid, dstFolderExtID)
			}

			dstFolderCoords := *session.DstFolderCoords

			if dstFolderCoords.CloudExtID != session.FolderCoords.CloudExtID {
				if !o.logicConfig.Flags.AllowMoveBetweenClouds {
					return operations.Operation{}, semerr.FailedPrecondition("moving cluster between folders in different clouds is unavailable")
				}

				srcResources, err := o.metadb.GetClusterQuotaUsage(ctx, cid)
				if err != nil {
					return operations.Operation{}, err
				}

				session.DstQuota.RequestChange(srcResources.ToQuotaResources())
				err = session.DstQuota.Validate()
				if err != nil {
					return operations.Operation{}, err
				}

				_, err = o.metadb.UpdateCloudUsedQuota(ctx, dstFolderCoords.CloudID, srcResources, requestid.FromContextOrNew(ctx))
				if err != nil {
					return operations.Operation{}, err
				}

				var resourcesDiff = quota.Resources{}.Sub(srcResources.ToQuotaResources())
				_, err = o.metadb.UpdateCloudUsedQuota(ctx, session.FolderCoords.CloudID, (&metadb.Resources{}).FromQuotaResources(resourcesDiff), requestid.FromContextOrNew(ctx))
				if err != nil {
					return operations.Operation{}, err
				}
			}

			return o.modifyCluster(ctx, cid, typ, session, true,
				func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
					return do(ctx, session, cluster)
				},
			)
		},
	)
}

func (o *Operator) MoveCluster(ctx context.Context, cid, dstFolderExtID string, typ clustermodels.Type, do clusters.MoveClusterFunc) (op operations.Operation, err error) {
	if cid == "" {
		return operations.Operation{}, semerr.InvalidInput("cluster id must be specified")
	}
	if dstFolderExtID == "" {
		return operations.Operation{}, semerr.InvalidInput("target folder id must be specified")
	}

	return o.move(
		ctx,
		sessions.ResolveByClusterAndDstFolder(cid, dstFolderExtID),
		cid,
		dstFolderExtID,
		typ,
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			if cluster.Status == clustermodels.StatusStopped {
				return operations.Operation{}, semerr.FailedPrecondition("operation is not allowed in current cluster status")
			}

			return do(ctx, session, o.modifier, cluster)
		})
}

func (o *Operator) ModifyOnClusterByBackupID(ctx context.Context, backupID string, typ clustermodels.Type, do clusters.ModifyOnClusterFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllModify}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.modifyByBackupID(ctx, backupID, typ, sessions.ResolveByBackupID(backupID, opOpts.Permission),
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			return do(ctx, session, o.reader, o.modifier, cluster)
		})
}

func (o *Operator) DeleteOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do clusters.DeleteOnClusterFunc, opts ...clusters.OperatorOption) (operations.Operation, error) {
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllDelete}
	for _, opt := range opts {
		opt(opOpts)
	}

	return o.modify(ctx, cid, typ, sessions.ResolveByClusterDependency(cid, opOpts.Permission), true,
		func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error) {
			return do(ctx, session, o.reader, o.modifier, cluster)
		})
}

type modifyFunc func(ctx context.Context, session sessions.Session, cluster clusters.Cluster) (operations.Operation, error)

func (o *Operator) modify(ctx context.Context, cid string, typ clustermodels.Type, resolver sessions.SessionResolver, changeRev bool, do modifyFunc) (operations.Operation, error) {
	return o.operate(
		ctx,
		resolver,
		func(ctx context.Context, session sessions.Session) (operations.Operation, error) {
			if cid == "" {
				return operations.Operation{}, semerr.InvalidInput("cluster id must be specified")
			}
			return o.modifyCluster(ctx, cid, typ, session, changeRev, do)
		},
	)
}

// modifyByBackupID is like modify but first it resolve CID from backupID
func (o *Operator) modifyByBackupID(ctx context.Context, backupID string, typ clustermodels.Type, resolver sessions.SessionResolver, do modifyFunc) (operations.Operation, error) {
	return o.operate(
		ctx,
		resolver,
		func(ctx context.Context, session sessions.Session) (operations.Operation, error) {
			if backupID == "" {
				return operations.Operation{}, semerr.InvalidInput("backup id must be specified")
			}
			cid, err := o.metadb.ClusterIDByBackupID(ctx, backupID)
			if err != nil {
				return operations.Operation{}, err
			}
			return o.modifyCluster(ctx, cid, typ, session, true, do)
		},
	)
}

// modifyCluster function has common code for all modify* functions (modification itself, without access checks and CID resolving)
func (o *Operator) modifyCluster(ctx context.Context, cid string, typ clustermodels.Type, session sessions.Session, changeRev bool, do modifyFunc) (operations.Operation, error) {
	var err error
	var cluster clusters.Cluster
	if changeRev {
		cluster, err = o.lockCluster(ctx, cid, typ)
	} else {
		cluster, err = o.clusterByClusterID(ctx, cid, typ, models.VisibilityVisible)
	}

	if err != nil {
		return operations.Operation{}, err
	}

	op, err := do(ctx, session, cluster)
	if err != nil {
		return operations.Operation{}, err
	}

	// Update used quota if needed
	if session.Quota.Changed() {
		reqid := requestid.MustFromContext(ctx)
		if _, err = o.metadb.UpdateCloudUsedQuota(ctx, session.FolderCoords.CloudID, metadb.Resources(session.Quota.Diff()), reqid); err != nil {
			return operations.Operation{}, err
		}
	}

	if !changeRev {
		return op, nil
	}

	if err = o.metadb.CompleteClusterChange(ctx, cid, cluster.Revision); err != nil {
		return operations.Operation{}, err
	}

	return op, nil
}

type operateFunc func(ctx context.Context, session sessions.Session) (operations.Operation, error)

func (o *Operator) operate(ctx context.Context, resolver sessions.SessionResolver, do operateFunc) (operations.Operation, error) {
	ctx, session, original, err := o.sessions.BeginWithIdempotence(ctx, resolver)
	if err != nil {
		return operations.Operation{}, err
	}
	defer o.sessions.Rollback(ctx)

	if original.Exists {
		return original.Op, nil
	}

	op, err := do(ctx, session)
	if err != nil {
		// Enrich semantic error details if needed
		if se := semerr.AsSemanticError(err); se != nil {
			switch details := se.Details.(type) {
			case interface{ SetCloudExtID(string) }:
				details.SetCloudExtID(session.FolderCoords.CloudExtID)
			}
		}

		return operations.Operation{}, err
	}

	if err = o.sessions.Commit(ctx); err != nil {
		return operations.Operation{}, err
	}

	return op, nil
}

type fakeOperateFunc func(ctx context.Context, session sessions.Session) error

func (o *Operator) fakeOperate(ctx context.Context, resolver sessions.SessionResolver, do fakeOperateFunc) error {
	ctx, session, original, err := o.sessions.BeginWithIdempotence(ctx, resolver)
	if err != nil {
		return err
	}
	defer o.sessions.Rollback(ctx)

	if original.Exists {
		return nil
	}

	err = do(ctx, session)
	if err != nil {
		// Enrich semantic error details if needed
		if se := semerr.AsSemanticError(err); se != nil {
			switch details := se.Details.(type) {
			case interface{ SetCloudExtID(string) }:
				details.SetCloudExtID(session.FolderCoords.CloudExtID)
			}
		}

		return err
	}

	return nil
}

func (o *Operator) ReadCluster(ctx context.Context, cid string, do clusters.ReadClusterFunc, opts ...clusters.OperatorOption) error {
	if cid == "" {
		return semerr.InvalidInput("cluster id must be specified")
	}
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllRead}
	for _, opt := range opts {
		opt(opOpts)
	}

	ctx, session, err := o.sessions.Begin(ctx, sessions.ResolveByCluster(cid, opOpts.Permission))
	if err != nil {
		return err
	}
	defer o.sessions.Rollback(ctx)

	return do(ctx, session, o.reader)
}

func (o *Operator) clusterByClusterID(ctx context.Context, cid string, typ clustermodels.Type, vis models.Visibility) (clusters.Cluster, error) {
	cluster, err := o.metadb.ClusterByClusterID(ctx, cid, vis)
	if err != nil {
		return clusters.Cluster{}, wrapWithClusterNotFoundIfSQLErrNotFound(err, cid)
	}

	if cluster.Type != typ {
		return clusters.Cluster{}, wrapWithClusterNotFound(xerrors.Errorf("found invalid cluster type: requested %q but found %q", typ, cluster.Type), cid)
	}

	return clusters.NewClusterModel(cluster.Cluster, cluster.Pillar), nil
}

func (o *Operator) ReadOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do clusters.ReadOnClusterFunc, opts ...clusters.OperatorOption) error {
	if cid == "" {
		return semerr.InvalidInput("cluster id must be specified")
	}
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllRead}
	for _, opt := range opts {
		opt(opOpts)
	}

	ctx, session, err := o.sessions.Begin(ctx, sessions.ResolveByClusterDependency(cid, opOpts.Permission))
	if err != nil {
		return err
	}
	defer o.sessions.Rollback(ctx)
	cluster, err := o.clusterByClusterID(ctx, cid, typ, models.VisibilityVisible)
	if err != nil {
		return err
	}
	return do(ctx, session, o.reader, cluster)
}

func (o *Operator) ReadOnDeletedCluster(ctx context.Context, cid string, do clusters.ReadOnDeletedClusterFunc, opts ...clusters.OperatorOption) error {
	if cid == "" {
		return semerr.InvalidInput("cluster id must be specified")
	}
	opOpts := &clusters.OperatorOptions{Permission: models.PermMDBAllRead}
	for _, opt := range opts {
		opt(opOpts)
	}

	ctx, session, err := o.sessions.Begin(ctx, sessions.ResolveByDeletedCluster(cid, opOpts.Permission))
	if err != nil {
		return err
	}
	defer o.sessions.Rollback(ctx)

	return do(ctx, session, o.reader)
}
