package move

import (
	"fmt"
	"io/ioutil"
	stdlog "log"
	"os"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"

	"a.yandex-team.ru/cloud/blockstore/public/api/protos"
	"a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"

	"github.com/gofrs/uuid"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/chunker"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const (
	maxChangedBlockCountPerIteration = 1 << 20
	byteBits                         = 1 << 3
)

type changedBlocksMask struct {
	mask      []byte
	blockSize uint64
}

func newChangedBlocksMask(blockSize, totalSize uint64) *changedBlocksMask {
	blocksCount := (totalSize + blockSize - 1) / blockSize
	maskSize := (blocksCount + byteBits - 1) / byteBits
	return &changedBlocksMask{
		mask:      make([]byte, maskSize),
		blockSize: blockSize,
	}
}

func (cbm *changedBlocksMask) AddMask(ctx context.Context, blockOffset, blockSize uint64, mask []byte) {
	// blockSize <= cbm.BlockSize
	if cbm.blockSize%blockSize != 0 {
		log.G(ctx).Panic("Block size mismatch.", zap.Uint64("cbm.blockSize", cbm.blockSize),
			zap.Uint64("block_size", blockSize))
	}

	for i, b := range mask {
		for j := uint64(0); j < byteBits; j++ {
			sourceBlockOffset := blockOffset + uint64(i)*byteBits + j
			maskBlockOffset := sourceBlockOffset * blockSize / cbm.blockSize
			maskByte, maskBit := uint(maskBlockOffset/byteBits), uint(maskBlockOffset%byteBits)
			cbm.mask[maskByte] |= ((b >> j) & 1) << maskBit
		}
	}
}

func (cbm *changedBlocksMask) GetBit(ctx context.Context, blockOffset, blockSize uint64) bool {
	if blockSize > cbm.blockSize && blockSize%cbm.blockSize != 0 ||
		blockSize < cbm.blockSize && cbm.blockSize%blockSize != 0 {
		log.G(ctx).Panic("Block size mismatch", zap.Uint64("blockSize", blockSize),
			zap.Uint64("cbm.blockSize", cbm.blockSize))
	}

	maskStartOffset := blockOffset * blockSize / cbm.blockSize
	maskEndOffset := (blockOffset + 1) * blockSize / cbm.blockSize
	if maskStartOffset == maskEndOffset {
		// For small blocks
		maskEndOffset = maskStartOffset + 1
	}

	var result bool
	for maskBlockOffset := maskStartOffset; maskBlockOffset < maskEndOffset; maskBlockOffset++ {
		maskByte, maskBit := uint(maskBlockOffset/byteBits), uint(maskBlockOffset%byteBits)
		result = result || (cbm.mask[maskByte]>>maskBit)&1 == 1
	}
	return result
}

type nbsDevice struct {
	session   *client.Session
	client    *client.Client
	diskID    string
	size      int64
	blockSize int64

	// For src only.
	firstCheckpointID string
	lastCheckpointID  string
	cbm               *changedBlocksMask
}

func (nd *nbsDevice) GetBlockSize() int64 {
	return nd.blockSize
}

func (nd *nbsDevice) GetSize() int64 {
	return nd.size
}

