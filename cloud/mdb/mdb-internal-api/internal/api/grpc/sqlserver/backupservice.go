package sqlserver

import (
	"context"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/core/log"
)

// BackupService implements gRPC methods for user management.
type BackupService struct {
	ssv1.UnimplementedBackupServiceServer

	SQLServer sqlserver.SQLServer
	L         log.Logger
}

var _ ssv1.BackupServiceServer = &BackupService{}

// Returns the specified SQLServer backup
func (us *BackupService) Get(ctx context.Context, req *ssv1.GetBackupRequest) (*ssv1.Backup, error) {
	backup, err := us.SQLServer.BackupByGlobalID(ctx, req.GetBackupId())
	if err != nil {
		return nil, err
	}

	return backupToGRPC(backup), nil
}

// Returns backup listing for all visible and deleted clusters in folder
func (us *BackupService) List(ctx context.Context, req *ssv1.ListBackupsRequest) (*ssv1.ListBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return nil, err
	}
	backups, nextPageToken, err := us.SQLServer.ListBackupsInFolder(ctx, req.GetFolderId(), pageToken, req.GetPageSize())
	if err != nil {
		return nil, err
	}
	nextToken, err := api.BuildPageTokenToGRPC(nextPageToken, false)
	if err != nil {
		return nil, err
	}
	return &ssv1.ListBackupsResponse{
		Backups:       backupsToGRPC(backups),
		NextPageToken: nextToken,
	}, nil
}
