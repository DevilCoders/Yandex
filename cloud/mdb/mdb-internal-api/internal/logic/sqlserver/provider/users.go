package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/provider/internal/sspillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ss *SQLServer) User(ctx context.Context, cid, name string) (ssmodels.User, error) {
	var res ssmodels.User
	if err := ss.operator.ReadOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := sspillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			validator := ssmodels.NewSystemsUsersValidator()
			err = validator.ValidateString(name)
			if err != nil {
				return semerr.NotFoundf("user %q not found", name)
			}
			res, err = pillar.User(cid, name)
			return err
		},
	); err != nil {
		return ssmodels.User{}, err
	}

	return res, nil
}

func (ss *SQLServer) Users(ctx context.Context, cid string, limit, offset int64) ([]ssmodels.User, error) {
	var res []ssmodels.User
	if err := ss.operator.ReadOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := sspillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			res, err = pillar.Users(cid), nil
			return err
		},
	); err != nil {
		return nil, err
	}

	validator := ssmodels.NewSystemsUsersValidator()
	filteredUsers := make([]ssmodels.User, 0, len(res))
	for _, user := range res {
		if err := validator.ValidateString(user.Name); err != nil {
			continue
		}
		filteredUsers = append(filteredUsers, user)
	}
	return filteredUsers, nil
}

func (ss *SQLServer) CreateUser(ctx context.Context, cid string, spec ssmodels.UserSpec) (operations.Operation, error) {
	return ss.operator.CreateOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.AddUser(spec, ss.cryptoProvider); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeUserCreate,
					OperationType: ssmodels.OperationTypeUserCreate,
					Metadata: ssmodels.MetadataCreateUser{
						UserName: spec.Name,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-user": spec.Name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (ss *SQLServer) DeleteUser(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ss.operator.DeleteOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.DeleteUser(name); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cid, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeUserDelete,
					OperationType: ssmodels.OperationTypeUserDelete,
					Metadata: ssmodels.MetadataDeleteUser{
						UserName: name,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-user": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (ss *SQLServer) UpdateUser(ctx context.Context, cid string, args sqlserver.UserArgs) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.UpdateUser(ss.cryptoProvider, args); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeUserModify,
					OperationType: ssmodels.OperationTypeUserModify,
					Metadata: ssmodels.MetadataUpdateUser{
						UserName: args.Name,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-user": args.Name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (ss *SQLServer) GrantPermission(ctx context.Context, cid, username string, permission ssmodels.Permission) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.GrantPermissions(username, permission, ss.cryptoProvider); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeUserModify,
					OperationType: ssmodels.OperationTypeUserPermissionGrant,
					Metadata: ssmodels.MetadataGrantPermission{
						UserName: username,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-user": username,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			return op, err
		},
	)
}

func (ss *SQLServer) RevokePermission(ctx context.Context, cid, username string, permission ssmodels.Permission) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.RevokePermissions(username, permission, ss.cryptoProvider); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeUserModify,
					OperationType: ssmodels.OperationTypeUserPermissionRevoke,
					Metadata: ssmodels.MetadataRevokePermission{
						UserName: username,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-user": username,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			return op, nil
		},
	)
}
