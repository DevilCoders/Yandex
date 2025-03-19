package nbs

import (
	"context"
	"encoding/json"
	"fmt"
	"hash/crc32"
	"strings"
	"sync"
	"time"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"

	private_protos "a.yandex-team.ru/cloud/blockstore/private/api/protos"
	protos "a.yandex-team.ru/cloud/blockstore/public/api/protos"
	nbs_client "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
	"a.yandex-team.ru/cloud/disk_manager/pkg/client/codes"
	coreprotos "a.yandex-team.ru/cloud/storage/core/protos"
)

////////////////////////////////////////////////////////////////////////////////

const (
	maxConsecutiveRetries            = 3
	maxChangedBlockCountPerIteration = 1 << 20
	blockCountToSaveStateThreshold   = 20 * maxChangedBlockCountPerIteration
)

////////////////////////////////////////////////////////////////////////////////

func highestBitPosition(b byte) int {
	i := 0
	for ; b != 0; b = b >> 1 {
		i++
	}
	return i
}

////////////////////////////////////////////////////////////////////////////////

func setProtoFlag(currentFlags uint32, flag protos.EMountFlag) uint32 {
	return currentFlags | (1 << (flag - 1))
}

func protoFlags(flag protos.EMountFlag) uint32 {
	return setProtoFlag(0, flag)
}

////////////////////////////////////////////////////////////////////////////////

func getStorageMediaKind(
	diskKind types.DiskKind,
) (coreprotos.EStorageMediaKind, error) {

	switch diskKind {
	case types.DiskKind_DISK_KIND_SSD:
		return coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD, nil
	case types.DiskKind_DISK_KIND_HDD:
		return coreprotos.EStorageMediaKind_STORAGE_MEDIA_HYBRID, nil
	case types.DiskKind_DISK_KIND_SSD_NONREPLICATED:
		return coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_NONREPLICATED, nil
	case types.DiskKind_DISK_KIND_SSD_MIRROR2:
		return coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_MIRROR2, nil
	case types.DiskKind_DISK_KIND_SSD_MIRROR3:
		return coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_MIRROR3, nil
	case types.DiskKind_DISK_KIND_SSD_LOCAL:
		return coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_LOCAL, nil
	default:
		return 0, errors.CreateInvalidArgumentError(
			"unknown disk kind %v",
			diskKind,
		)
	}
}

func getEncryptionMode(
	encryptionMode types.EncryptionMode,
) (protos.EEncryptionMode, error) {

	switch encryptionMode {
	case types.EncryptionMode_NO_ENCRYPTION:
		return protos.EEncryptionMode_NO_ENCRYPTION, nil
	case types.EncryptionMode_ENCRYPTION_AES_XTS:
		return protos.EEncryptionMode_ENCRYPTION_AES_XTS, nil
	default:
		return 0, errors.CreateInvalidArgumentError(
			"unknown encryption mode %v",
			encryptionMode,
		)
	}
}

func toPlacementStrategy(
	placementStrategy types.PlacementStrategy,
) (protos.EPlacementStrategy, error) {

	switch placementStrategy {
	case types.PlacementStrategy_PLACEMENT_STRATEGY_SPREAD:
		return protos.EPlacementStrategy_PLACEMENT_STRATEGY_SPREAD, nil
	default:
		return 0, errors.CreateInvalidArgumentError(
			"unknown placement strategy %v",
			placementStrategy,
		)
	}
}

func fromPlacementStrategy(
	placementStrategy protos.EPlacementStrategy,
) (types.PlacementStrategy, error) {

	switch placementStrategy {
	case protos.EPlacementStrategy_PLACEMENT_STRATEGY_SPREAD:
		return types.PlacementStrategy_PLACEMENT_STRATEGY_SPREAD, nil
	default:
		return 0, errors.CreateInvalidArgumentError(
			"unknown placement strategy %v",
			placementStrategy,
		)
	}
}

////////////////////////////////////////////////////////////////////////////////

