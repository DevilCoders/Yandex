package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func (mg *MongoDB) DeleteBackup(ctx context.Context, backupID string) (operations.Operation, error) {
	return mg.operator.ModifyOnClusterByBackupID(ctx, backupID, clusters.TypeMongoDB,
		func(ctx context.Context, session sessions.Session, _ clusterslogic.Reader, _ clusterslogic.Modifier, cl clusterslogic.Cluster) (operations.Operation, error) {
			err := mg.backups.MarkManagedBackupObsolete(ctx, backupID)
			if err != nil {
				return operations.Operation{}, err
			}

			op, err := mg.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cl.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      mongomodels.TaskTypeDeleteBackup,
					OperationType: mongomodels.OperationTypeDeleteBackup,
					Metadata: mongomodels.MetadataDeleteBackup{
						BackupID: backupID,
					},
					TaskArgs: map[string]interface{}{
						"backup_ids": []string{backupID},
					},

					Timeout:  optional.NewDuration(time.Hour * 24), // TODO: Calculate timeout properly
					Revision: cl.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			// TODO: Send event (?)
			return op, nil
		},
	)
}
