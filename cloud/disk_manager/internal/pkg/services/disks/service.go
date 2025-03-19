package disks

import (
	"context"
	"math"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	disks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func diskKindToString(kind types.DiskKind) string {
	switch kind {
	case types.DiskKind_DISK_KIND_SSD:
		return "ssd"
	case types.DiskKind_DISK_KIND_HDD:
		return "hdd"
	case types.DiskKind_DISK_KIND_SSD_NONREPLICATED:
		return "ssd-nonreplicated"
	case types.DiskKind_DISK_KIND_SSD_MIRROR2:
		return "ssd-mirror2"
	case types.DiskKind_DISK_KIND_SSD_MIRROR3:
		return "ssd-mirror3"
	case types.DiskKind_DISK_KIND_SSD_LOCAL:
		return "ssd-local"
	}
	return "unknown"
}

func prepareDiskKind(kind disk_manager.DiskKind) (types.DiskKind, error) {
	switch kind {
	case disk_manager.DiskKind_DISK_KIND_UNSPECIFIED:
		return 0, errors.CreateInvalidArgumentError("disk kind is required")
	case disk_manager.DiskKind_DISK_KIND_SSD:
		return types.DiskKind_DISK_KIND_SSD, nil
	case disk_manager.DiskKind_DISK_KIND_HDD:
		return types.DiskKind_DISK_KIND_HDD, nil
	case disk_manager.DiskKind_DISK_KIND_SSD_NONREPLICATED:
		return types.DiskKind_DISK_KIND_SSD_NONREPLICATED, nil
	case disk_manager.DiskKind_DISK_KIND_SSD_MIRROR2:
		return types.DiskKind_DISK_KIND_SSD_MIRROR2, nil
	case disk_manager.DiskKind_DISK_KIND_SSD_MIRROR3:
		return types.DiskKind_DISK_KIND_SSD_MIRROR3, nil
	case disk_manager.DiskKind_DISK_KIND_SSD_LOCAL:
		return types.DiskKind_DISK_KIND_SSD_LOCAL, nil
	default:
		return 0, errors.CreateInvalidArgumentError(
			"unknown disk kind %v",
			kind,
		)
	}
}

func prepareEncryptionMode(
	mode disk_manager.EncryptionMode,
) (types.EncryptionMode, error) {
	switch mode {
	case disk_manager.EncryptionMode_NO_ENCRYPTION:
		return types.EncryptionMode_NO_ENCRYPTION, nil
	case disk_manager.EncryptionMode_ENCRYPTION_AES_XTS:
		return types.EncryptionMode_ENCRYPTION_AES_XTS, nil
	default:
		return 0, errors.CreateInvalidArgumentError(
			"unknown encryption mode %v",
			mode,
		)
	}
}

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

////////////////////////////////////////////////////////////////////////////////

type service struct {
	taskScheduler tasks.Scheduler
	config        *disks_config.DisksConfig
	nbsFactory    nbs.Factory
	poolService   pools.Service
}

func (s *service) prepareCreateDiskParams(
	req *disk_manager.CreateDiskRequest,
) (*protos.CreateDiskParams, error) {

	if len(req.DiskId.ZoneId) == 0 ||
		len(req.DiskId.DiskId) == 0 {

		return nil, errors.CreateInvalidArgumentError(
			"invalid disk id: %v",
			req.DiskId,
		)
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

	blocksCount, err := getBlocksCountForSize(uint64(req.Size), blockSize)
	if err != nil {
		return nil, err
	}

	kind, err := prepareDiskKind(req.Kind)
	if err != nil {
		return nil, err
	}

	encryptionMode, err := prepareEncryptionMode(req.EncryptionMode)
	if err != nil {
		return nil, err
	}

	if req.TabletVersion < 0 || req.TabletVersion > math.MaxUint32 {
		return nil, errors.CreateInvalidArgumentError(
			"invalid tablet version: %v",
			req.TabletVersion,
		)
	}

	tabletVersion := uint32(req.TabletVersion)

	return &protos.CreateDiskParams{
		BlocksCount: blocksCount,
		Disk: &types.Disk{
			ZoneId: req.DiskId.ZoneId,
			DiskId: req.DiskId.DiskId,
		},
		BlockSize:             blockSize,
		Kind:                  kind,
		CloudId:               req.CloudId,
		FolderId:              req.FolderId,
		TabletVersion:         tabletVersion,
		PlacementGroupId:      req.PlacementGroupId,
		EncryptionMode:        encryptionMode,
		EncryptionKeyFilePath: req.EncryptionKeyFilePath,
		StoragePoolName:       req.StoragePoolName,
		AgentIds:              req.AgentIds,
	}, nil
}

func (s *service) isOverlayDiskAllowed(
	ctx context.Context,
	req *disk_manager.CreateDiskRequest,
	srcImageID string,
	params *protos.CreateDiskParams,
) (bool, error) {

	technicallyImpossible := params.BlockSize != 4096 ||
		params.Kind == types.DiskKind_DISK_KIND_SSD_NONREPLICATED ||
		params.Kind == types.DiskKind_DISK_KIND_SSD_MIRROR2 ||
		params.Kind == types.DiskKind_DISK_KIND_SSD_MIRROR3 ||
		params.Kind == types.DiskKind_DISK_KIND_SSD_LOCAL ||
		req.Size > 4<<40 // 4 TB
	if technicallyImpossible {
		return false, nil
	}

	if req.ForceNotLayered {
		return false, nil
	}

	configured, err := s.poolService.IsPoolConfigured(
		ctx,
		srcImageID,
		params.Disk.ZoneId,
	)
	if err != nil {
		return false, err
	}

	if !configured {
		return false, nil
	}

	if common.Find(s.config.GetOverlayDisksFolderIdWhitelist(), params.FolderId) {
		return true, nil
	}

	if common.Find(s.config.GetOverlayDisksFolderIdBlacklist(), params.FolderId) {
		return false, nil
	}

	return !s.config.GetDisableOverlayDisks(), nil
}

func (s *service) CreateDisk(
	ctx context.Context,
	req *disk_manager.CreateDiskRequest,
) (string, error) {

	params, err := s.prepareCreateDiskParams(req)
	if err != nil {
		return "", err
	}

	switch src := req.Src.(type) {
	case *disk_manager.CreateDiskRequest_SrcEmpty:
		return s.taskScheduler.ScheduleTask(
			ctx,
			"disks.CreateEmptyDisk",
			"",
			params,
			params.CloudId,
			params.FolderId,
		)
	case *disk_manager.CreateDiskRequest_SrcImageId:
		allowed, err := s.isOverlayDiskAllowed(ctx, req, src.SrcImageId, params)
		if err != nil {
			return "", err
		}

		if allowed {
			return s.taskScheduler.ScheduleTask(
				ctx,
				"disks.CreateOverlayDisk",
				"",
				&protos.CreateOverlayDiskRequest{
					SrcImageId: src.SrcImageId,
					Params:     params,
				},
				params.CloudId,
				params.FolderId,
			)
		}

		return s.taskScheduler.ScheduleTask(
			ctx,
			"disks.CreateDiskFromImage",
			"",
			&protos.CreateDiskFromImageRequest{
				SrcImageId:                          src.SrcImageId,
				Params:                              params,
				OperationCloudId:                    req.OperationCloudId,
				OperationFolderId:                   req.OperationFolderId,
				UseDataplaneTasksForLegacySnapshots: s.config.GetUseDataplaneTasksForLegacySnapshots(),
			},
			req.OperationCloudId,
			req.OperationFolderId,
		)
	case *disk_manager.CreateDiskRequest_SrcSnapshotId:
		return s.taskScheduler.ScheduleTask(
			ctx,
			"disks.CreateDiskFromSnapshot",
			"",
			&protos.CreateDiskFromSnapshotRequest{
				SrcSnapshotId:                       src.SrcSnapshotId,
				Params:                              params,
				OperationCloudId:                    req.OperationCloudId,
				OperationFolderId:                   req.OperationFolderId,
				UseDataplaneTasksForLegacySnapshots: s.config.GetUseDataplaneTasksForLegacySnapshots(),
			},
			req.OperationCloudId,
			req.OperationFolderId,
		)
	default:
		return "", errors.CreateInvalidArgumentError("unknown src %s", src)
	}
}

func (s *service) DeleteDisk(
	ctx context.Context,
	req *disk_manager.DeleteDiskRequest,
) (string, error) {

	if len(req.DiskId.ZoneId) == 0 || len(req.DiskId.DiskId) == 0 {
		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"disks.DeleteDisk",
		"",
		&protos.DeleteDiskRequest{
			Disk: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
		},
		"",
		"",
	)
}

func (s *service) ResizeDisk(
	ctx context.Context,
	req *disk_manager.ResizeDiskRequest,
) (string, error) {

	if len(req.DiskId.ZoneId) == 0 || len(req.DiskId.DiskId) == 0 {
		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	if req.Size < 0 {
		return "", errors.CreateInvalidArgumentError(
			"invalid size: %v",
			req.Size,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"disks.ResizeDisk",
		"",
		&protos.ResizeDiskRequest{
			Disk: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
			Size: uint64(req.Size),
		},
		"",
		"",
	)
}

func (s *service) AlterDisk(
	ctx context.Context,
	req *disk_manager.AlterDiskRequest,
) (string, error) {

	if len(req.DiskId.ZoneId) == 0 || len(req.DiskId.DiskId) == 0 {
		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"disks.AlterDisk",
		"",
		&protos.AlterDiskRequest{
			Disk: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
			CloudId:  req.CloudId,
			FolderId: req.FolderId,
		},
		req.CloudId,
		req.FolderId,
	)
}

func (s *service) AssignDisk(
	ctx context.Context,
	req *disk_manager.AssignDiskRequest,
) (string, error) {

	if len(req.DiskId.ZoneId) == 0 || len(req.DiskId.DiskId) == 0 {
		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"disks.AssignDisk",
		"",
		&protos.AssignDiskRequest{
			Disk: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
			InstanceId: req.InstanceId,
			Host:       req.Host,
			Token:      req.Token,
		},
		"",
		"",
	)
}

func (s *service) UnassignDisk(
	ctx context.Context,
	req *disk_manager.UnassignDiskRequest,
) (string, error) {

	if len(req.DiskId.ZoneId) == 0 || len(req.DiskId.DiskId) == 0 {
		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"disks.UnassignDisk",
		"",
		&protos.UnassignDiskRequest{
			Disk: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
		},
		"",
		"",
	)
}

func (s *service) DescribeDiskModel(
	ctx context.Context,
	req *disk_manager.DescribeDiskModelRequest,
) (*disk_manager.DiskModel, error) {

	var client nbs.Client
	var err error

	if len(req.ZoneId) == 0 {
		client, err = s.nbsFactory.GetClientFromDefaultZone(ctx)
	} else {
		client, err = s.nbsFactory.GetClient(ctx, req.ZoneId)
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

	if req.TabletVersion < 0 || req.TabletVersion > math.MaxUint32 {
		return nil, errors.CreateInvalidArgumentError(
			"invalid tablet version: %v",
			req.TabletVersion,
		)
	}

	kind, err := prepareDiskKind(req.Kind)
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
		uint32(req.TabletVersion),
	)
	if err != nil {
		return nil, err
	}

	return &disk_manager.DiskModel{
		BlockSize:     int64(model.BlockSize),
		Size:          int64(uint64(model.BlockSize) * model.BlocksCount),
		ChannelsCount: int64(model.ChannelsCount),
		Kind:          req.Kind,
		PerformanceProfile: &disk_manager.DiskPerformanceProfile{
			MaxReadBandwidth:   int64(model.PerformanceProfile.MaxReadBandwidth),
			MaxPostponedWeight: int64(model.PerformanceProfile.MaxPostponedWeight),
			ThrottlingEnabled:  model.PerformanceProfile.ThrottlingEnabled,
			MaxReadIops:        int64(model.PerformanceProfile.MaxReadIops),
			BoostTime:          int64(model.PerformanceProfile.BoostTime),
			BoostRefillTime:    int64(model.PerformanceProfile.BoostRefillTime),
			BoostPercentage:    int64(model.PerformanceProfile.BoostPercentage),
			MaxWriteBandwidth:  int64(model.PerformanceProfile.MaxWriteBandwidth),
			MaxWriteIops:       int64(model.PerformanceProfile.MaxWriteIops),
			BurstPercentage:    int64(model.PerformanceProfile.BurstPercentage),
		},
		MergedChannelsCount: int64(model.MergedChannelsCount),
		MixedChannelsCount:  int64(model.MixedChannelsCount),
	}, nil
}

func (s *service) StatDisk(
	ctx context.Context,
	req *disk_manager.StatDiskRequest,
) (*disk_manager.DiskStats, error) {

	if len(req.DiskId.ZoneId) == 0 ||
		len(req.DiskId.DiskId) == 0 {

		return nil, errors.CreateInvalidArgumentError(
			"invalid disk id: %v",
			req.DiskId,
		)
	}

	client, err := s.nbsFactory.GetClient(ctx, req.DiskId.ZoneId)
	if err != nil {
		return nil, err
	}

	stats, err := client.Stat(ctx, req.DiskId.DiskId)
	if err != nil {
		return nil, err
	}

	return &disk_manager.DiskStats{StorageSize: int64(stats.StorageSize)}, nil
}

func (s *service) MigrateDisk(
	ctx context.Context,
	req *disk_manager.MigrateDiskRequest,
) (string, error) {

	if len(req.DiskId.ZoneId) == 0 || len(req.DiskId.DiskId) == 0 ||
		len(req.DstZoneId) == 0 {

		return "", errors.CreateInvalidArgumentError(
			"some of parameters are empty, req=%v",
			req,
		)
	}
	if req.DiskId.ZoneId == req.DstZoneId {
		return "", errors.CreateInvalidArgumentError(
			"cannot migrate disk to the same zone, req=%v",
			req,
		)
	}

	return s.taskScheduler.ScheduleTask(
		ctx,
		"disks.MigrateDisk",
		"",
		&protos.MigrateDiskRequest{
			Disk: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
			DstZoneId: req.DstZoneId,
		},
		"",
		"",
	)
}

func (s *service) SendMigrationSignal(
	_ context.Context,
	_ *disk_manager.SendMigrationSignalRequest,
) error {

	return errors.New("not implemented")
}

////////////////////////////////////////////////////////////////////////////////

func CreateService(
	taskScheduler tasks.Scheduler,
	config *disks_config.DisksConfig,
	nbsFactory nbs.Factory,
	poolService pools.Service,
) Service {

	return &service{
		taskScheduler: taskScheduler,
		config:        config,
		nbsFactory:    nbsFactory,
		poolService:   poolService,
	}
}
