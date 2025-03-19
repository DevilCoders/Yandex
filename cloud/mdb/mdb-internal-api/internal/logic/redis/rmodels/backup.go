package rmodels

import (
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	WalgSentinelSuffix  = "_backup_stop_sentinel.json"
	BackupPathPrefix    = "redis-backup"
	BaseBackupDir       = "basebackups_005"
	BackupPathDelimiter = "/"
)

func IsWalgSentinelFilename(key string) bool {
	return strings.HasSuffix(key, WalgSentinelSuffix)
}

type BackupMeta struct {
	BackupName      string
	StartLocalTime  time.Time
	FinishLocalTime time.Time
	UserData        UserData
}

type UserData struct {
	BackupID  string `json:"backup_id"`
	ShardName string `json:"shard_name"`
}

type ShardData struct {
	Cid     string
	ShardID string
	Name    string
}

func GetBackupShardPrefix(shardPrefix string) string {
	return fmt.Sprintf("%s%s%s", shardPrefix, BaseBackupDir, BackupPathDelimiter)
}

func (sd *ShardData) GetBackupShardData(key string) error {
	parts := strings.Split(key, BackupPathDelimiter)
	if len(parts) != 5 {
		return xerrors.Errorf("backup key malformed, expected X/X/X/X/X-like: %+v", key)
	}
	sd.Cid = parts[1]
	sd.ShardID = parts[2]
	sd.Name = strings.TrimSuffix(parts[4], WalgSentinelSuffix)
	return nil
}

func GetBackupCidPrefix(cid string) string {
	parts := []string{BackupPathPrefix, cid}
	return strings.Join(parts, BackupPathDelimiter) + BackupPathDelimiter
}

func BackupFromBackupMeta(backupMeta BackupMeta, shardData ShardData) (bmodels.Backup, error) {
	fmt.Printf("ZZZ:meta:%+v\n", backupMeta)

	var createdAt, startedAt time.Time
	startedAt = backupMeta.StartLocalTime
	createdAt = backupMeta.FinishLocalTime
	shardNames := []string{backupMeta.UserData.ShardName}
	name := shardData.Name
	if backupMeta.BackupName != "" {
		name = backupMeta.BackupName
	}
	id := fmt.Sprintf("%s:%s", shardData.ShardID, name)

	backup := bmodels.Backup{
		ID:               id,
		SourceClusterID:  shardData.Cid,
		SourceShardNames: shardNames,
		StartedAt:        optional.NewTime(startedAt),
		CreatedAt:        createdAt,
	}

	return backup, nil
}

func DecodeGlobalBackupID(globalBackupID string) (string, string, error) {
	parts := strings.Split(globalBackupID, ":")
	if len(parts) != 3 {
		return "", "", semerr.InvalidInputf("malformed backup id, awaited X:X:X-like: %s", globalBackupID)
	}
	bID := strings.Join(parts[1:], ":")
	return parts[0], bID, nil
}
