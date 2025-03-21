package filesystem

import (
	"context"
	"math"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func getBlocksCountForSize(size uint64, blockSize uint32) (uint64, error) {
	if blockSize == 0 {
		return 0, errors.CreateInvalidArgumentError(
			"invalid block size %v",
			blockSize,
		)
	}

	if size%uint64(blockSize) != 0 {
		return 0, errors.CreateInvalidArgumentError(
			"invalid size %v for block size %v",
			size,
			blockSize,
		)
	}

	return size / uint64(blockSize), nil
}

func fsKindToString(kind types.FilesystemStorageKind) string {
	switch kind {
	case types.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_SSD:
		return "ssd"
	case types.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_HDD:
		return "hdd"
	}
	return "unknown"
}

func prepareFilesystemKind(kind disk_manager.FilesystemStorageKind) (types.FilesystemStorageKind, error) {
	switch kind {
	case disk_manager.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_UNSPECIFIED:
		fallthrough
	case disk_manager.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_HDD:
		return types.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_HDD, nil
	case disk_manager.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_SSD:
		return types.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_SSD, nil
	default:
		return 0, errors.CreateInvalidArgumentError(
			"unknown filesystem storage kind %v",
			kind,
		)
	}
}

////////////////////////////////////////////////////////////////////////////////

type service struct {
	scheduler tasks.Scheduler
	config    *config.FilesystemConfig
	factory   nfs.Factory
}

func (s *service) CreateFilesystem(
	ctx context.Context,
	req *disk_manager.CreateFilesystemRequest,
) (string, error) {

	if len(req.FilesystemId.ZoneId) == 0 ||
		len(req.FilesystemId.FilesystemId) == 0 {
		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty: %v",
			req.FilesystemId,
		)
	}

	if req.Size < 0 {
		return "", errors.CreateInvalidArgumentError(
			"invalid size: %v",
			req.Size,
		)
	}

	if req.BlockSize < 0 || req.BlockSize > math.MaxUint32 {
		return "", errors.CreateInvalidArgumentError(
			"invalid block size: %v",
			req.BlockSize,
		)
	}

	blockSize := uint32(req.BlockSize)
	if blockSize == 0 {
		blockSize = s.config.GetDefaultBlockSize()
	}

	blocksCount, err := getBlocksCountForSize(uint64(req.Size), blockSize)
	if err != nil {
		return "", err
	}

	kind, err := prepareFilesystemKind(req.StorageKind)
	if err != nil {
		return "", err
	}

	return s.scheduler.ScheduleTask(
		ctx,
		"filesystem.CreateFilesystem",
		"",
		&protos.CreateFilesystemRequest{
			Filesystem: &protos.FilesystemId{
				ZoneId:       req.FilesystemId.ZoneId,
				FilesystemId: req.FilesystemId.FilesystemId,
			},
			CloudId:     req.CloudId,
			FolderId:    req.FolderId,
			BlockSize:   blockSize,
			BlocksCount: blocksCount,
			StorageKind: kind,
		},
		req.CloudId,
		req.FolderId,
	)
}

func (s *service) DeleteFilesystem(
	ctx context.Context,
	req *disk_manager.DeleteFilesystemRequest,
) (string, error) {

	if len(req.FilesystemId.ZoneId) == 0 ||
		len(req.FilesystemId.FilesystemId) == 0 {

		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.scheduler.ScheduleTask(
		ctx,
		"filesystem.DeleteFilesystem",
		"",
		&protos.DeleteFilesystemRequest{
			Filesystem: &protos.FilesystemId{
				ZoneId:       req.FilesystemId.ZoneId,
				FilesystemId: req.FilesystemId.FilesystemId,
			},
		},
		"",
		"",
	)
}

func (s *service) ResizeFilesystem(
	ctx context.Context,
	req *disk_manager.ResizeFilesystemRequest,
) (string, error) {

	if len(req.FilesystemId.ZoneId) == 0 ||
		len(req.FilesystemId.FilesystemId) == 0 ||
		req.Size == 0 {

		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.scheduler.ScheduleTask(
		ctx,
		"filesystem.ResizeFilesystem",
		"",
		&protos.ResizeFilesystemRequest{
			Filesystem: &protos.FilesystemId{
				ZoneId:       req.FilesystemId.ZoneId,
				FilesystemId: req.FilesystemId.FilesystemId,
			},
			Size: uint64(req.Size),
		},
		"",
		"",
	)
}

func (s *service) DescribeFilesystemModel(
	ctx context.Context,
	req *disk_manager.DescribeFilesystemModelRequest,
) (*disk_manager.FilesystemModel, error) {

	var client nfs.Client
	var err error

	if len(req.ZoneId) == 0 {
		client, err = s.factory.CreateClientFromDefaultZone(ctx)
	} else {
		client, err = s.factory.CreateClient(ctx, req.ZoneId)
	}
	if err != nil {
		return nil, err
	}

	if req.Size < 0 {
		return nil, errors.CreateInvalidArgumentError(
			"invalid size: %v",
			req.Size,
		)
	}

	if req.BlockSize < 0 || req.BlockSize > math.MaxUint32 {
		return nil, errors.CreateInvalidArgumentError(
			"invalid block size: %v",
			req.BlockSize,
		)
	}

	blockSize := uint32(req.BlockSize)
	if blockSize == 0 {
		blockSize = s.config.GetDefaultBlockSize()
	}

	kind, err := prepareFilesystemKind(req.StorageKind)
	if err != nil {
		return nil, err
	}

	blocksCount, err := getBlocksCountForSize(uint64(req.Size), blockSize)
	if err != nil {
		return nil, err
	}

	model, err := client.DescribeModel(
		ctx,
		blocksCount,
		uint32(req.BlockSize),
		kind,
	)
	if err != nil {
		return nil, err
	}

	return &disk_manager.FilesystemModel{
		BlockSize:     int64(model.BlockSize),
		Size:          int64(uint64(model.BlockSize) * model.BlocksCount),
		ChannelsCount: int64(model.ChannelsCount),
		StorageKind:   req.StorageKind,
		PerformanceProfile: &disk_manager.FilesystemPerformanceProfile{
			MaxReadBandwidth:  int64(model.PerformanceProfile.MaxReadBandwidth),
			MaxReadIops:       int64(model.PerformanceProfile.MaxReadIops),
			MaxWriteBandwidth: int64(model.PerformanceProfile.MaxWriteBandwidth),
			MaxWriteIops:      int64(model.PerformanceProfile.MaxWriteIops),
		},
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func CreateService(
	taskScheduler tasks.Scheduler,
	config *config.FilesystemConfig,
	factory nfs.Factory,
) Service {

	return &service{
		scheduler: taskScheduler,
		config:    config,
		factory:   factory,
	}
}
