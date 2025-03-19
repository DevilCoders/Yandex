package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/provider/internal/sspillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/slices"
)

func (ss *SQLServer) Database(ctx context.Context, cid, name string) (ssmodels.Database, error) {
	var res ssmodels.Database
	if err := ss.operator.ReadOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := sspillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			res, err = pillar.Database(cid, name)
			return err
		},
	); err != nil {
		return ssmodels.Database{}, err
	}

	return res, nil
}

func (ss *SQLServer) Databases(ctx context.Context, cid string, limit, offset int64) ([]ssmodels.Database, error) {
	var res []ssmodels.Database
	if err := ss.operator.ReadOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cl clusterslogic.Cluster) error {
			var err error

			pillar := sspillars.NewCluster()
			if err = cl.Pillar(pillar); err != nil {
				return err
			}

			res, err = pillar.Databases(cid), nil
			return err
		},
	); err != nil {
		return nil, err
	}

	return res, nil
}

func (ss *SQLServer) CreateDatabase(ctx context.Context, cid string, spec ssmodels.DatabaseSpec) (operations.Operation, error) {
	return ss.operator.CreateOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.AddDatabase(spec); err != nil {
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
					TaskType:      ssmodels.TaskTypeDatabaseCreate,
					OperationType: ssmodels.OperationTypeDatabaseAdd,
					Metadata: ssmodels.MetadataCreateDatabase{
						DatabaseName: spec.Name,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-database": spec.Name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (ss *SQLServer) DeleteDatabase(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ss.operator.DeleteOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if err := pillar.DeleteDatabase(name); err != nil {
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
					TaskType:      ssmodels.TaskTypeDatabaseDelete,
					OperationType: ssmodels.OperationTypeDatabaseDelete,
					Metadata: ssmodels.MetadataDeleteDatabase{
						DatabaseName: name,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-database": name,
					},
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (ss *SQLServer) RestoreDatabase(ctx context.Context, cid string, spec ssmodels.RestoreDatabaseSpec) (operations.Operation, error) {
	if spec.Time.After(time.Now()) {
		// we should check time before calling lock_cluster,
		// because we can choose newly created revision
		// and fail to find cluster rev for it.
		// Suddenly it would look like a NotFound error.
		return operations.Operation{}, semerr.InvalidInput("time must be in the past")
	}
	return ss.operator.CreateOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}
			sourceCluster, _, err := reader.ClusterAndResourcesAtTime(ctx,
				cluster.ClusterID, spec.Time, clusters.TypeSQLServer, hosts.RoleSQLServer)
			if err != nil {
				return operations.Operation{}, err
			}
			//check that backup exists

			_, backupID, err := bmodels.DecodeGlobalBackupID(spec.BackupID)
			if err != nil {
				return operations.Operation{}, err
			}
			backup, err := ss.backups.BackupByClusterIDBackupID(ctx, cid, backupID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return ss.listS3BackupImpl(ctx, reader, client, cid)
				})
			if err != nil {
				return operations.Operation{}, err
			}

			//check that backup contains database requested
			if !slices.ContainsString(backup.Databases, spec.FromDatabase) {
				return operations.Operation{}, semerr.InvalidInputf("database %s is not found in the backup chosen", spec.FromDatabase)
			}

			//pillar of the same cluster at the previous revision from which we'll restore the database
			var sourcePillar sspillars.Cluster
			if err := sourceCluster.Pillar(&sourcePillar); err != nil {
				return operations.Operation{}, err
			}

			if _, ok := sourcePillar.Data.SQLServer.Databases[spec.FromDatabase]; !ok {
				return operations.Operation{}, semerr.InvalidInputf("database %s is not found at the given time", spec.FromDatabase)
			}

			if err := pillar.AddDatabase(spec.DatabaseSpec); err != nil {
				return operations.Operation{}, err
			}

			for user := range sourcePillar.Data.SQLServer.Users {
				if user == "sa" {
					continue
				}
				if userdata, ok := pillar.Data.SQLServer.Users[user]; ok { //grant access only to logins existing now. Skip the missing ones.
					if srcdatabase, ok := sourcePillar.Data.SQLServer.Users[user].Databases[spec.FromDatabase]; ok {
						permissions := make([]ssmodels.Permission, 0, len(userdata.Databases))
						for dbName, userDBData := range userdata.Databases {
							permissions = append(permissions, ssmodels.Permission{
								DatabaseName: dbName,
								Roles:        userDBData.Roles,
							})
						}

						permissions = append(permissions, ssmodels.Permission{DatabaseName: spec.Name, Roles: srcdatabase.Roles[:]})
						ua := sqlserver.UserArgs{Name: user, ClusterID: cid, Permissions: ssmodels.OptionalPermissions{Permissions: permissions, Valid: true}}
						if err := pillar.UpdateUser(ss.cryptoProvider, ua); err != nil {
							return operations.Operation{}, err
						}
					}
				}
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
					TaskType:      ssmodels.TaskTypeDatabaseRestore,
					OperationType: ssmodels.OperationTypeDatabaseRestore,
					Metadata: ssmodels.MetadataRestoreDatabase{
						ClusterID:    cid,
						DatabaseName: spec.Name,
						FromDatabase: spec.FromDatabase,
						BackupID:     spec.BackupID,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-database": spec.Name,
						"db-restore-from": map[string]interface{}{
							"database":  spec.FromDatabase,
							"backup-id": backupID,
							"time":      spec.Time.UTC().Format(time.RFC3339),
						},
					},
					// TODO: MDB-16309 better timeout estimation
					Timeout: optional.NewDuration(9 * time.Hour),
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (ss *SQLServer) ImportDatabaseBackup(ctx context.Context, cid string, spec ssmodels.ImportDatabaseBackupSpec) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := spec.Validate(); err != nil {
				return operations.Operation{}, err
			}
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}
			if err := requireServiceAccount(pillar); err != nil {
				return operations.Operation{}, err
			}
			if err := pillar.AddDatabase(spec.DatabaseSpec); err != nil {
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
					TaskType:      ssmodels.TaskTypeDatabaseBackupImport,
					OperationType: ssmodels.OperationTypeDatabaseImport,
					Metadata: ssmodels.MetadataImportDatabaseBackup{
						ClusterID:    cid,
						DatabaseName: spec.Name,
						S3Bucket:     spec.S3Bucket,
						S3Path:       spec.S3Path,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-database": spec.Name,
						"backup-import": map[string]interface{}{
							"s3": map[string]interface{}{
								"s3_bucket": spec.S3Bucket,
								"s3_path":   spec.S3Path,
								"files":     spec.Files,
							},
						},
						"db-restore-from": map[string]interface{}{
							"database":  spec.Name,
							"backup-id": "LATEST",
						},
					},
					// TODO: MDB-16309 better timeout estimation
					Timeout: optional.NewDuration(9 * time.Hour),
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}
			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
	)
}

