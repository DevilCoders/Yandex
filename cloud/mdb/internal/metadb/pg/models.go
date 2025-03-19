package pg

import (
	"database/sql"
	"encoding/json"
	"time"

	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
)

const (
	// DBName is metaDB database name in PostgreSQL
	DBName = "dbaas_metadb"
	// InternalAPIUserName is metaDB database user for Internal API
	InternalAPIUserName = "dbaas_api"
)

// Operation models 'worker_queue' table
type Operation struct {
	ID            string         `db:"task_id"`
	FolderID      string         `db:"folder_id"`
	CreatedBy     string         `db:"created_by"`
	ClusterID     sql.NullString `db:"cid"`
	WorkerID      string         `db:"worker_id"`
	TaskType      string         `db:"task_type"`
	Args          []byte         `db:"task_args"`
	CreatedAt     time.Time      `db:"create_ts"`
	Deadline      time.Time      `db:"deadline_ts"`
	StartedAt     time.Time      `db:"start_ts"`
	EndedAt       sql.NullTime   `db:"end_ts"`
	Changes       []byte         `db:"changes"`
	Comment       string         `db:"comment"`
	Result        sql.NullBool   `db:"result"`
	OperationType string         `db:"operation_type"`
	MetaData      []byte         `db:"metadata"`
}

// CidAndTimestamp cid and timestamp
type CidAndTimestamp struct {
	CID string    `db:"cid"`
	TS  time.Time `db:"ts"`
}

// ClusterInfo ...
type ClusterInfo struct {
	PKey     []byte `db:"pkey"`
	NetID    string `db:"nid"`
	CType    string `db:"ctype"`
	Env      string `db:"env"`
	FolderID int64  `db:"folder_id"`
	Status   string `db:"status"`
}

// ShardInfo ...
type ShardInfo struct {
	SName string `db:"sname"`
}

// Cloud ...
type Cloud struct {
	CloudID       string  `db:"cloud_ext_id"`
	CPUQuota      float64 `db:"cpu_quota"`
	CPUUsed       float64 `db:"cpu_used"`
	MemoryQuota   int64   `db:"memory_quota"`
	MemoryUsed    int64   `db:"memory_used"`
	SSDQuota      int64   `db:"ssd_space_quota"`
	SSDUsed       int64   `db:"ssd_space_used"`
	HDDQuota      int64   `db:"hdd_space_quota"`
	HDDUsed       int64   `db:"hdd_space_used"`
	ClustersQuota int64   `db:"clusters_quota"`
	ClustersUsed  int64   `db:"clusters_used"`
}

type hostRow struct {
	FQDN         string           `db:"fqdn"`
	ClusterID    string           `db:"cid"`
	SubClusterID string           `db:"subcid"`
	ShardID      sql.NullString   `db:"shard_id"`
	Geo          string           `db:"geo"`
	Roles        pgtype.TextArray `db:"roles"`
	CreatedAt    time.Time        `db:"created_at"`
	DiskType     string           `db:"disk_type_ext_id"`
}

type clusterRow struct {
	Name        string             `db:"name"`
	Type        metadb.ClusterType `db:"type"`
	Environment string             `db:"env"`
	Visible     bool               `db:"visible"`
	Status      string             `db:"status"`
}

type workerQueueEventRow struct {
	ID        int64     `db:"event_id"`
	Data      string    `db:"data"`
	CreatedAt time.Time `db:"created_at"`
}

func formatWorkerQueueEvent(r workerQueueEventRow) metadb.WorkerQueueEvent {
	return metadb.WorkerQueueEvent{
		ID:        r.ID,
		Data:      r.Data,
		CreatedAt: r.CreatedAt,
	}
}

// FormatOperation formats db operation model into operation model
func FormatOperation(c Operation) (*metadb.Operation, error) {
	op := &metadb.Operation{
		ID:            c.ID,
		FolderID:      c.FolderID,
		CreatedBy:     c.CreatedBy,
		ClusterID:     c.ClusterID.String,
		WorkerID:      c.WorkerID,
		TaskType:      c.TaskType,
		CreatedAt:     c.CreatedAt,
		Deadline:      c.Deadline,
		StartedAt:     c.StartedAt,
		EndedAt:       c.EndedAt.Time,
		Comment:       c.Comment,
		Result:        c.Result.Bool,
		OperationType: c.OperationType,
		MetaData:      c.MetaData,
	}

	if err := json.Unmarshal(c.Args, &op.Args); err != nil {
		return nil, err
	}

	var changes []map[string]interface{}
	if err := json.Unmarshal(c.Changes, &changes); err != nil {
		return nil, err
	}

	for _, change := range changes {
		var ch metadb.Change
		for k, v := range change {
			switch k {
			case "timestamp":
				ts, err := time.Parse("2006-01-02 15:04:05.000000", v.(string))
				//ts, err := time.Parse(time.RFC3339Nano, v.(string))
				if err != nil {
					return nil, err
				}

				ch.TS = ts
			default:
				ch.What = k
				if v != nil {
					ch.Status = v.(string)
				}
			}
		}

		op.Changes = append(op.Changes, ch)
	}

	return op, nil
}

// FormatCidAndTS formats cid and timestamp from db model to metadb
func FormatCidAndTS(row CidAndTimestamp) metadb.CidAndTimestamp {
	return metadb.CidAndTimestamp{
		ClusterID: row.CID,
		TS:        row.TS,
	}
}

// FormatClusterInfo formats cluster info
func FormatClusterInfo(row ClusterInfo) metadb.ClusterInfo {
	return metadb.ClusterInfo{
		PubKey:   row.PKey,
		NetID:    row.NetID,
		CType:    metadb.ClusterType(row.CType),
		Env:      metadb.Environment(row.Env),
		FolderID: row.FolderID,
		Status:   metadb.ClusterStatus(row.Status),
	}
}

// FormatShardInfo formats shard info
func FormatShardInfo(row ShardInfo) metadb.ShardInfo {
	return metadb.ShardInfo{
		ShardName: row.SName,
	}
}

// FormatCloud format cloud
func FormatCloud(row Cloud) metadb.Cloud {
	return metadb.Cloud{
		CloudID: row.CloudID,
		Used: metadb.Resources{
			CPUCores:      row.CPUUsed,
			MemoryBytes:   row.MemoryUsed,
			SSDBytes:      row.SSDUsed,
			HDDBytes:      row.HDDUsed,
			ClustersCount: row.ClustersUsed,
		},
		Quota: metadb.Resources{
			CPUCores:      row.CPUQuota,
			MemoryBytes:   row.MemoryQuota,
			SSDBytes:      row.SSDQuota,
			HDDBytes:      row.HDDQuota,
			ClustersCount: row.ClustersQuota,
		},
	}
}

func formatHost(r hostRow) metadb.Host {
	ret := metadb.Host{
		FQDN:         r.FQDN,
		ClusterID:    r.ClusterID,
		SubClusterID: r.SubClusterID,
		Geo:          r.Geo,
		CreatedAt:    r.CreatedAt,
		DType:        metadb.DiskType(r.DiskType),
	}
	if r.ShardID.Valid {
		ret.ShardID.Set(r.ShardID.String)
	}
	for _, role := range r.Roles.Elements {
		ret.Roles = append(ret.Roles, role.String)
	}
	return ret
}

func formatCluster(r clusterRow) metadb.Cluster {
	return metadb.Cluster{
		Name:        r.Name,
		Type:        r.Type,
		Environment: r.Environment,
		Visible:     r.Visible,
		Status:      r.Status,
	}
}
