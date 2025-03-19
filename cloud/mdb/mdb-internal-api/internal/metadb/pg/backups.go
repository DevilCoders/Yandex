package pg

import (
	"context"
	"database/sql"
	"strings"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
)

var (
	querySelectBackupByID = sqlutil.Stmt{
		Name: "SelectBackupByID",
		Query: `
SELECT
	backup_id,
	cid,
	subcid,
	shard_id,
	status,
	scheduled_date,
	created_at,
	delayed_until,
	started_at,
	finished_at,
	updated_at,
	shipment_id,
	metadata,
	errors,
	initiator,
	method
FROM dbaas.backups
WHERE backup_id = :backup_id`,
	}

	querySelectBackupsByClusterID = sqlutil.Stmt{
		Name: "SelectBackupsByClusterID",
		Query: `
SELECT
	backup_id,
	cid,
	subcid,
	shard_id,
	status,
	scheduled_date,
	created_at,
	delayed_until,
	started_at,
	finished_at,
	updated_at,
	shipment_id,
	metadata,
	errors,
	initiator,
	method
FROM dbaas.backups
WHERE cid = :cid`,
	}

	querySelectClusterIDByBackupID = sqlutil.Stmt{
		Name: "SelectClusterIDByBackupID",
		Query: `
SELECT
    cid
FROM dbaas.backups
WHERE backup_id = :backup_id`,
	}

	queryMarkBackupObsolete = sqlutil.Stmt{
		Name: "MarkBackupObsolete",
		Query: `
UPDATE dbaas.backups
SET status = 'OBSOLETE'
WHERE backup_id = :backup_id`,
	}

	scheduleBackupForNow = sqlutil.Stmt{
		Name: "ScheduleBackupForNow",
		Query: `
SELECT * FROM code.plan_managed_backup(
	i_backup_id      => :backup_id,
	i_cid            => :cid,
	i_subcid         => :subcid,
	i_shard_id       => :shard_id,
	i_status         => CAST('PLANNED' as dbaas.backup_status),
	i_method         => CAST(:backup_method as dbaas.backup_method),
	i_initiator      => CAST('USER' as dbaas.backup_initiator),
	i_delayed_until  => NOW(),
	i_scheduled_date => NULL,
	i_parent_ids     => '{}',
	i_child_id		 => NULL,
	i_metadata		 => CAST(:metadata as jsonb)
);
`,
	}
)

type Backup struct {
	ID        string `db:"backup_id"`
	ClusterID string `db:"cid"`
	// TODO: https://st.yandex-team.ru/MDB-16099
	SubClusterID string         `db:"subcid"`
	ShardID      sql.NullString `db:"shard_id"`
	Status       string         `db:"status"`
	Scheduled    sql.NullTime   `db:"scheduled_date"`
	CreatedAt    time.Time      `db:"created_at"`
	DelayedUntil time.Time      `db:"delayed_until"`
	StartedAt    sql.NullTime   `db:"started_at"`
	FinishedAt   sql.NullTime   `db:"finished_at"`
	UpdatedAt    sql.NullTime   `db:"updated_at"`
	ShipmentID   sql.NullString `db:"shipment_id"`
	Metadata     sql.NullString `db:"metadata"`
	Errors       sql.NullString `db:"errors"`
	Initiator    string         `db:"initiator"`
	Method       string         `db:"method"`
}

func (b *Backend) ScheduleBackupForNow(ctx context.Context, backupID string, cid string, subcid string, shardID string, method backups.BackupMethod, metadata []byte) error {
	_, err := sqlutil.QueryTx(
		ctx,
		scheduleBackupForNow,
		map[string]interface{}{
			"backup_id":     backupID,
			"cid":           cid,
			"subcid":        subcid,
			"shard_id":      shardID,
			"backup_method": backups.BackupMethodToNameMapping[method],
			"metadata":      string(metadata),
		},
		sqlutil.NopParser,
		b.logger,
	)
	return err
}

func (b *Backend) BackupByID(ctx context.Context, backupID string) (backups.ManagedBackup, error) {
	var bak Backup
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&bak)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		querySelectBackupByID,
		map[string]interface{}{
			"backup_id": backupID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return backups.ManagedBackup{}, err
	}
	if count == 0 {
		return backups.ManagedBackup{}, sqlerrors.ErrNotFound
	}

	return backupFromDB(bak), nil
}

func (b *Backend) BackupsByClusterID(ctx context.Context, clusterID string) ([]backups.ManagedBackup, error) {
	managedBackups := []backups.ManagedBackup{}
	parser := func(rows *sqlx.Rows) error {
		var backup Backup
		err := rows.StructScan(&backup)
		if err != nil {
			return err
		}

		managedBackups = append(managedBackups, backupFromDB(backup))
		return nil
	}

	_, err := sqlutil.QueryTx(
		ctx,
		querySelectBackupsByClusterID,
		map[string]interface{}{
			"cid": clusterID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return managedBackups, err
	}

	return managedBackups, nil
}

func (b *Backend) ClusterIDByBackupID(ctx context.Context, backupID string) (string, error) {
	var cid string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&cid)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectClusterIDByBackupID,
		map[string]interface{}{
			"backup_id": backupID,
		},
		parser,
		b.logger,
	)
	if err != nil {
		return "", err
	}
	if count == 0 {
		return "", sqlerrors.ErrNotFound
	}

	return cid, nil
}

func (b *Backend) MarkBackupObsolete(ctx context.Context, backupID string) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryMarkBackupObsolete,
		map[string]interface{}{
			"backup_id": backupID,
		},
		sqlutil.NopParser,
		b.logger,
	)
	return err
}

func backupFromDB(backup Backup) backups.ManagedBackup {
	//Currently we just copypaste everything, but in future we'll want to add some parsing of metadata
	b := backups.ManagedBackup{
		ID:           backup.ID,
		ClusterID:    backup.ClusterID,
		SubClusterID: backup.SubClusterID,
		ShardID:      optional.String{String: backup.ShardID.String, Valid: backup.ShardID.Valid},
		Status:       backups.NameToBackupStatusMapping[strings.ToLower(backup.Status)],
		Scheduled:    optional.Time{Time: backup.Scheduled.Time, Valid: backup.Scheduled.Valid},
		CreatedAt:    backup.CreatedAt,
		DelayedUntil: backup.DelayedUntil,
		StartedAt:    optional.Time{Time: backup.StartedAt.Time, Valid: backup.StartedAt.Valid},
		FinishedAt:   optional.Time{Time: backup.FinishedAt.Time, Valid: backup.FinishedAt.Valid},
		UpdatedAt:    optional.Time{Time: backup.UpdatedAt.Time, Valid: backup.UpdatedAt.Valid},
		ShipmentID:   optional.String{String: backup.ShipmentID.String, Valid: backup.ShipmentID.Valid},
		Metadata:     optional.String{String: backup.Metadata.String, Valid: backup.Metadata.Valid},
		Errors:       optional.String{String: backup.Errors.String, Valid: backup.Errors.Valid},
		Initiator:    backups.NameToBackupInitiatorMapping[strings.ToLower(backup.Initiator)],
		Method:       backups.NameToBackupMethodMapping[strings.ToLower(backup.Method)],
	}

	return b
}