func (ss *SQLServer) ExportDatabaseBackup(ctx context.Context, cid string, spec ssmodels.ExportDatabaseBackupSpec) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, cid, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if err := spec.Validate(); err != nil {
				return operations.Operation{}, err
			}
			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}
			if err := requireServiceAccount(pillar); err != nil {
				return operations.Operation{}, err
			}
			if _, err := pillar.Database(cid, spec.Name); err != nil {
				return operations.Operation{}, err
			}
			op, err := ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeDatabaseBackupExport,
					OperationType: ssmodels.OperationTypeDatabaseExport,
					Metadata: ssmodels.MetadataExportDatabaseBackup{
						ClusterID:    cid,
						DatabaseName: spec.Name,
						S3Bucket:     spec.S3Bucket,
						S3Path:       spec.S3Path,
					},
					Revision: cluster.Revision,
					TaskArgs: map[string]interface{}{
						"target-database": spec.Name,
						"backup-export": map[string]interface{}{
							"s3": map[string]interface{}{
								"s3_bucket": spec.S3Bucket,
								"s3_path":   spec.S3Path,
								"prefix":    spec.Prefix,
							},
						},
					},
					// TODO: MDB-16309 better timeout estimation
					Timeout: optional.NewDuration(9 * time.Hour),
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}
			if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
	)
}

func requireServiceAccount(p *sspillars.Cluster) error {
	if p.Data.ServiceAccountID == "" {
		return semerr.FailedPrecondition("service account should be set")
	}
	return nil
}
