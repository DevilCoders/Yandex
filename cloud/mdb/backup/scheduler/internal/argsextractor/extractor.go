package argsextractor

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
)

type BackupArgsExtractor interface {
	GetBackupArgs(ctx context.Context, blank metadb.BackupBlank, backupIDGen generator.IDGenerator) (metadb.CreateBackupArgs, error)
}
