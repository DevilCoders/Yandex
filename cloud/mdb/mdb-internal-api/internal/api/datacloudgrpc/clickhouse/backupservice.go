package clickhouse

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
)

// BackupService implements DB-specific gRPC methods
type BackupService struct {
	chv1.UnimplementedBackupServiceServer

	cs *grpcapi.ClusterService
	ch clickhouse.ClickHouse
}

var _ chv1.BackupServiceServer = &BackupService{}

func NewBackupService(cs *grpcapi.ClusterService, ch clickhouse.ClickHouse) *BackupService {
	return &BackupService{
		cs: cs,
		ch: ch,
	}
}

func (bs *BackupService) Get(ctx context.Context, req *chv1.GetBackupRequest) (*chv1.Backup, error) {
	backup, err := bs.ch.Backup(ctx, req.GetBackupId())
	if err != nil {
		return nil, err
	}

	return BackupToGRPC(backup), err
}

func (bs *BackupService) List(ctx context.Context, req *chv1.ListBackupsRequest) (*chv1.ListBackupsResponse, error) {
	var pageToken bmodels.BackupsPageToken
	err := api.ParsePageTokenFromGRPC(req.GetPaging().GetPageToken(), &pageToken)
	if err != nil {
		return &chv1.ListBackupsResponse{}, err
	}

	backups, newToken, err := bs.ch.FolderBackups(ctx, req.ProjectId, pageToken, req.GetPaging().GetPageSize())
	if err != nil {
		return nil, err
	}

	grpcToken, err := api.BuildPageTokenToGRPC(newToken, false)
	if err != nil {
		return nil, err
	}

	return &chv1.ListBackupsResponse{
		Backups:  BackupsToGRPC(backups),
		NextPage: &apiv1.NextPage{Token: grpcToken},
	}, err
}

func (bs *BackupService) Delete(ctx context.Context, req *chv1.DeleteBackupRequest) (*chv1.DeleteBackupResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Delete not implemented")
}
