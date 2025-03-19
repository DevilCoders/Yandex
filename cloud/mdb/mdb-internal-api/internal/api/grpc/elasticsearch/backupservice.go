package elasticsearch

import (
	"context"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/core/log"
)

// BackupsService implements gRPC methods for backups management.
type BackupService struct {
	esv1.UnimplementedBackupServiceServer

	es elasticsearch.ElasticSearch
	L  log.Logger
}

var _ esv1.BackupServiceServer = &BackupService{}

func NewBackupService(es elasticsearch.ElasticSearch, l log.Logger) *BackupService {
	return &BackupService{es: es, L: l}
}

func (bs *BackupService) Get(ctx context.Context, req *esv1.GetBackupRequest) (*esv1.Backup, error) {
	backup, err := bs.es.Backup(ctx, req.GetBackupId())
	if err != nil {
		return nil, err
	}

	return BackupToGRPC(backup), nil
}

func (bs *BackupService) List(ctx context.Context, req *esv1.ListBackupsRequest) (*esv1.ListBackupsResponse, error) {
	var pageToken backups.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPageToken(), &pageToken)
	if err != nil {
		return &esv1.ListBackupsResponse{}, err
	}

	backups, newPageToken, err := bs.es.FolderBackups(ctx, req.FolderId, pageToken, req.GetPageSize())
	if err != nil {
		return &esv1.ListBackupsResponse{}, err
	}

	nextToken, err := api.BuildPageTokenToGRPC(newPageToken, false)
	if err != nil {
		return &esv1.ListBackupsResponse{}, err
	}

	return &esv1.ListBackupsResponse{
		Backups:       BackupsToGRPC(backups),
		NextPageToken: nextToken,
	}, nil
}
