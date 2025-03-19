package pg

import (
	"database/sql"
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	// DBName is metaDB database name in PostgreSQL
	DBName = "dbaas_metadb"
)

type ClusterPillar struct {
	Data ClusterData `json:"data"`
}

type ClusterData struct {
	S3Bucket string `json:"s3_bucket"`
}

func UnmarshalS3Bucket(raw json.RawMessage) (string, error) {
	p := &ClusterPillar{}
	if err := json.Unmarshal(raw, p); err != nil {
		return "", xerrors.Errorf("failed to unmarshal cluster pillar: %w", err)
	}

	return p.Data.S3Bucket, nil
}

type backupBlankRow struct {
	BackupID       sql.NullString     `db:"backup_id"`
	ClusterID      string             `db:"cid"`
	ClusterType    metadb.ClusterType `db:"cluster_type"`
	SubClusterID   string             `db:"subcid"`
	SubClusterName string             `db:"subcluster_name"`
	ShardID        sql.NullString     `db:"shard_id"`
	ScheduledTS    time.Time          `db:"scheduled_ts"`
	SleepSeconds   int                `db:"sleep_seconds"`
}

type backupRow struct {
	BackupID      string                 `db:"backup_id"`
	ClusterID     string                 `db:"cid"`
	SubClusterID  string                 `db:"subcid"`
	ClusterType   metadb.ClusterType     `db:"cluster_type"`
	ShardID       sql.NullString         `db:"shard_id"`
	ScheduledDate sql.NullTime           `db:"scheduled_date"`
	Status        metadb.BackupStatus    `db:"status"`
	Method        metadb.BackupMethod    `db:"method"`
	Initiator     metadb.BackupInitiator `db:"initiator"`
	DelayedUntil  time.Time              `db:"delayed_until"`
	CreatedAt     time.Time              `db:"created_at"`
	StartedAt     sql.NullTime           `db:"started_at"`
	FinishedAt    sql.NullTime           `db:"finished_at"`
	UpdatedAt     sql.NullTime           `db:"updated_at"`
	ShipmentID    sql.NullString         `db:"shipment_id"`
	Metadata      []byte                 `db:"metadata"`
	Errors        []byte                 `db:"errors"`
}

func formatBackupBlank(r backupBlankRow) metadb.BackupBlank {
	ret := metadb.BackupBlank{
		ClusterID:      r.ClusterID,
		ClusterType:    r.ClusterType,
		SubClusterID:   r.SubClusterID,
		SubClusterName: r.SubClusterName,
		ScheduledTS:    r.ScheduledTS,
		SleepSeconds:   r.SleepSeconds,
	}
	if r.ShardID.Valid {
		ret.ShardID.Set(r.ShardID.String)
	}
	return ret
}

func formatBackup(r backupRow) (metadb.Backup, error) {
	ret := metadb.Backup{
		BackupID:     r.BackupID,
		ClusterType:  r.ClusterType,
		ClusterID:    r.ClusterID,
		SubClusterID: r.SubClusterID,
		Status:       r.Status,
		Method:       r.Method,
		Initiator:    r.Initiator,
		CreatedAt:    r.CreatedAt,
		Metadata:     r.Metadata,
		DelayedUntil: r.DelayedUntil,
	}
	if r.ShardID.Valid {
		ret.ShardID.Set(r.ShardID.String)
	}

	if r.ShipmentID.Valid {
		ret.ShipmentID.Set(r.ShipmentID.String)
	}

	if r.ScheduledDate.Valid {
		ret.ScheduledDate.Set(r.ScheduledDate.Time)
	}

	if r.StartedAt.Valid {
		ret.StartedAt.Set(r.StartedAt.Time)
	}

	if r.UpdatedAt.Valid {
		ret.UpdatedAt.Set(r.UpdatedAt.Time)
	}

	if r.FinishedAt.Valid {
		ret.FinishedAt.Set(r.FinishedAt.Time)
	}

	errs, err := UnmarshalErrors(r.Errors)
	if err != nil {
		return metadb.Backup{}, err
	}
	ret.Errors = errs

	return ret, nil
}

type host struct {
	ClusterID    string         `db:"cid"`
	SubClusterID string         `db:"subcid"`
	ShardID      sql.NullString `db:"shard_id"`
	FQDN         string         `db:"fqdn"`
}

func hostFromDB(h host) (metadb.Host, error) {
	return metadb.Host{
		ClusterID:    h.ClusterID,
		SubClusterID: h.SubClusterID,
		ShardID:      optional.String(h.ShardID),
		FQDN:         h.FQDN,
	}, nil
}

