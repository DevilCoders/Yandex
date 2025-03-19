package mysql

import (
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
)

type MySQLMetadata struct {
	BackupName string          `json:"name,omitempty"`
	BackupID   string          `json:"id,omitempty"`
	StartTime  time.Time       `json:"start_time,omitempty"`
	FinishTime time.Time       `json:"finish_time,omitempty"`
	DataSize   int64           `json:"compressed_size,omitempty"`
	RootPath   string          `json:"root_path,omitempty"`
	RawMeta    json.RawMessage `json:"meta,omitempty"`
}

func NewMySQLMetadata(backupName, rootPath string, metadata StreamSentinelDto, rawMeta []byte,
	sentinel StreamSentinelDto) (MySQLMetadata, error) {
	return MySQLMetadata{
		BackupName: backupName,
		BackupID:   metadata.UserData.BackupID,
		StartTime:  metadata.StartLocalTime,
		FinishTime: metadata.StopLocalTime,
		DataSize:   metadata.CompressedSize,
		RawMeta:    rawMeta,
		RootPath:   rootPath,
	}, nil
}

var _ metadb.BackupMetadata = MySQLMetadata{}

func UnmarshalMySQLMetadata(raw []byte) (MySQLMetadata, error) {
	var meta MySQLMetadata
	if err := json.Unmarshal(raw, &meta); err != nil {
		return MySQLMetadata{}, err
	}
	return meta, nil
}

func (meta MySQLMetadata) ID() string {
	return meta.BackupID
}

func (meta MySQLMetadata) Marshal() ([]byte, error) {
	return json.Marshal(meta)
}

func (meta MySQLMetadata) String() string {
	raw, err := meta.Marshal()
	if err != nil {
		return fmt.Sprintf("can not marshal mysql metadata with error %v: %+v", err, raw)
	}
	return string(raw)
}

type MySQLUserDataMeta struct {
	BackupID string `json:"backup_id"`
}

// StreamSentinelDto describes file structure of json sentinel
// https://github.com/wal-g/wal-g/blob/8a211e8f6a92bc676703de15cc8c6a0b1b48be63/internal/backup_sentinel_dto.go#L9
// https://github.com/wal-g/wal-g/blob/f1329093d7815464425b6c0f567b24515d21e119/internal/databases/mysql/mysql.go#L138
type StreamSentinelDto struct {
	BinLogStart    string    `json:"BinLogStart,omitempty"`
	BinLogEnd      string    `json:"BinLogEnd,omitempty"`
	StartLocalTime time.Time `json:"StartLocalTime,omitempty"`
	StopLocalTime  time.Time `json:"StopLocalTime,omitempty"`

	UncompressedSize int64  `json:"UncompressedSize,omitempty"`
	CompressedSize   int64  `json:"CompressedSize,omitempty"`
	Hostname         string `json:"Hostname,omitempty"`

	IsPermanent bool              `json:"IsPermanent,omitempty"`
	UserData    MySQLUserDataMeta `json:"UserData,omitempty"`
	//todo: add other fields from internal.GenericMetadata
}

func UnmarshalStreamSentinelDto(raw []byte) (StreamSentinelDto, error) {
	var sentinel StreamSentinelDto
	if err := json.Unmarshal(raw, &sentinel); err != nil {
		return StreamSentinelDto{}, err
	}
	return sentinel, nil
}
