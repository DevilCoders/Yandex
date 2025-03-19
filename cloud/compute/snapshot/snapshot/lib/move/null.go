package move

import (
	"math/rand"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"

	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

type nullDevice struct {
	size          int64
	blockSize     int64
	random        bool
	closeCallback func(context.Context, error)
}

func (nd *nullDevice) GetBlockSize() int64 {
	return nd.blockSize
}

func (nd *nullDevice) GetSize() int64 {
	return nd.size
}

func (nd *nullDevice) ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	span, _ := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	if offset%nd.blockSize != 0 {
		return false, false, misc.ErrInvalidOffset
	}

	if int64(len(data))%nd.blockSize != 0 {
		return false, false, misc.ErrInvalidBlockSize
	}

	if !nd.random {
		return true, true, nil
	}

	_, err = rand.Read(data)
	return true, false, err
}

func (nd *nullDevice) WriteAt(ctx context.Context, offset int64, data []byte) error {
	span, _ := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, nil) }()

	return nil
}

func (nd *nullDevice) WriteZeroesAt(ctx context.Context, offset int64, size int) error {
	span, _ := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, nil) }()

	return nil
}

func (nd *nullDevice) GetHint() directreader.Hint {
	return directreader.Hint{Size: nd.GetSize()}
}

func (nd *nullDevice) Close(ctx context.Context, err error) error {
	if nd.closeCallback != nil {
		nd.closeCallback(ctx, err)
	}
	return nil
}

func (mdf *DeviceFactory) newNullMoveSrc(_ context.Context, src *common.NullMoveSrc) (Src, error) {
	return &nullDevice{size: src.Size, blockSize: src.BlockSize, random: src.Random}, nil
}

func (mdf *DeviceFactory) newNullMoveDst(_ context.Context, dst *common.NullMoveDst) (Dst, error) {
	return &nullDevice{size: dst.Size, blockSize: dst.BlockSize, closeCallback: dst.CloseCallback}, nil
}
