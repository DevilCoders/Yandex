package redis

import (
	"context"

	redisv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/redis/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/core/log"
)

// BackupService implements backup-specific gRPC methods
type BackupService struct {
	redisv1.UnimplementedBackupServiceServer

	l     log.Logger
	redis redis.Redis
}

var _ redisv1.BackupServiceServer = &BackupService{}

func NewBackupService(redis redis.Redis, l log.Logger) *BackupService {
	return &BackupService{redis: redis, l: l}
}

func (bs *BackupService) Get(ctx context.Context, req *redisv1.GetBackupRequest) (*redisv1.Backup, error) {
	backup, err := bs.redis.Backup(ctx, req.GetBackupId())
	if err != nil {
		return nil, err
	}

	return BackupToGRPC(backup), nil
}

func (bs *BackupService) List(ctx context.Context, req *redisv1.ListBackupsRequest) (*redisv1.ListBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &redisv1.ListBackupsResponse{}, err
	}

	backups, backupPageToken, err := bs.redis.FolderBackups(ctx, req.FolderId, pageToken, req.GetPageSize())
	if err != nil {
		return &redisv1.ListBackupsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(backupPageToken, false)
	if err != nil {
		return &redisv1.ListBackupsResponse{}, err
	}

	return &redisv1.ListBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextPageToken,
	}, nil
}
