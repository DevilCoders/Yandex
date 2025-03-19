package provider

import (
	"context"

	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/provider/internal/mongopillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func (mg *MongoDB) User(ctx context.Context, cid, name string) (mongomodels.User, error) {
	if err := mongomodels.ValidateUserName(name); err != nil {
		return mongomodels.User{}, err
	}

	var user mongomodels.User
	if err := mg.operator.ReadOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			var pillar mongopillars.Cluster
			if err = cl.Pillar(&pillar); err != nil {
				return err
			}

			user, err = pillar.User(cid, name)
			return err
		},
	); err != nil {
		return mongomodels.User{}, err
	}

	return user, nil
}

func (mg *MongoDB) Users(ctx context.Context, cid string, limit, offset int64) ([]mongomodels.User, error) {
	var users []mongomodels.User
	if err := mg.operator.ReadOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			var pillar mongopillars.Cluster
			if err = cl.Pillar(&pillar); err != nil {
				return err
			}

			// TODO: support limit&offset

			users = pillar.Users(cid, false)
			return nil
		},
	); err != nil {
		return nil, err
	}

	return users, nil
}

func (mg *MongoDB) CreateUser(ctx context.Context, cid string, spec mongomodels.UserSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return mg.operator.CreateOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cl clusterslogic.Cluster) (operations.Operation, error) {
			var pillar mongopillars.Cluster
			if err := cl.Pillar(&pillar); err != nil {
				return operations.Operation{}, err
			}

			if len(spec.Permissions) == 0 {
				spec.SetDefaultPermissions(pillar.Databases(cid))
			}

			if err := pillar.CreateUser(spec, mg.cryptoProvider); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cl.ClusterID, cl.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return mg.tasks.CreateTask(ctx, session, tasks.CreateTaskArgs{
				ClusterID: cid,
				FolderID:  session.FolderCoords.FolderID,
				Auth:      session.Subject,
				TaskType:  mongomodels.TaskTypeUserCreate,
				TaskArgs: map[string]interface{}{
					"target-user": spec.Name,
				},
				OperationType: mongomodels.OperationTypeUserCreate,
				Metadata:      mongomodels.MetadataCreateUser{UserName: spec.Name},
				Revision:      cl.Revision,
			})
		},
	)
}

func (mg *MongoDB) DeleteUser(ctx context.Context, cid string, username string) (operations.Operation, error) {
	if err := mongomodels.ValidateUserName(username); err != nil {
		return operations.Operation{}, err
	}

	return mg.operator.DeleteOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cl clusterslogic.Cluster) (operations.Operation, error) {
			var pillar mongopillars.Cluster
			if err := cl.Pillar(&pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.DeleteUser(username); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cl.ClusterID, cl.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return mg.tasks.CreateTask(ctx, session, tasks.CreateTaskArgs{
				ClusterID: cid,
				FolderID:  session.FolderCoords.FolderID,
				Auth:      session.Subject,
				TaskType:  mongomodels.TaskTypeUserDelete,
				TaskArgs: map[string]interface{}{
					"target-user": username,
				},
				OperationType: mongomodels.OperationTypeUserDelete,
				Metadata:      mongomodels.MetadataDeleteUser{UserName: username},
				Revision:      cl.Revision,
			})
		},
	)
}