func (nd *nbsDevice) ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	if offset%nd.blockSize != 0 {
		return false, false, misc.ErrInvalidOffset
	}

	if int64(len(data))%nd.blockSize != 0 {
		return false, false, misc.ErrInvalidBlockSize
	}

	if nd.cbm != nil && !nd.cbm.GetBit(ctx, uint64(offset)/uint64(len(data)), uint64(len(data))) {
		// NOTE: BlockSize calculation ensures
		// len(data) % nd.blockSize == 0 and
		// len(data) % params.BlockSize == 0
		return false, false, nil
	}

	blocksCount := uint32(int64(len(data)) / nd.blockSize)

	// TODO: Check changed block map.
	t := time.Now()
	t1 := misc.NBSReadZerosTimer.Start()
	t2 := misc.NBSReadDataFullTimer.Start()
	buffers, err := nd.session.ReadBlocks(
		ctx, uint64(offset/nd.blockSize),
		blocksCount, nd.lastCheckpointID)
	if err != nil {
		log.G(ctx).Error("ReadBlocks failed", zap.String("diskID", nd.diskID), zap.Error(err),
			zap.Int64("nbs_offset", offset))
		return false, false, err
	}
	done := time.Since(t)

	if client.AllBlocksEmpty(buffers) {
		misc.NBSReadAtSpeedZero.Observe(misc.Speed(len(data), done))
		t1.ObserveDuration()
		return true, true, nil
	}

	defer t2.ObserveDuration()

	err = client.JoinBlocks(uint32(nd.blockSize), blocksCount, buffers, data)
	if err != nil {
		log.G(ctx).Error("JoinBlocks failed", zap.Error(err), zap.String("diskID", nd.diskID),
			zap.Int64("nbs_offset", offset))
		return false, false, err
	}

	// NBS returns all-zero blocks for written zeroes, need to check.
	var nz chunker.NotZero
	_, _ = nz.Write(data)
	if !nz {
		misc.NBSReadAtSpeedZero.Observe(misc.Speed(len(data), done))
	} else {
		misc.NBSReadAtSpeed.ObserveContext(ctx, misc.Speed(len(data), done))
	}
	return true, !bool(nz), nil
}

func (nd *nbsDevice) WriteAt(ctx context.Context, offset int64, data []byte) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	tm := misc.NBSWriteDataFullTimer.Start()
	defer tm.ObserveDuration()

	if offset%nd.blockSize != 0 {
		return misc.ErrInvalidOffset
	}

	if int64(len(data))%nd.blockSize != 0 {
		return misc.ErrInvalidBlockSize
	}

	blocks := make([][]byte, 0, int64(len(data))/nd.blockSize)
	for i := int64(0); i < int64(len(data)); i += nd.blockSize {
		blocks = append(blocks, data[i:i+nd.blockSize])
	}
	writeTimer := misc.NBSWriteDataTimer.Start()
	defer writeTimer.ObserveDuration()

	t := time.Now()
	err := nd.session.WriteBlocks(ctx, uint64(offset/nd.blockSize), blocks)
	if err != nil {
		log.G(ctx).Error("WriteBlocks failed", zap.String("diskID", nd.diskID), zap.Error(err),
			zap.Int64("nbs_offset", offset))
		return err
	}
	misc.NBSWriteAtSpeed.ObserveContext(ctx, misc.SpeedSince(len(data), t))

	return nil
}

func (nd *nbsDevice) WriteZeroesAt(ctx context.Context, offset int64, size int) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	tm := misc.NBSWriteZerosTimer.Start()
	defer tm.ObserveDuration()

	if offset%nd.blockSize != 0 {
		return misc.ErrInvalidOffset
	}

	if int64(size)%nd.blockSize != 0 {
		return misc.ErrInvalidBlockSize
	}

	t := time.Now()
	err := nd.session.ZeroBlocks(ctx, uint64(offset/nd.blockSize), uint32(int64(size)/nd.blockSize))
	if err != nil {
		log.G(ctx).Error("ZeroBlocks failed", zap.String("diskID", nd.diskID), zap.Error(err),
			zap.Int64("nbs_offset", offset))
		return err
	}
	misc.NBSWriteAtSpeedZero.Observe(misc.SpeedSince(size, t))
	return nil
}

func (nd *nbsDevice) GetHint() directreader.Hint {
	return directreader.Hint{Size: nd.GetSize()}
}

func (nd *nbsDevice) Close(ctx context.Context, reason error) error {
	log.G(ctx).Debug("Calling Device Close UnmountVolume", zap.Error(reason), zap.String("diskID", nd.diskID))
	err := nd.session.UnmountVolume(ctx)
	err2 := nd.client.Close()
	nd.session.Close()
	log.DebugErrorCtx(ctx, err, "UnmountVolume", zap.String("diskID", nd.diskID))
	log.DebugErrorCtx(ctx, err2, "Close nbs client", zap.String("diskID", nd.diskID))
	if err == nil {
		err = err2
	}
	return err

}

