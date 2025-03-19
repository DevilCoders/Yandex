package transfer

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type service struct {
	taskScheduler tasks.Scheduler
}

func (s *service) TransferFromImageToDisk(
	ctx context.Context,
	req *protos.TransferFromImageToDiskRequest,
) (string, error) {

	if len(req.SrcImageId) == 0 ||
		len(req.Dst.ZoneId) == 0 ||
		len(req.Dst.DiskId) == 0 {

		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"transfer.TransferFromImageToDisk",
		"",
		req,
		req.OperationCloudId,
		req.OperationFolderId,
	)
}

func (s *service) TransferFromSnapshotToDisk(
	ctx context.Context,
	req *protos.TransferFromSnapshotToDiskRequest,
) (string, error) {

	if len(req.SrcSnapshotId) == 0 ||
		len(req.Dst.ZoneId) == 0 ||
		len(req.Dst.DiskId) == 0 {

		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"transfer.TransferFromSnapshotToDisk",
		"",
		req,
		req.OperationCloudId,
		req.OperationFolderId,
	)
}

////////////////////////////////////////////////////////////////////////////////

func CreateService(
	taskScheduler tasks.Scheduler,
) Service {

	return &service{
		taskScheduler: taskScheduler,
	}
}