type hostPillar struct {
	FQDN  string         `db:"fqdn"`
	Value sql.NullString `db:"value"`
}

type Error struct {
	Err    string    `json:"err"`
	IsTemp bool      `json:"is_temp"`
	TS     time.Time `json:"ts"`
}

func ErrorFromMError(err metadb.Error) Error {
	return Error{IsTemp: err.IsTemp, Err: err.Error(), TS: err.TS}
}

type Errors struct {
	Errors []Error `json:"errors"`
}

func NewErrors() *Errors {
	return &Errors{}
}

func FromErrors(errors ...error) *Errors {
	if len(errors) == 0 {
		return &Errors{}
	}

	errs := NewErrors()
	for _, err := range errors {
		errs.Add(*metadb.FromError(err))
	}
	return errs
}

func (errs *Errors) Add(err metadb.Error) {
	errs.Errors = append(errs.Errors, ErrorFromMError(err))
}

func (errs *Errors) Marshal() ([]byte, error) {
	return json.Marshal(errs)
}

func UnmarshalErrors(raw json.RawMessage) (*Errors, error) {
	errs := NewErrors()
	if len(raw) == 0 {
		return errs, nil
	}

	if err := json.Unmarshal(raw, errs); err != nil {
		return &Errors{}, xerrors.Errorf("failed to unmarshal errors: %w", err)
	}

	return errs, nil
}

type cluster struct {
	ClusterID   string             `db:"cid"`
	Name        string             `db:"name"`
	Type        metadb.ClusterType `db:"type"`
	Environment metadb.SaltEnv     `db:"env"`
}

func clusterFromDB(c cluster) metadb.Cluster {
	return metadb.Cluster{
		ClusterID:   c.ClusterID,
		Name:        c.Name,
		Type:        c.Type,
		Environment: c.Environment,
	}
}

type shard struct {
	SubClusterID   string         `db:"subcluster_id"`
	SubClusterName string         `db:"subcluster_name"`
	ID             sql.NullString `db:"shard_id"`
	Name           sql.NullString `db:"shard_name"`
}

func shardFromDB(s shard) metadb.Shard {
	return metadb.Shard{
		SubClusterID:   s.SubClusterID,
		SubClusterName: s.SubClusterName,
		ID:             optional.String(s.ID),
		Name:           optional.String(s.Name),
	}
}

type subcluster struct {
	ClusterID      string `db:"cluster_id"`
	SubClusterID   string `db:"subcluster_id"`
	SubClusterName string `db:"subcluster_name"`
}

func subclusterFromDB(s subcluster) metadb.SubCluster {
	return metadb.SubCluster{
		ClusterID:    s.ClusterID,
		SubClusterID: s.SubClusterID,
		Name:         s.SubClusterName,
	}
}

type versionRow struct {
	Component      string `db:"component"`
	Edition        string `db:"edition"`
	MajorVersion   string `db:"major_version"`
	MinorVersion   string `db:"minor_version"`
	PackageVersion string `db:"package_version"`
}

func componentVersionFromDB(version versionRow) metadb.ComponentVersion {
	return metadb.ComponentVersion{
		Component:      version.Component,
		Edition:        version.Edition,
		MajorVersion:   version.MajorVersion,
		MinorVersion:   version.MinorVersion,
		PackageVersion: version.PackageVersion,
	}
}

func clusterTypesToDB(ctypes []metadb.ClusterType) []string {
	strCtypes := make([]string, len(ctypes))
	for i, ct := range ctypes {
		strCtypes[i] = string(ct)
	}

	return strCtypes
}

func clusterStatusesToDB(cstatuses []metadb.ClusterStatus) []string {
	strCstats := make([]string, len(cstatuses))
	for i, ct := range cstatuses {
		strCstats[i] = string(ct)
	}

	return strCstats
}

func sqlInitiatorsFromModel(initiators []metadb.BackupInitiator) []string {
	if len(initiators) < 1 {
		return nil
	}
	sqlInitiators := make([]string, len(initiators))
	for i := range initiators {
		sqlInitiators[i] = string(initiators[i])
	}
	return sqlInitiators
}

func sqlStatusesFromModel(statuses []metadb.BackupStatus) []string {
	if len(statuses) < 1 {
		return nil
	}
	sqlStatuses := make([]string, len(statuses))
	for i := range statuses {
		sqlStatuses[i] = string(statuses[i])
	}
	return sqlStatuses
}
