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

func (mg *MongoDB) Database(ctx context.Context, cid string, name string) (mongomodels.Database, error) {
	var db mongomodels.Database
	if err := mg.operator.ReadOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			var pillar mongopillars.Cluster
			if err = cl.Pillar(&pillar); err != nil {
				return err
			}

			db, err = pillar.Database(cid, name)
			return err
		},
	); err != nil {
		return mongomodels.Database{}, err
	}

	return db, nil
}

func (mg *MongoDB) Databases(ctx context.Context, cid string, offset, limit int64) ([]mongomodels.Database, error) {
	var dbs []mongomodels.Database
	if err := mg.operator.ReadOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			var pillar mongopillars.Cluster
			if err = cl.Pillar(&pillar); err != nil {
				return err
			}
			dbs = pillar.Databases(cid)
			return nil
		},
	); err != nil {
		return nil, err
	}

	return dbs, nil
}

func (mg *MongoDB) CreateDatabase(ctx context.Context, cid string, spec mongomodels.DatabaseSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return mg.operator.CreateOnCluster(ctx, cid, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cl clusterslogic.Cluster) (operations.Operation, error) {
			var pillar mongopillars.Cluster
			if err := cl.Pillar(&pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.AddDatabase(spec); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdatePillar(ctx, cl.ClusterID, cl.Revision, pillar); err != nil {
				return operations.Operation{}, err
			}

			return mg.tasks.CreateTask(ctx, session, tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				TaskType:      mongomodels.TaskTypeDatabaseCreate,
				OperationType: mongomodels.OperationTypeDatabaseAdd,
				Metadata:      mongomodels.MetadataCreateDatabase{DatabaseName: spec.Name},
				Revision:      cl.Revision,
			})
		},
	)
}