// GetNbsClient returns an NBS client.
func (mdf *DeviceFactory) GetNbsClient(ctx context.Context, clusterID string) (cl *client.Client, err error) {
	logger := log.G(ctx)
	clientConfig, ok := mdf.conf.Nbs[clusterID]
	if !ok {
		logger.Error("GetNbsClient failed: invalid cluster id", zap.String("clusterID", clusterID))
		return nil, misc.ErrInvalidNbsClusterID
	}

	var credentials *client.ClientCredentials
	if clientConfig.SSL {
		credentials = &client.ClientCredentials{
			RootCertsFile:      clientConfig.RootCertsFile,
			CertFile:           clientConfig.CertFile,
			CertPrivateKeyFile: clientConfig.KeyFile,
			IAMClient:          globalauth.GetCredentials(),
		}
	}

	discoveryClient, err := mdf.nbsDiscoveryClientCreate(
		ctx,
		clientConfig.Hosts,
		credentials,
		clientConfig.DiscoveryHostLimit,
	)

	logger.Debug("Create discovery nbs client", zap.Strings("nbsHosts", clientConfig.Hosts), zap.Error(err))

	if err != nil {
		return nil, err
	}

	defer func() {
		err := discoveryClient.Close()
		if err == nil {
			logger.Debug("close discovery nbs client success")
		} else {
			logger.Error("Error while closing discovery nbs client", zap.Error(err))
		}
	}()

	nbsClient, err := discoveryClient.DiscoverInstance(ctx)

	logger.Debug("Create nbs client", zap.Error(err))

	return nbsClient, err
}

func (mdf *DeviceFactory) nbsDiscoveryClientCreate(
	ctx context.Context,
	hosts []string,
	credentials *client.ClientCredentials,
	discoveryHostLimit uint32,
) (*client.DiscoveryClient, error) {
	logger := log.G(ctx)

	hostname, err := os.Hostname()
	if err != nil {
		prettyErr := fmt.Errorf("os.Hostname() failed: %w", err)
		logger.Error("nbsDiscoveryClientCreate", zap.Error(prettyErr))
		return nil, prettyErr
	}

	newID, err := uuid.NewV4()
	if err != nil {
		prettyErr := fmt.Errorf("failed to generate uuid: %w", err)
		logger.Error("nbsDiscoveryClientCreate", zap.Error(prettyErr))
		return nil, prettyErr
	}
	clientID := fmt.Sprintf("%v@%v", newID.String(), hostname)

	c, err := client.NewDiscoveryClient(
		hosts,
		&client.GrpcClientOpts{
			Credentials: credentials,
			ClientId:    clientID,
		},
		&client.DurableClientOpts{},
		&client.DiscoveryClientOpts{
			Limit: discoveryHostLimit,
		},
		mdf.newNbsLog(ctx))
	return c, err
}

func (mdf *DeviceFactory) newNbsLog(ctx context.Context) client.Log {
	var err error
	logger := stdlog.New(ioutil.Discard, "", 0)
	if mdf.conf.Logging.EnableNbs {
		logger, err = zap.NewStdLogAt(log.G(ctx).Named("nbs_client"), zapcore.InfoLevel)
		if err != nil {
			log.G(ctx).Error("newNbsLog: failed to create logger", zap.Error(err))
		}
	}
	return client.NewLog(logger, mdf.conf.Logging.GetNbsLogLevel())
}

