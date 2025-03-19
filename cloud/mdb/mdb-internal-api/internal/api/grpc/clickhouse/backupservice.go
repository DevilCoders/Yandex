package clickhouse

import (
	"context"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/core/log"
)

// BackupService implements backup-specific gRPC methods
type BackupService struct {
	chv1.UnimplementedBackupServiceServer

	l  log.Logger
	ch clickhouse.ClickHouse
}

var _ chv1.BackupServiceServer = &BackupService{}

func NewBackupService(ch clickhouse.ClickHouse, l log.Logger) *BackupService {
	return &BackupService{ch: ch, l: l}
}

func (bs *BackupService) Get(ctx context.Context, req *chv1.GetBackupRequest) (*chv1.Backup, error) {
	backup, err := bs.ch.Backup(ctx, req.GetBackupId())
	if err != nil {
		return nil, err
	}

	return BackupToGRPC(backup), nil
}

func (bs *BackupService) List(ctx context.Context, req *chv1.ListBackupsRequest) (*chv1.ListBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListBackupsResponse{}, err
	}

	backups, backupPageToken, err := bs.ch.FolderBackups(ctx, req.FolderId, pageToken, req.GetPageSize())
	if err != nil {
		return &chv1.ListBackupsResponse{}, err
	}

	nextPageToken, err := api.BuildPageTokenToGRPC(backupPageToken, false)
	if err != nil {
		return &chv1.ListBackupsResponse{}, err
	}

	return &chv1.ListBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextPageToken,
	}, nil
}