func wrapError(e error) error {
	if IsNotFoundError(e) {
		return &errors.NonRetriableError{Err: e, Silent: true}
	}

	var clientErr *nbs_client.ClientError
	if errors.As(e, &clientErr) {
		if clientErr.IsRetriable() ||
			clientErr.Code == nbs_client.E_CANCELLED ||
			clientErr.Code == nbs_client.E_INVALID_SESSION ||
			clientErr.Code == nbs_client.E_MOUNT_CONFLICT {

			return &errors.RetriableError{Err: e}
		}

		// Public errors handling.
		// TODO: Should be reconsidered after NBS-1853 when ClientError will
		// have public/internal flag.
		switch clientErr.Code {
		case nbs_client.E_RESOURCE_EXHAUSTED:
			e = &errors.DetailedError{
				Err: e,
				Details: &errors.ErrorDetails{
					Code:     codes.ResourceExhausted,
					Message:  clientErr.Message,
					Internal: false,
				},
			}
		}
	}

	return e
}

func isAbortedError(e error) bool {
	var clientErr *nbs_client.ClientError
	if errors.As(e, &clientErr) {
		if clientErr.Code == nbs_client.E_CONFIG_VERSION_MISMATCH {
			return true
		}
	}

	return false
}

func IsDiskNotFoundError(e error) bool {
	var clientErr *nbs_client.ClientError
	if errors.As(e, &clientErr) {
		if clientErr.Facility() == nbs_client.FACILITY_SCHEMESHARD {
			// TODO: remove support for PathDoesNotExist.
			if clientErr.Status() == 2 {
				return true
			}

			// Hack for NBS-3162.
			if strings.Contains(clientErr.Error(), "Another drop in progress") {
				return true
			}
		}
	}

	return false
}

func IsNotFoundError(e error) bool {
	if IsDiskNotFoundError(e) {
		return true
	}

	var clientErr *nbs_client.ClientError
	return errors.As(e, &clientErr) && clientErr.Code == nbs_client.E_NOT_FOUND
}

////////////////////////////////////////////////////////////////////////////////

