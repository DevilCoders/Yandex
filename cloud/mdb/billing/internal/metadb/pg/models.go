package pg

import (
	"database/sql"

	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
)

const (
	// DBName is metaDB database name in PostgreSQL
	DBName = "dbaas_metadb"
)

type backupRow struct {
	BackupID string        `db:"backup_id"`
	DataSize sql.NullInt64 `db:"data_size"`
}

func formatBackup(r backupRow) metadb.Backup {
	b := metadb.Backup{
		BackupID: r.BackupID,
		DataSize: 0,
	}
	if r.DataSize.Valid {
		b.DataSize = r.DataSize.Int64
	}

	return b
}

type clusterRow struct {
	ID       string             `db:"cluster_id"`
	Type     metadb.ClusterType `db:"cluster_type"`
	FolderID string             `db:"folder_id"`
	CloudID  string             `db:"cloud_id"`
}

func formatCluster(r clusterRow) metadb.Cluster {
	return metadb.Cluster(r)
}
