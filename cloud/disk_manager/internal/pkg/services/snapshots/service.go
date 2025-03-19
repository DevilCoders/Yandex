package snapshots

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	transfer_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type service struct {
	taskScheduler   tasks.Scheduler
	config          *config.SnapshotsConfig
	transferService transfer.Service
}

func (s *service) CreateSnapshot(
	ctx context.Context,
	req *disk_manager.CreateSnapshotRequest,
) (string, error) {

	if len(req.Src.ZoneId) == 0 ||
		len(req.Src.DiskId) == 0 ||
		len(req.SnapshotId) == 0 {

		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	useDataplaneTasks := s.config.GetUseDataplaneTasks() ||
		common.Find(s.config.GetUseDataplaneTasksForFolder(), req.FolderId)
	incremental := (s.config.GetEnableIncrementality() && useDataplaneTasks) || req.Incremental
	useS3 := common.Find(s.config.GetUseS3ForFolder(), req.FolderId)

	return s.taskScheduler.ScheduleTask(
		ctx,
		"snapshots.CreateSnapshotFromDisk",
		"",
		&protos.CreateSnapshotFromDiskRequest{
			SrcDisk: &types.Disk{
				ZoneId: req.Src.ZoneId,
				DiskId: req.Src.DiskId,
			},
			DstSnapshotId:     req.SnapshotId,
			FolderId:          req.FolderId,
			Incremental:       incremental,
			OperationCloudId:  req.OperationCloudId,
			OperationFolderId: req.OperationFolderId,
			UseDataplaneTasks: useDataplaneTasks,
			UseS3:             useS3,
		},
		req.OperationCloudId,
		req.OperationFolderId,
	)
}

func (s *service) DeleteSnapshot(
	ctx context.Context,
	req *disk_manager.DeleteSnapshotRequest,
) (string, error) {

	if len(req.SnapshotId) == 0 {
		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"snapshots.DeleteSnapshot",
		"",
		&protos.DeleteSnapshotRequest{
			SnapshotId:        req.SnapshotId,
			OperationCloudId:  req.OperationCloudId,
			OperationFolderId: req.OperationFolderId,
		},
		req.OperationCloudId,
		req.OperationFolderId,
	)
}

func (s *service) RestoreDiskFromSnapshot(
	ctx context.Context,
	req *disk_manager.RestoreDiskFromSnapshotRequest,
) (string, error) {

	return s.transferService.TransferFromSnapshotToDisk(
		ctx,
		&transfer_protos.TransferFromSnapshotToDiskRequest{
			SrcSnapshotId: req.SnapshotId,
			Dst: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
			OperationCloudId:  req.OperationCloudId,
			OperationFolderId: req.OperationFolderId,
		},
	)
}

////////////////////////////////////////////////////////////////////////////////

func CreateService(
	taskScheduler tasks.Scheduler,
	config *config.SnapshotsConfig,
	transferService transfer.Service,
) Service {

	return &service{
		taskScheduler:   taskScheduler,
		config:          config,
		transferService: transferService,
	}
}
