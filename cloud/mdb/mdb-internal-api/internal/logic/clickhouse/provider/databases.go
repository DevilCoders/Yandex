package provider

import (
	"context"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ch *ClickHouse) database(ctx context.Context, reader clusterslogic.Reader, cid, name string) (chmodels.Database, error) {
	sc, err := ch.chSubCluster(ctx, reader, cid)
	if err != nil {
		return chmodels.Database{}, err
	}

	if sc.Pillar.SQLDatabaseManagement() {
		return chmodels.Database{}, semerr.FailedPrecondition("operation not permitted when SQL database management is enabled.")
	}

	// Construct response
	for _, dbname := range sc.Pillar.Data.ClickHouse.DBs {
		if dbname == name {
			return chmodels.Database{ClusterID: cid, Name: dbname}, nil
		}
	}

	return chmodels.Database{}, semerr.NotFoundf("database %q not found", name)
}

func (ch *ClickHouse) Database(ctx context.Context, cid, name string) (chmodels.Database, error) {
	var db chmodels.Database
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			var err error
			db, err = ch.database(ctx, reader, cid, name)
			return err
		},
	); err != nil {
		return chmodels.Database{}, err
	}

	return db, nil
}

func (ch *ClickHouse) databases(ctx context.Context, reader clusterslogic.Reader, cid string) ([]chmodels.Database, error) {
	sc, err := ch.chSubCluster(ctx, reader, cid)
	if err != nil {
		return nil, err
	}

	if sc.Pillar.SQLDatabaseManagement() {
		return nil, nil
	}

	// Construct response
	dbs := make([]chmodels.Database, 0, len(sc.Pillar.Data.ClickHouse.DBs))
	for _, db := range sc.Pillar.Data.ClickHouse.DBs {
		dbs = append(dbs, chmodels.Database{ClusterID: cid, Name: db})
	}

	return dbs, nil
}

func (ch *ClickHouse) Databases(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.Database, pagination.OffsetPageToken, error) {
	var dbs []chmodels.Database
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			var err error
			dbs, err = ch.databases(ctx, reader, cid)
			return err
		},
	); err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	page := pagination.NewPage(int64(len(dbs)), pageSize, offset)
	nextPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}

	return dbs[page.LowerIndex:page.UpperIndex], nextPageToken, nil
}

func (ch *ClickHouse) createDatabase(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier, session sessions.Session, cluster clusterslogic.Cluster, cid string, spec chmodels.DatabaseSpec) (operations.Operation, error) {
	subCluster, err := ch.chSubCluster(ctx, reader, cid)
	if err != nil {
		return operations.Operation{}, err
	}

	if subCluster.Pillar.SQLDatabaseManagement() {
		return operations.Operation{}, semerr.FailedPrecondition("operation not permitted when SQL database management is enabled.")
	}

	if err = subCluster.Pillar.AddDatabase(spec); err != nil {
		return operations.Operation{}, err
	}

	if err = modifier.UpdateSubClusterPillar(ctx, subCluster.ClusterID, subCluster.SubClusterID, cluster.Revision, subCluster.Pillar); err != nil {
		return operations.Operation{}, err
	}

	taskArgs := map[string]interface{}{
		"target-database": spec.Name,
	}

	op, err := ch.tasks.CreateTask(
		ctx,
		session,
		tasks.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      chmodels.TaskTypeDatabaseCreate,
			OperationType: chmodels.OperationTypeDatabaseAdd,
			Metadata:      chmodels.MetadataCreateDatabase{DatabaseName: spec.Name},
			TaskArgs:      taskArgs,
			Revision:      cluster.Revision,
		},
	)
	if err != nil {
		return operations.Operation{}, err
	}

	event := &cheventspub.CreateDatabase{
		Authentication:  ch.events.NewAuthentication(session.Subject),
		Authorization:   ch.events.NewAuthorization(session.Subject),
		RequestMetadata: ch.events.NewRequestMetadata(ctx),
		EventStatus:     cheventspub.CreateDatabase_STARTED,
		Details: &cheventspub.CreateDatabase_EventDetails{
			ClusterId:    cid,
			DatabaseName: spec.Name,
		},
	}
	em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return operations.Operation{}, err
	}
	event.EventMetadata = em

	if err = ch.events.Store(ctx, event, op); err != nil {
		return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
	}

	return op, nil
}

func (ch *ClickHouse) CreateDatabase(ctx context.Context, cid string, spec chmodels.DatabaseSpec) (operations.Operation, error) {
	if err := spec.Validate(); err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.CreateOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			return ch.createDatabase(ctx, reader, modifier, session, cluster, cid, spec)
		},
	)
}

func (ch *ClickHouse) deleteDatabase(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier, session sessions.Session, cluster clusterslogic.Cluster, cid, name string) (operations.Operation, error) {
	subCluster, err := ch.chSubCluster(ctx, reader, cid)
	if err != nil {
		return operations.Operation{}, err
	}

	if subCluster.Pillar.SQLDatabaseManagement() {
		return operations.Operation{}, semerr.FailedPrecondition("operation not permitted when SQL database management is enabled.")
	}

	if err = subCluster.Pillar.DeleteDatabase(name); err != nil {
		return operations.Operation{}, err
	}

	if err = modifier.UpdateSubClusterPillar(ctx, subCluster.ClusterID, subCluster.SubClusterID, cluster.Revision, subCluster.Pillar); err != nil {
		return operations.Operation{}, err
	}

	taskArgs := map[string]interface{}{
		"target-database": name,
	}

	op, err := ch.tasks.CreateTask(
		ctx,
		session,
		tasks.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      chmodels.TaskTypeDatabaseDelete,
			OperationType: chmodels.OperationTypeDatabaseDelete,
			Metadata:      chmodels.MetadataDeleteDatabase{DatabaseName: name},
			TaskArgs:      taskArgs,
			Revision:      cluster.Revision,
		},
	)
	if err != nil {
		return operations.Operation{}, err
	}

	event := &cheventspub.DeleteDatabase{
		Authentication:  ch.events.NewAuthentication(session.Subject),
		Authorization:   ch.events.NewAuthorization(session.Subject),
		RequestMetadata: ch.events.NewRequestMetadata(ctx),
		EventStatus:     cheventspub.DeleteDatabase_STARTED,
		Details: &cheventspub.DeleteDatabase_EventDetails{
			ClusterId:    cid,
			DatabaseName: name,
		},
	}
	em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return operations.Operation{}, err
	}
	event.EventMetadata = em

	if err = ch.events.Store(ctx, event, op); err != nil {
		return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
	}

	return op, nil
}

func (ch *ClickHouse) DeleteDatabase(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ch.operator.DeleteOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			return ch.deleteDatabase(ctx, reader, modifier, session, cluster, cid, name)
		},
	)
}
