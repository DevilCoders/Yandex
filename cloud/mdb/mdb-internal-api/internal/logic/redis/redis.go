package redis

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

//go:generate ../../../../scripts/mockgen.sh Redis

type Redis interface {
	Backup(ctx context.Context, backupID string) (backups.Backup, error)
	FolderBackups(ctx context.Context, fid string, pageToken backups.BackupsPageToken, pageSize int64) ([]backups.Backup, backups.BackupsPageToken, error)
	ClusterBackups(ctx context.Context, cid string, pageToken backups.BackupsPageToken, pageSize int64) ([]backups.Backup, backups.BackupsPageToken, error)
	BackupCluster(ctx context.Context, cid string) (operations.Operation, error)

	StartFailover(ctx context.Context, cid string, hostNames []string) (operations.Operation, error)
	Rebalance(ctx context.Context, cid string) (operations.Operation, error)

	StartCluster(ctx context.Context, cid string) (operations.Operation, error)
	StopCluster(ctx context.Context, cid string) (operations.Operation, error)
	MoveCluster(ctx context.Context, cid, destinationFolderID string) (operations.Operation, error)

	ListHosts(ctx context.Context, cid string, limit, offset int64) ([]hosts.HostExtended, pagination.OffsetPageToken, error)
	AddHosts(ctx context.Context, cid string, specs []rmodels.HostSpec) (operations.Operation, error)
}
