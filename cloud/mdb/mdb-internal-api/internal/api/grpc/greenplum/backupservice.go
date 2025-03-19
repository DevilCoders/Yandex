package greenplum

import (
	"context"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/core/log"
)

// BackupsService implements gRPC methods for backups management.
type BackupService struct {
	gpv1.UnimplementedBackupServiceServer

	gp greenplum.Greenplum
	L  log.Logger
}

var _ gpv1.BackupServiceServer = &BackupService{}

func NewBackupService(gp greenplum.Greenplum, l log.Logger) *BackupService {
	return &BackupService{gp: gp, L: l}
}

func (bs *BackupService) Get(ctx context.Context, req *gpv1.GetBackupRequest) (*gpv1.Backup, error) {
	backup, err := bs.gp.Backup(ctx, req.GetBackupId())
	if err != nil {
		return nil, err
	}

	return BackupToGRPC(backup), nil
}

func (bs *BackupService) List(ctx context.Context, req *gpv1.ListBackupsRequest) (*gpv1.ListBackupsResponse, error) {
	var pageToken backups.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &gpv1.ListBackupsResponse{}, err
	}

	backups, newPageToken, err := bs.gp.FolderBackups(ctx, req.FolderId, pageToken, req.GetPageSize())
	if err != nil {
		return &gpv1.ListBackupsResponse{}, err
	}

	nextToken, err := api.BuildPageTokenToGRPC(newPageToken, false)
	if err != nil {
		return &gpv1.ListBackupsResponse{}, err
	}

	return &gpv1.ListBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextToken,
	}, nil
}
