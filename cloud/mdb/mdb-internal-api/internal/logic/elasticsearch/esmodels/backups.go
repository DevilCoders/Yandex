package esmodels

const S3MetaKey = "backups/.yc-metadata.json"

type BackupMeta struct {
	Version   int            `json:"version"`
	Snapshots []SnapshotInfo `json:"snapshots"`
}

type SnapshotInfo struct {
	ID             string   `json:"id"`
	ElasticVersion string   `json:"version"`
	StartTimeMs    int64    `json:"start_time_ms"`
	EndTimeMs      int64    `json:"end_time_ms"`
	IndicesTotal   int      `json:"indices_total"`
	Indices        []string `json:"indices"`
	Size           uint64   `json:"size"`
}