func (mdf *DeviceFactory) newNbsDevice(
	ctx context.Context,
	clusterID, diskID string,
	write bool,
) (nd *nbsDevice, err error) {
	c, err := mdf.GetNbsClient(ctx, clusterID)
	if err != nil {
		return nil, err
	}
	defer func() {
		if err != nil {
			log.DebugErrorCtx(ctx, c.Close(), "Close nbs client", zap.String("diskID", diskID))
		}
	}()
	ctx = log.WithLogger(ctx, log.G(ctx).With(zap.String("diskID", diskID), zap.String("cluster_id", clusterID)))

	log.G(ctx).Debug("Calling MountVolume")
	s := client.NewSession(
		*c,
		mdf.newNbsLog(ctx))
	defer func() {
		if err != nil {
			log.G(ctx).Debug("Closing session due to error")
			s.Close()
		}
	}()
	opts := client.MountVolumeOpts{
		MountFlags:     1 << (protos.EMountFlag_MF_THROTTLING_DISABLED - 1),
		MountSeqNumber: 0,
	}
	if write {
		opts.AccessMode = protos.EVolumeAccessMode_VOLUME_ACCESS_READ_WRITE
		opts.MountMode = protos.EVolumeMountMode_VOLUME_MOUNT_LOCAL
	} else {
		opts.AccessMode = protos.EVolumeAccessMode_VOLUME_ACCESS_READ_ONLY
		opts.MountMode = protos.EVolumeMountMode_VOLUME_MOUNT_REMOTE
	}
	err = s.MountVolume(ctx, diskID, &opts)
	log.DebugErrorCtx(ctx, err, "MountVolume")
	if err != nil {
		return nil, err
	}
	volume := s.Volume()

	return &nbsDevice{
		session:   s,
		client:    c,
		diskID:    diskID,
		size:      int64(volume.BlocksCount * uint64(volume.BlockSize)),
		blockSize: int64(volume.BlockSize),
	}, nil
}

func (mdf *DeviceFactory) newNbsMoveSrc(ctx context.Context, src *common.NbsMoveSrc, params common.MoveParams) (Src, error) {
	nd, err := mdf.newNbsDevice(ctx, src.ClusterID, src.DiskID, false)
	if err != nil {
		return nil, err
	}

	ctx = log.WithLogger(ctx, log.G(ctx).With(zap.String("cluster_id", src.ClusterID), zap.String("disk_id", src.DiskID)))

	maskBlockSize := nd.blockSize
	if params.BlockSize > maskBlockSize {
		// Better safe then sorry
		if params.BlockSize%maskBlockSize != 0 {
			err = misc.ErrInvalidBlockSize
			log.G(ctx).Error("newNbsMoveSrc failed", zap.Error(err),
				zap.Int64("params.BlockSize", params.BlockSize), zap.Int64("maskBlockSize", maskBlockSize))
			_ = nd.Close(ctx, err)
			return nil, err
		}
		maskBlockSize = params.BlockSize
	}

	nd.firstCheckpointID = src.FirstCheckpointID
	nd.lastCheckpointID = src.LastCheckpointID

	if clientConfig, ok := mdf.conf.Nbs[src.ClusterID]; !ok || !clientConfig.SupportsChangedBlocks {
		// cbm == nil
		return nd, nil
	}

	nd.cbm = newChangedBlocksMask(uint64(maskBlockSize), uint64(nd.GetSize()))

	totalBlocks := uint64(nd.GetSize() / nd.GetBlockSize())
	for blockOffset := uint64(0); blockOffset < totalBlocks; {
		blocksCount := uint32(maxChangedBlockCountPerIteration)
		if uint64(blocksCount) > totalBlocks-blockOffset {
			blocksCount = uint32(totalBlocks - blockOffset)
		}
		if blocksCount%byteBits != 0 {
			log.G(ctx).Error("newNbsMoveSrc: mask size invalid", zap.Uint32("blocksCount", blocksCount))
			e := xerrors.Errorf("newNbsMoveSrc: mask size invalid: %v", blocksCount)
			_ = nd.Close(ctx, e)
			return nil, e
		}

		// TODO: Temporary hack to suppress check for overlay disks.
		// Should be fixed in NBS-1268.
		ignoreBaseDisk := true
		mask, err := nd.client.GetChangedBlocks(ctx, src.DiskID, blockOffset, blocksCount,
			src.FirstCheckpointID, src.LastCheckpointID, ignoreBaseDisk)
		if err != nil {
			log.G(ctx).Error("GetChangedBlocks failed", zap.Error(err))
			_ = nd.Close(ctx, err)
			return nil, err
		}

		nd.cbm.AddMask(ctx, blockOffset, uint64(nd.GetBlockSize()), mask)
		blockOffset += uint64(blocksCount)
	}

	return nd, nil
}

func (mdf *DeviceFactory) newNbsMoveDst(ctx context.Context, dst *common.NbsMoveDst) (Dst, error) {
	return mdf.newNbsDevice(ctx, dst.ClusterID, dst.DiskID, true)
}
