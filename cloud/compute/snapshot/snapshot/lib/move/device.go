package move

import (
	"context"
	"errors"
	"fmt"
	"net"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/mon"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/convert"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

const nbsCheckTimeout = 3 * time.Second

// ConsistencyVerifier used for checking validity of incoming data
type ConsistencyVerifier interface {
	VerifyMetadata(ctx context.Context) error
}

// BlockDevice is a common reader/writer interface part.
type BlockDevice interface {
	GetBlockSize() int64
	GetSize() int64
}

// Src represent data move source.
type Src interface {
	BlockDevice
	ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error)
	GetHint() directreader.Hint
	Close(ctx context.Context, err error) error
}

// Dst represents data move destination.
type Dst interface {
	BlockDevice
	WriteAt(ctx context.Context, offset int64, data []byte) error
	WriteZeroesAt(ctx context.Context, offset int64, size int) error
	Close(ctx context.Context, err error) error
}

// Flushable represents device which require flushing data.
type Flushable interface {
	Flush(ctx context.Context) error
}

// DeviceFactory creates move devices.
type DeviceFactory struct {
	st        storage.Storage
	conf      *config.Config
	c         *convert.Converter
	reg       convert.ImageSourceRegistry
	proxySock string
}

// NewDeviceFactory returns a new DeviceFactory instance.
func NewDeviceFactory(st storage.Storage, conf *config.Config, c *convert.Converter, reg convert.ImageSourceRegistry, proxySock string) *DeviceFactory {
	return &DeviceFactory{
		st:        st,
		conf:      conf,
		c:         c,
		reg:       reg,
		proxySock: proxySock,
	}
}

// GetSrc builds a Src from request.
func (mdf *DeviceFactory) GetSrc(ctx context.Context, src common.MoveSrc, params common.MoveParams) (resSrc Src, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	const srcType = "src_type"
	span.SetTag(srcType, "unknown")

	switch src := src.(type) {
	case *common.SnapshotMoveSrc:
		span.SetTag(srcType, "snapshot")
		return mdf.newSnapshotMoveSrc(ctx, src, mdf.conf.General.ChecksumAlgorithm)
	case *common.NbsMoveSrc:
		span.SetTag(srcType, "nbs")
		return mdf.newNbsMoveSrc(ctx, src, params)
	case *common.NullMoveSrc:
		span.SetTag(srcType, "null")
		return mdf.newNullMoveSrc(ctx, src)
	case *common.URLMoveSrc:
		span.SetTag(srcType, "url")
		span.SetTag("src_format", src.Format)
		log.G(ctx).Debug("Read blocks map start.")
		ctx = metriclabels.WithFormat(ctx, src.Format)

		// create http client for work through proxy sock
		sockHTTPClient := http.Client{
			Transport: &http.Transport{
				DialContext: func(ctx context.Context, network, addr string) (conn net.Conn, err error) {
					d := net.Dialer{}
					return d.DialContext(ctx, "unix", mdf.proxySock)
				},
			},
		}
		getEtag := func() (string, error) {
			resp, err := sockHTTPClient.Head(src.ShadowedURL)
			if err != nil {
				return "", fmt.Errorf("failed to get size from Head request: %w", err)
			}
			_ = resp.Body.Close()
			if resp.StatusCode >= 400 && resp.StatusCode <= 499 {
				return "", misc.ErrUnreachableSource
			}

			if resp.StatusCode != http.StatusOK {
				return "", errors.New("status is not ok")
			}
			return resp.Header.Get("Etag"), nil
		}

		getSize := func() (int64, error) {
			resp, err := sockHTTPClient.Head(src.ShadowedURL)
			if err != nil {
				return 0, fmt.Errorf("failed to get size from Head request: %w", err)
			}
			_ = resp.Body.Close()
			if resp.StatusCode != http.StatusOK || resp.ContentLength < 0 {
				return 0, errors.New("failed to get content-length from request")
			}
			return resp.ContentLength, nil
		}
		if blockMap, err := directreader.ReadBlocksMap(ctx, src.ShadowedURL, src.Format, getEtag, getSize, mdf.proxySock); err == nil {
			urlImgReader := directreader.NewSequenceURLImageReader(blockMap, storage.DefaultChunkSize, &sockHTTPClient)

			canDirectRead := urlImgReader.CanReadDirect()
			log.G(ctx).Debug("Read block map completed", zap.Int64("image_size", blockMap.Size()),
				zap.Bool("can_read_direct", canDirectRead))

			if canDirectRead {
				log.G(ctx).Debug("Select http image reader: Direct reader")
				return urlImgReader, nil
			} else {
				_ = urlImgReader.Close(ctx, errors.New("can't direct read image"))
				log.G(ctx).Debug("Select http image reader: NBD reader")
				return mdf.newURLMoveSrc(ctx, src, blockMap, getEtag)
			}
		} else {
			log.G(ctx).Error("Error while read map of blocks.", zap.Error(err))
			return nil, err
		}
	default:
		return nil, misc.ErrInvalidMoveSrc
	}
}

// GetDst builds a Dst from request.
func (mdf *DeviceFactory) GetDst(ctx context.Context, dst common.MoveDst, hint directreader.Hint) (resDst Dst, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	const dstType = "dst_type"
	span.SetTag(dstType, "unknown")

	switch dst := dst.(type) {
	case *common.SnapshotMoveDst:
		span.SetTag(dstType, "snapshot")
		return mdf.newSnapshotMoveDst(ctx, dst, hint, mdf.conf.General.ChecksumAlgorithm)
	case *common.NbsMoveDst:
		span.SetTag(dstType, "nbs")
		return mdf.newNbsMoveDst(ctx, dst)
	case *common.NullMoveDst:
		span.SetTag(dstType, "null")
		return mdf.newNullMoveDst(ctx, dst)
	default:
		return nil, misc.ErrInvalidMoveDst
	}
}

// RegisterMon adds monitoring checks.
func (mdf *DeviceFactory) RegisterMon(r mon.Repository) error {
	for clusterID, cfg := range mdf.conf.Nbs {
		cID := clusterID
		name := fmt.Sprintf("NBS.%s[%s]", cID, cfg.Hosts[0])
		r.Add(name, mon.CheckFunc(func(ctx context.Context) error {
			// We want to return well-format error on retry timeout.
			// So we use the timeout smaller than check's.
			ctx, cancel := context.WithTimeout(ctx, nbsCheckTimeout)
			defer cancel()

			logger := log.G(ctx).With(zap.String("cluster_id", cID))

			nbsClient, err := mdf.GetNbsClient(ctx, cID)
			if err != nil {
				logger.Error("Can't get nbs client from cache.", zap.Error(err))
				return err
			}
			defer func() {
				err = nbsClient.Close()
				if err != nil {
					logger.Error("Close ping nbs_client failed.", zap.Error(err))
				}
			}()
			err = nbsClient.Ping(ctx)
			if err != nil {
				logger.Error("Ping nbs_client failed.", zap.Error(err))
				return err
			}
			return nil
		}))
	}
	return nil
}

// Close releases the resources.
func (mdf *DeviceFactory) Close(ctx context.Context) error {
	// Clients no more need to be Closed
	return nil
}
