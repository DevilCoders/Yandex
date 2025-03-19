package move

import (
	"io"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/convert"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/nbd"
)

var _ Src = &nbdMoveSrc{}

type nbdMoveSrc struct {
	params convert.NbdParams
}

func newNbdMoveSrc(params convert.NbdParams) *nbdMoveSrc {
	return &nbdMoveSrc{params: params}
}

func (nms *nbdMoveSrc) GetSize() int64 {
	return nms.params.Size
}

func (nms *nbdMoveSrc) GetBlockSize() int64 {
	return nms.params.BlockSize
}

func (nms *nbdMoveSrc) GetHint() (mh directreader.Hint) {
	return directreader.Hint{
		Size:        nms.GetSize(),
		ReaderCount: nms.params.WorkersCount,
	}
}

func (nms *nbdMoveSrc) ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	t := misc.LoadChunkFromS3.Start()
	defer t.ObserveDuration()

	for i := 0; i < len(data)/nms.params.ChunkSize; i++ {
		var read int
		err := misc.Retry(ctx, "LoadChunk", func() error {
			off := offset + int64(i*nms.params.ChunkSize) + int64(read)
			chunkStart := i*nms.params.ChunkSize + read
			chunkEnd := (i + 1) * nms.params.ChunkSize
			n, err := nms.params.Device.ReadAt(data[chunkStart:chunkEnd], off)
			read += n
			if err != nil {
				if err == io.EOF {
					log.G(ctx).Info("Got io.EOF error while read nbd chunk.", zap.Int64("nbd_offset", off), zap.Error(err))
				} else {
					log.G(ctx).Error("Got error while read nbd chunk.", zap.Int64("nbd_offset", off), zap.Error(err))
				}
			}
			switch err {
			case nil, io.EOF:
				return err
			case io.ErrUnexpectedEOF:
				return misc.ErrInternalRetry
			default:
				// No need retry on closed device
				if nbd.IsClosedDeviceError(err) {
					return err
				}
				return misc.ErrInternalRetry
			}
		})

		switch err {
		case nil:
		case io.EOF:
			// Have we read everything?
			if offset+int64(i*nms.params.ChunkSize)+int64(read) == nms.params.Device.Size() {
				copy(data[i*nms.params.ChunkSize+read:], make([]byte, len(data)))
				return true, false, nil
			}
			log.G(ctx).Error("Got EOF", zap.Int64("nbd_offset", offset), zap.Error(err))
			return false, false, xerrors.Errorf("eof on nbd read offset=%d", offset)
		default:
			log.G(ctx).Error("Can't read data from nbd file", zap.Error(err), zap.Int64("nbd_offset", offset))
			return false, false, err
		}
	}

	return false, false, nil
}

func (nms *nbdMoveSrc) Close(ctx context.Context, _ error) error {
	return nms.params.Device.Close(ctx)
}