func setupStderrLogger(ctx context.Context) context.Context {
	return logging.SetLogger(
		ctx,
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

func min(x, y uint64) uint64 {
	if x > y {
		return y
	}

	return x
}

////////////////////////////////////////////////////////////////////////////////

func requestDurationBuckets() metrics.DurationBuckets {
	return metrics.NewDurationBuckets(
		1*time.Millisecond, 10*time.Millisecond, 20*time.Millisecond, 50*time.Millisecond,
		100*time.Millisecond, 200*time.Millisecond, 500*time.Millisecond,
		1*time.Second, 2*time.Second, 5*time.Second,
	)
}

////////////////////////////////////////////////////////////////////////////////

type requestStats struct {
	errors        metrics.Counter
	count         metrics.Counter
	timeHistogram metrics.Timer
}

func (s *requestStats) onCount() {
	s.count.Inc()
}

func (s *requestStats) onError() {
	s.errors.Inc()
}

func (s *requestStats) recordDuration(duration time.Duration) {
	s.timeHistogram.RecordDuration(duration)
}

////////////////////////////////////////////////////////////////////////////////

type clientMetrics struct {
	registry         metrics.Registry
	underlyingErrors metrics.Counter
	requestStats     map[string]*requestStats
	requestMutex     sync.Mutex
}

func (m *clientMetrics) getOrCreateRequestStats(
	name string,
) *requestStats {

	m.requestMutex.Lock()
	defer m.requestMutex.Unlock()

	stats, ok := m.requestStats[name]
	if !ok {
		subRegistry := m.registry.WithTags(map[string]string{
			"request": name,
		})

		stats = &requestStats{
			errors:        subRegistry.Counter("errors"),
			count:         subRegistry.Counter("count"),
			timeHistogram: subRegistry.DurationHistogram("time", requestDurationBuckets()),
		}

		m.requestStats[name] = stats
	}

	return stats
}

func (m *clientMetrics) OnError(err nbs_client.ClientError) {
	if err.Succeeded() {
		return
	}

	// TODO: split metrics into types (retriable, fatal, etc.)
	m.underlyingErrors.Inc()
}

func (m *clientMetrics) StatRequest(name string) func(err *error) {
	start := time.Now()
	stats := m.getOrCreateRequestStats(name)

	return func(err *error) {
		if *err != nil {
			stats.onError()
		} else {
			stats.onCount()
			stats.recordDuration(time.Since(start))
		}
	}
}

func makeClientMetrics(registry metrics.Registry) *clientMetrics {
	return &clientMetrics{
		registry:         registry,
		underlyingErrors: registry.Counter("underlying_errors"),
		requestStats:     make(map[string]*requestStats),
	}
}

////////////////////////////////////////////////////////////////////////////////

type client struct {
	nbs     *nbs_client.DiscoveryClient
	metrics *clientMetrics
}

func (c *client) updateVolume(
	ctx context.Context,
	saveState func() error,
	diskID string,
	do func(volume *protos.TVolume) error,
) error {

	retries := 0
	for {
		volume, err := c.nbs.DescribeVolume(ctx, diskID)
		if err != nil {
			return wrapError(err)
		}

		err = saveState()
		if err != nil {
			return err
		}

		err = do(volume)
		if err != nil {
			if !isAbortedError(err) {
				return err
			}

			if retries == maxConsecutiveRetries {
				return &errors.RetriableError{Err: err}
			}

			retries++
			continue
		}

		return nil
	}
}

// TODO: unify with updateVolume.
func (c *client) updatePlacementGroup(
	ctx context.Context,
	saveState func() error,
	groupID string,
	do func(group *protos.TPlacementGroup) error,
) error {

	retries := 0
	for {
		group, err := c.nbs.DescribePlacementGroup(ctx, groupID)
		if err != nil {
			return wrapError(err)
		}

		err = saveState()
		if err != nil {
			return err
		}

		err = do(group)
		if err != nil {
			if !isAbortedError(err) {
				return err
			}

			if retries == maxConsecutiveRetries {
				return &errors.RetriableError{Err: err}
			}

			retries++
			continue
		}

		return nil
	}
}

func (c *client) executeAction(
	ctx context.Context,
	action string,
	request proto.Message,
	response proto.Message,
) error {

	input, err := new(jsonpb.Marshaler).MarshalToString(request)
	if err != nil {
		return &errors.NonRetriableError{
			Err: fmt.Errorf("failed to marshal request: %v", err),
		}
	}

	output, err := c.nbs.ExecuteAction(ctx, action, []byte(input))
	if err != nil {
		return wrapError(err)
	}

	err = new(jsonpb.Unmarshaler).Unmarshal(
		strings.NewReader(string(output)),
		response,
	)
	if err != nil {
		return &errors.NonRetriableError{
			Err: fmt.Errorf("failed to unmarshal response: %v", err),
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func (c *client) Create(
	ctx context.Context,
	params CreateDiskParams,
) (err error) {

	defer c.metrics.StatRequest("Create")(&err)

	kind, err := getStorageMediaKind(params.Kind)
	if err != nil {
		return err
	}

	encryptionMode, err := getEncryptionMode(params.EncryptionMode)
	if err != nil {
		return err
	}

	err = c.nbs.CreateVolume(
		ctx,
		params.ID,
		params.BlocksCount,
		&nbs_client.CreateVolumeOpts{
			BaseDiskId:            params.BaseDiskID,
			BaseDiskCheckpointId:  params.BaseDiskCheckpointID,
			BlockSize:             params.BlockSize,
			StorageMediaKind:      kind,
			CloudId:               params.CloudID,
			FolderId:              params.FolderID,
			TabletVersion:         params.TabletVersion,
			PlacementGroupId:      params.PlacementGroupID,
			PartitionsCount:       params.PartitionsCount,
			IsSystem:              params.IsSystem,
			EncryptionMode:        encryptionMode,
			EncryptionKeyFilePath: params.EncryptionKeyFilePath,
			StoragePoolName:       params.StoragePoolName,
			AgentIds:              params.AgentIds,
		},
	)
	return wrapError(err)
}

func (c *client) Delete(
	ctx context.Context,
	diskID string,
) (err error) {

	defer c.metrics.StatRequest("Delete")(&err)

	err = c.nbs.DestroyVolume(ctx, diskID)
	return wrapError(err)
}

func (c *client) CreateCheckpoint(
	ctx context.Context,
	diskID string,
	checkpointID string,
) (err error) {

	defer c.metrics.StatRequest("CreateCheckpoint")(&err)

	err = c.nbs.CreateCheckpoint(ctx, diskID, checkpointID)
	return wrapError(err)
}

func (c *client) DeleteCheckpoint(
	ctx context.Context,
	diskID string,
	checkpointID string,
) (err error) {

	defer c.metrics.StatRequest("DeleteCheckpoint")(&err)

	err = c.nbs.DeleteCheckpoint(ctx, diskID, checkpointID)
	if IsNotFoundError(err) {
		return nil
	}

	return wrapError(err)
}

func (c *client) DeleteCheckpointData(
	ctx context.Context,
	diskID string,
	checkpointID string,
) (err error) {

	defer c.metrics.StatRequest("DeleteCheckpointData")(&err)

	response := &private_protos.TDeleteCheckpointDataResponse{}

	err = c.executeAction(
		ctx,
		"DeleteCheckpointData",
		&private_protos.TDeleteCheckpointDataRequest{
			DiskId:       diskID,
			CheckpointId: checkpointID,
		},
		response,
	)
	if IsNotFoundError(err) {
		return nil
	}

	return err
}

func (c *client) Resize(
	ctx context.Context,
	checkpoint func() error,
	diskID string,
	size uint64,
) (err error) {

	defer c.metrics.StatRequest("Resize")(&err)

	return c.updateVolume(ctx, checkpoint, diskID, func(volume *protos.TVolume) error {
		if volume.BlockSize == 0 {
			return fmt.Errorf("invalid volume config %v", volume)
		}
		if size%uint64(volume.BlockSize) != 0 {
			return fmt.Errorf(
				"size %v should be divisible by volume.BlockSize %v",
				size,
				volume.BlockSize,
			)
		}
		newBlocksCount := size / uint64(volume.BlockSize)

		err := c.nbs.ResizeVolume(
			ctx,
			diskID,
			newBlocksCount,
			0, // channelsCount
			volume.ConfigVersion,
		)
		return wrapError(err)
	})
}

func (c *client) Alter(
	ctx context.Context,
	saveState func() error,
	diskID string,
	cloudID string,
	folderID string,
) (err error) {

	defer c.metrics.StatRequest("Alter")(&err)

	return c.updateVolume(ctx, saveState, diskID, func(volume *protos.TVolume) error {
		err := c.nbs.AlterVolume(
			ctx,
			diskID,
			volume.ProjectId,
			folderID,
			cloudID,
			volume.ConfigVersion,
		)
		return wrapError(err)
	})
}

func (c *client) Rebase(
	ctx context.Context,
	saveState func() error,
	diskID string,
	baseDiskID string,
	targetBaseDiskID string,
) (err error) {

	defer c.metrics.StatRequest("Rebase")(&err)

	return c.updateVolume(ctx, saveState, diskID, func(volume *protos.TVolume) error {
		if volume.BaseDiskId == targetBaseDiskID {
			// Should be idempotent.
			return nil
		}

		if volume.BaseDiskId != baseDiskID {
			return &errors.NonRetriableError{
				Err: fmt.Errorf(
					"unexpected baseDiskID for rebase, expected=%v, actual=%v",
					baseDiskID,
					volume.BaseDiskId,
				),
			}
		}

		response := &private_protos.TRebaseVolumeResponse{}
		return c.executeAction(
			ctx,
			"RebaseVolume",
			&private_protos.TRebaseVolumeRequest{
				DiskId:           diskID,
				TargetBaseDiskId: targetBaseDiskID,
				ConfigVersion:    volume.ConfigVersion,
			},
			response,
		)
	})
}

func (c *client) Assign(
	ctx context.Context,
	params AssignDiskParams,
) (err error) {

	defer c.metrics.StatRequest("Assign")(&err)

	_, err = c.nbs.AssignVolume(
		ctx,
		params.ID,
		params.InstanceID,
		params.Token,
		params.Host,
	)
	return wrapError(err)
}

func (c *client) Unassign(
	ctx context.Context,
	diskID string,
) (err error) {

	defer c.metrics.StatRequest("Unassign")(&err)

	_, err = c.nbs.AssignVolume(
		ctx,
		diskID,
		"",
		"",
		"",
	)
	if IsNotFoundError(err) {
		return nil
	}

	return wrapError(err)
}

func (c *client) DescribeModel(
	ctx context.Context,
	blocksCount uint64,
	blockSize uint32,
	kind types.DiskKind,
	tabletVersion uint32,
) (diskModel DiskModel, err error) {

	defer c.metrics.StatRequest("DescribeModel")(&err)

	mediaKind, err := getStorageMediaKind(kind)
	if err != nil {
		return DiskModel{}, err
	}

	model, err := c.nbs.DescribeVolumeModel(
		ctx,
		blocksCount,
		blockSize,
		mediaKind,
		tabletVersion,
	)
	if err != nil {
		return DiskModel{}, wrapError(err)
	}

	return DiskModel{
		BlockSize:     model.BlockSize,
		BlocksCount:   model.BlocksCount,
		ChannelsCount: model.ChannelsCount,
		Kind:          kind,
		PerformanceProfile: DiskPerformanceProfile{
			MaxReadBandwidth:   model.PerformanceProfile.MaxReadBandwidth,
			MaxPostponedWeight: model.PerformanceProfile.MaxPostponedWeight,
			ThrottlingEnabled:  model.PerformanceProfile.ThrottlingEnabled,
			MaxReadIops:        model.PerformanceProfile.MaxReadIops,
			BoostTime:          model.PerformanceProfile.BoostTime,
			BoostRefillTime:    model.PerformanceProfile.BoostRefillTime,
			BoostPercentage:    model.PerformanceProfile.BoostPercentage,
			MaxWriteBandwidth:  model.PerformanceProfile.MaxWriteBandwidth,
			MaxWriteIops:       model.PerformanceProfile.MaxWriteIops,
			BurstPercentage:    model.PerformanceProfile.BurstPercentage,
		},
		MergedChannelsCount: model.MergedChannelsCount,
		MixedChannelsCount:  model.MixedChannelsCount,
	}, nil
}

func (c *client) Describe(
	ctx context.Context,
	diskID string,
) (diskParams DiskParams, err error) {

	defer c.metrics.StatRequest("Describe")(&err)

	volume, err := c.nbs.DescribeVolume(ctx, diskID)
	if err != nil {
		return DiskParams{}, wrapError(err)
	}

	return DiskParams{
		BlockSize:   volume.BlockSize,
		BlocksCount: volume.BlocksCount,
	}, nil
}

func (c *client) CreatePlacementGroup(
	ctx context.Context,
	groupID string,
	placementStrategy types.PlacementStrategy,
) (err error) {

	defer c.metrics.StatRequest("CreatePlacementGroup")(&err)

	strategy, err := toPlacementStrategy(placementStrategy)
	if err != nil {
		return err
	}

	err = c.nbs.CreatePlacementGroup(ctx, groupID, strategy)
	return wrapError(err)
}

func (c *client) DeletePlacementGroup(
	ctx context.Context,
	groupID string,
) (err error) {

	defer c.metrics.StatRequest("DeletePlacementGroup")(&err)

	err = c.nbs.DestroyPlacementGroup(ctx, groupID)
	return wrapError(err)
}

func (c *client) AlterPlacementGroupMembership(
	ctx context.Context,
	saveState func() error,
	groupID string,
	disksToAdd []string,
	disksToRemove []string,
) (err error) {

	defer c.metrics.StatRequest("AlterPlacementGroupMembership")(&err)

	return c.updatePlacementGroup(ctx, saveState, groupID, func(group *protos.TPlacementGroup) error {
		err := c.nbs.AlterPlacementGroupMembership(
			ctx,
			groupID,
			disksToAdd,
			disksToRemove,
			group.ConfigVersion,
		)
		return wrapError(err)
	})
}

func (c *client) ListPlacementGroups(
	ctx context.Context,
) (groups []string, err error) {

	defer c.metrics.StatRequest("ListPlacementGroups")(&err)

	groups, err = c.nbs.ListPlacementGroups(ctx)
	if err != nil {
		return nil, wrapError(err)
	}

	return groups, nil
}

func (c *client) DescribePlacementGroup(
	ctx context.Context,
	groupID string,
) (placementGroup PlacementGroup, err error) {

	defer c.metrics.StatRequest("DescribePlacementGroup")(&err)

	group, err := c.nbs.DescribePlacementGroup(ctx, groupID)
	if err != nil {
		return PlacementGroup{}, wrapError(err)
	}

	strategy, err := fromPlacementStrategy(group.PlacementStrategy)
	if err != nil {
		return PlacementGroup{}, err
	}

	return PlacementGroup{
		GroupID:           group.GroupId,
		PlacementStrategy: strategy,
		DiskIDs:           group.DiskIds,
		Racks:             group.Racks,
	}, nil
}

func (c *client) MountRO(
	ctx context.Context,
	diskID string,
) (s Session, err error) {

	defer c.metrics.StatRequest("MountRO")(&err)

	endpoint, err := c.nbs.DiscoverInstance(ctx)
	if err != nil {
		return Session{}, wrapError(err)
	}

	session := nbs_client.NewSession(
		*endpoint,
		NewNbsClientLog(nbs_client.LOG_DEBUG),
	)

	opts := nbs_client.MountVolumeOpts{
		MountFlags:     protoFlags(protos.EMountFlag_MF_THROTTLING_DISABLED),
		MountSeqNumber: 0,
		AccessMode:     protos.EVolumeAccessMode_VOLUME_ACCESS_READ_ONLY,
		MountMode:      protos.EVolumeMountMode_VOLUME_MOUNT_REMOTE,
	}
	err = session.MountVolume(ctx, diskID, &opts)
	if err != nil {
		return Session{}, wrapError(err)
	}

	return Session{
		client:  *endpoint,
		session: session,
		metrics: c.metrics,
	}, nil
}

func (c *client) MountRW(
	ctx context.Context,
	diskID string,
) (s Session, err error) {

	defer c.metrics.StatRequest("MountRW")(&err)

	endpoint, err := c.nbs.DiscoverInstance(ctx)
	if err != nil {
		return Session{}, wrapError(err)
	}

	session := nbs_client.NewSession(
		*endpoint,
		NewNbsClientLog(nbs_client.LOG_DEBUG),
	)

	// We use local mount here for saving one network hop.
	opts := nbs_client.MountVolumeOpts{
		MountFlags:     protoFlags(protos.EMountFlag_MF_THROTTLING_DISABLED),
		MountSeqNumber: 0,
		AccessMode:     protos.EVolumeAccessMode_VOLUME_ACCESS_READ_WRITE,
		MountMode:      protos.EVolumeMountMode_VOLUME_MOUNT_LOCAL,
	}
	err = session.MountVolume(ctx, diskID, &opts)
	if err != nil {
		return Session{}, wrapError(err)
	}

	return Session{
		client:  *endpoint,
		session: session,
		metrics: c.metrics,
	}, nil
}

func (c *client) GetChangedBlocks(
	ctx context.Context,
	diskID string,
	startIndex uint64,
	blockCount uint32,
	baseCheckpointID,
	checkpointID string,
) (blockMask []byte, err error) {

	defer c.metrics.StatRequest("GetChangedBlocks")(&err)

	blockMask, err = c.nbs.GetChangedBlocks(
		ctx,
		diskID,
		startIndex,
		blockCount,
		baseCheckpointID, // lowCheckpointID
		checkpointID,     // highCheckpointID
		false,            // ignoreBaseDisk
	)
	return blockMask, wrapError(err)
}

func (c *client) GetCheckpointSize(
	ctx context.Context,
	saveState func(blockIndex uint64, checkpointSize uint64) error,
	diskID string,
	checkpointID string,
	milestoneBlockIndex uint64,
	milestoneCheckpointSize uint64,
) (err error) {

	defer c.metrics.StatRequest("GetCheckpointSize")(&err)

	volume, err := c.nbs.DescribeVolume(ctx, diskID)
	if err != nil {
		return wrapError(err)
	}

	blockIndex := milestoneBlockIndex
	if blockIndex >= volume.BlocksCount {
		return nil
	}

	maxUsedBlockIndex := milestoneCheckpointSize / uint64(volume.BlockSize)

	for {
		blockCount := uint32(maxChangedBlockCountPerIteration)
		if uint64(blockCount) > volume.BlocksCount-blockIndex {
			blockCount = uint32(volume.BlocksCount - blockIndex)
		}

		blockMask, err := c.GetChangedBlocks(
			ctx,
			diskID,
			blockIndex,
			blockCount,
			"",
			checkpointID,
		)
		if err != nil {
			return err
		}

		if len(blockMask) != 0 {
			for i := len(blockMask) - 1; i >= 0; i-- {
				h := highestBitPosition(blockMask[i])
				if h == 0 {
					continue
				}

				maxUsedBlockIndex = blockIndex + uint64(8*i) + uint64(h-1)
				break
			}
		}

		blockIndex += maxChangedBlockCountPerIteration
		checkpointSize := maxUsedBlockIndex * uint64(volume.BlockSize)

		if blockIndex >= volume.BlocksCount {
			return saveState(blockIndex, checkpointSize)
		}

		if blockIndex%(blockCountToSaveStateThreshold) == 0 {
			err = saveState(blockIndex, checkpointSize)
			if err != nil {
				return err
			}
		}
	}
}

func (c *client) Stat(
	ctx context.Context,
	diskID string,
) (stats DiskStats, err error) {

	defer c.metrics.StatRequest("Stat")(&err)

	volume, volumeStats, err := c.nbs.StatVolume(ctx, diskID, uint32(0))
	if err != nil {
		return DiskStats{}, wrapError(err)
	}

	return DiskStats{
		StorageSize: uint64(volume.BlockSize) * volumeStats.LogicalUsedBlocksCount,
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func (c *client) ValidateCrc32(
	diskID string,
	contentSize uint64,
	expectedCrc32 uint32,
) error {

	ctx := setupStderrLogger(context.Background())

	nbsClient, err := c.nbs.DiscoverInstance(ctx)
	if err != nil {
		return err
	}
	defer nbsClient.Close()

	session := nbs_client.NewSession(
		*nbsClient,
		NewNbsClientLog(nbs_client.LOG_DEBUG),
	)
	defer session.Close()

	opts := nbs_client.MountVolumeOpts{
		MountFlags:     protoFlags(protos.EMountFlag_MF_THROTTLING_DISABLED),
		MountSeqNumber: 0,
		AccessMode:     protos.EVolumeAccessMode_VOLUME_ACCESS_READ_ONLY,
		MountMode:      protos.EVolumeMountMode_VOLUME_MOUNT_REMOTE,
	}
	err = session.MountVolume(ctx, diskID, &opts)
	if err != nil {
		return err
	}
	defer session.UnmountVolume(ctx)

	volume := session.Volume()

	volumeBlockSize := uint64(volume.BlockSize)
	if volumeBlockSize == 0 {
		return fmt.Errorf(
			"%v volume block size should not be zero",
			diskID,
		)
	}

	if contentSize%volumeBlockSize != 0 {
		return fmt.Errorf(
			"%v contentSize %v should be multiple of volumeBlockSize %v",
			diskID,
			contentSize,
			volumeBlockSize,
		)
	}

	contentBlocksCount := contentSize / volumeBlockSize
	volumeSize := volume.BlocksCount * volumeBlockSize

	if contentSize > volumeSize {
		return fmt.Errorf(
			"%v contentSize %v should not be greater than volumeSize %v",
			diskID,
			contentSize,
			volumeSize,
		)
	}

	chunkSize := uint64(4 * 1024 * 1024)
	blocksInChunk := chunkSize / volumeBlockSize
	acc := crc32.NewIEEE()

	for offset := uint64(0); offset < contentBlocksCount; offset += blocksInChunk {
		blocksToRead := min(contentBlocksCount-offset, blocksInChunk)
		buffers, err := session.ReadBlocks(ctx, offset, uint32(blocksToRead), "")
		if err != nil {
			return fmt.Errorf(
				"%v read blocks at (%v, %v) failed: %w",
				diskID,
				offset,
				blocksToRead,
				err,
			)
		}

		for _, buffer := range buffers {
			if len(buffer) == 0 {
				buffer = make([]byte, volumeBlockSize)
			}

			_, err := acc.Write(buffer)
			if err != nil {
				return err
			}
		}
	}

	actualCrc32 := acc.Sum32()
	if expectedCrc32 != actualCrc32 {
		return fmt.Errorf(
			"%v crc32 doesn't match, expected=%v, actual=%v",
			diskID,
			expectedCrc32,
			actualCrc32,
		)
	}

	// Validate that region outside of contentSize is filled with zeroes.
	for offset := contentBlocksCount; offset < volume.BlocksCount; offset += blocksInChunk {
		blocksToRead := min(volume.BlocksCount-offset, blocksInChunk)
		buffers, err := session.ReadBlocks(ctx, offset, uint32(blocksToRead), "")
		if err != nil {
			return fmt.Errorf(
				"%v read blocks at (%v, %v) failed: %w",
				diskID,
				offset,
				blocksToRead,
				err,
			)
		}

		for i, buffer := range buffers {
			if len(buffer) != 0 {
				for j, b := range buffer {
					if b != 0 {
						return fmt.Errorf(
							"%v non zero byte %v detected at (%v, %v)",
							diskID,
							b,
							offset+uint64(i),
							j,
						)
					}
				}
			}
		}
	}

	return nil
}

func (c *client) MountForReadWrite(
	diskID string,
) (func(), error) {

	ctx := setupStderrLogger(context.Background())

	nbsClient, err := c.nbs.DiscoverInstance(ctx)
	if err != nil {
		return func() {}, err
	}

	session := nbs_client.NewSession(
		*nbsClient,
		NewNbsClientLog(nbs_client.LOG_DEBUG),
	)

	opts := nbs_client.MountVolumeOpts{
		MountFlags:     protoFlags(protos.EMountFlag_MF_THROTTLING_DISABLED),
		MountSeqNumber: 0,
		AccessMode:     protos.EVolumeAccessMode_VOLUME_ACCESS_READ_WRITE,
		MountMode:      protos.EVolumeMountMode_VOLUME_MOUNT_REMOTE,
	}
	err = session.MountVolume(ctx, diskID, &opts)
	if err != nil {
		session.Close()
		_ = nbsClient.Close()
		return func() {}, err
	}

	unmountFunc := func() {
		// Not interested in error.
		_ = session.UnmountVolume(ctx)
		session.Close()
		_ = nbsClient.Close()
	}
	return unmountFunc, nil
}

func (c *client) Write(
	diskID string,
	startIndex int,
	bytes []byte,
) error {

	ctx := setupStderrLogger(context.Background())

	nbsClient, err := c.nbs.DiscoverInstance(ctx)
	if err != nil {
		return err
	}
	defer nbsClient.Close()

	session := nbs_client.NewSession(
		*nbsClient,
		NewNbsClientLog(nbs_client.LOG_DEBUG),
	)
	defer session.Close()

	opts := nbs_client.MountVolumeOpts{
		MountFlags:     protoFlags(protos.EMountFlag_MF_THROTTLING_DISABLED),
		MountSeqNumber: 0,
		AccessMode:     protos.EVolumeAccessMode_VOLUME_ACCESS_READ_WRITE,
		MountMode:      protos.EVolumeMountMode_VOLUME_MOUNT_REMOTE,
	}
	err = session.MountVolume(ctx, diskID, &opts)
	if err != nil {
		return err
	}
	defer session.UnmountVolume(ctx)

	volume := session.Volume()
	diskSize := int(volume.BlocksCount) * int(volume.BlockSize)
	blockSize := int(volume.BlockSize)

	if startIndex < 0 || startIndex+len(bytes) > diskSize {
		return fmt.Errorf("invalid write range=(%v, %v)", startIndex, len(bytes))
	}

	if startIndex%blockSize != 0 || len(bytes)%blockSize != 0 {
		return fmt.Errorf("startIndex and len(bytes) should be divisible by block size")
	}

	chunkSize := 1024 * blockSize

	for i := 0; i < len(bytes); i += chunkSize {
		blocks := make([][]byte, 0)
		end := int(min(uint64(len(bytes)), uint64(i+chunkSize)))

		for j := i; j < end; j += blockSize {
			buffer := make([]byte, blockSize)
			copy(buffer, bytes[j:j+len(buffer)])

			blocks = append(blocks, buffer)
		}

		err = session.WriteBlocks(ctx, uint64((i+startIndex)/blockSize), blocks)
		if err != nil {
			return err
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type checkpoint struct {
	CheckpointID string `json:"CheckpointId"`
	// We don't need other checkpoint fields.
}

type partitionInfo struct {
	Checkpoints []checkpoint `json:"Checkpoints"`
	// We don't need other partitionInfo fields.
}

func (c *client) GetCheckpoints(
	ctx context.Context,
	diskID string,
) ([]string, error) {

	// TODO: Use Proto instead of raw JSON.
	output, err := c.nbs.ExecuteAction(
		ctx,
		"GetPartitionInfo",
		[]byte(fmt.Sprintf(`{"DiskId": "%v"}`, diskID)),
	)
	if err != nil {
		return []string{}, err
	}

	logging.Debug(ctx, "GetCheckpoints got output=%v", string(output))

	var info partitionInfo
	err = json.Unmarshal(output, &info)
	if err != nil {
		return []string{}, err
	}

	logging.Debug(ctx, "GetCheckpoints got partitionInfo=%v", info)

	ids := make([]string, 0, len(info.Checkpoints))
	for _, checkpoint := range info.Checkpoints {
		ids = append(ids, checkpoint.CheckpointID)
	}

	return ids, nil
}
