package clusters

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

// Operator provides API for different actions with cluster including its creation and deletion. These functions
// implement common logic for preparation and cleanup. Every database logic provider must use Operator as point of
// entry for performing ANY action. Handle session management and other boilerplate.
type Operator interface {
	// Create cluster using specified function.
	Create(ctx context.Context, folderExtID string, do CreateFunc, opts ...OperatorOption) (operations.Operation, error)
	// FakeCreate mimics cluster creation using specified function but does not really create it.
	FakeCreate(ctx context.Context, folderExtID string, do FakeCreateFunc, opts ...OperatorOption) error
	// Restore cluster using specified function.
	Restore(ctx context.Context, folderExtID string, do RestoreFunc, opts ...OperatorOption) (operations.Operation, error)
	// RestoreWithoutBackupService restores cluster using specified function on MDB DBMSs without BackupService support.
	RestoreWithoutBackupService(ctx context.Context, folderExtID optional.String, cid string, do RestoreFunc, opts ...OperatorOption) (operations.Operation, error)
	// ReadCluster reads cluster using specified function.
	ReadCluster(ctx context.Context, cid string, do ReadClusterFunc, opts ...OperatorOption) error
	// ReadOnFolder reads something within the folder using specified function.
	ReadOnFolder(ctx context.Context, folderExtID string, do ReadOnFolderFunc, opts ...OperatorOption) error
	// Delete cluster using specified function.
	Delete(ctx context.Context, cid string, typ clustermodels.Type, do DeleteFunc, opts ...OperatorOption) (operations.Operation, error)
	// CreateOnCluster creates something within the cluster using specified function.
	CreateOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do CreateOnClusterFunc, opts ...OperatorOption) (operations.Operation, error)
	// ReadOnCluster reads something within the cluster using specified function.
	ReadOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do ReadOnClusterFunc, opts ...OperatorOption) error
	// ReadOnDeletedCluster reads something within the deleted cluster using specified function.
	ReadOnDeletedCluster(ctx context.Context, cid string, do ReadOnDeletedClusterFunc, opts ...OperatorOption) error
	// ModifyOnCluster modifies something within the cluster using specified function.
	ModifyOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do ModifyOnClusterFunc, opts ...OperatorOption) (operations.Operation, error)
	// ModifyOnNotRunningCluster modifies something within the stopped cluster using specified function.
	ModifyOnNotRunningCluster(ctx context.Context, cid string, typ clustermodels.Type, do ModifyOnNotRunningClusterFunc, opts ...OperatorOption) (operations.Operation, error)
	// ModifyOnNotStoppedCluster modifies something within the running cluster using specified function.
	ModifyOnNotStoppedCluster(ctx context.Context, cid string, typ clustermodels.Type, do ModifyOnNotStoppedClusterFunc, opts ...OperatorOption) (operations.Operation, error)
	// ModifyOnClusterByBackupID modifies something within the cluster using specified function. Find cluster by Backup ID
	ModifyOnClusterByBackupID(ctx context.Context, backupID string, typ clustermodels.Type, do ModifyOnClusterFunc, opts ...OperatorOption) (operations.Operation, error)
	// MoveCluster moves entire cluster within folders using specified function.
	MoveCluster(ctx context.Context, cid, dstFolderExtID string, typ clustermodels.Type, do MoveClusterFunc) (operations.Operation, error)
	// DeleteOnCluster deletes something within the cluster using specified function.
	DeleteOnCluster(ctx context.Context, cid string, typ clustermodels.Type, do DeleteOnClusterFunc, opts ...OperatorOption) (operations.Operation, error)
	// ModifyOnClusterWithoutRevChanging modifies something within the cluster using specified function.
	ModifyOnClusterWithoutRevChanging(ctx context.Context, cid string, typ clustermodels.Type, do ModifyOnClusterFunc, opts ...OperatorOption) (operations.Operation, error)
}

type (
	CreateFunc                    func(ctx context.Context, session sessions.Session, creator Creator) (clustermodels.Cluster, operations.Operation, error)
	FakeCreateFunc                func(ctx context.Context, session sessions.Session, creator Creator) error
	RestoreFunc                   func(ctx context.Context, session sessions.Session, reader Reader, restorer Restorer) (clustermodels.Cluster, operations.Operation, error)
	ReadClusterFunc               func(ctx context.Context, session sessions.Session, reader Reader) error
	ReadOnFolderFunc              func(ctx context.Context, session sessions.Session, reader Reader) error
	DeleteFunc                    func(ctx context.Context, session sessions.Session, cluster Cluster, reader Reader) (operations.Operation, error)
	CreateOnClusterFunc           func(ctx context.Context, session sessions.Session, reader Reader, modifier Modifier, cluster Cluster) (operations.Operation, error)
	ReadOnClusterFunc             func(ctx context.Context, session sessions.Session, reader Reader, cluster Cluster) error
	ReadOnDeletedClusterFunc      func(ctx context.Context, session sessions.Session, reader Reader) error
	ModifyOnClusterFunc           func(ctx context.Context, session sessions.Session, reader Reader, modifier Modifier, cluster Cluster) (operations.Operation, error)
	ModifyOnNotRunningClusterFunc func(ctx context.Context, session sessions.Session, reader Reader, modifier Modifier, cluster Cluster) (operations.Operation, error)
	ModifyOnNotStoppedClusterFunc func(ctx context.Context, session sessions.Session, reader Reader, modifier Modifier, cluster Cluster) (operations.Operation, error)
	MoveClusterFunc               func(ctx context.Context, session sessions.Session, modifier Modifier, cluster Cluster) (operations.Operation, error)
	DeleteOnClusterFunc           func(ctx context.Context, session sessions.Session, reader Reader, modifier Modifier, cluster Cluster) (operations.Operation, error)
)

type OperatorOptions struct {
	Permission models.Permission
}

type OperatorOption func(opts *OperatorOptions)

func WithPermission(permission models.Permission) OperatorOption {
	return func(opts *OperatorOptions) {
		opts.Permission = permission
	}
}
