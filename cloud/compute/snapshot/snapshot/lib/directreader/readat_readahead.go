package directreader

import (
	"errors"
	"io"
	"sync"

	otlog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"

	"context"
)

type imgReadOffsetCloser interface {
	Read(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error)
	SetOffset(offset int64)
	Close() error
}

type imgCacheBlock struct {
	Offset       int64
	Exists, Zero bool
	Err          error
	Data         []byte
}

type readAtReadAhead struct {
	imgSize         int64
	blockSize       int64
	cacheBlockCount int

	mu                  sync.Mutex
	closed              bool
	cached              []imgCacheBlock
	reader              imgReadOffsetCloser
	currentReaderOffset int64
	readBlock           []byte
	blockPool           sync.Pool
}

func newReadAtReadAhead(imgSize int64, blockSize int64, cachedBlocks int, reader imgReadOffsetCloser) *readAtReadAhead {
	return &readAtReadAhead{
		imgSize:             imgSize,
		blockSize:           blockSize,
		cacheBlockCount:     cachedBlocks,
		reader:              reader,
		currentReaderOffset: -1,
		blockPool: sync.Pool{New: func() interface{} {
			block := make([]byte, blockSize)
			return &block
		}},
	}
}

func (r *readAtReadAhead) ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	r.mu.Lock()
	defer r.mu.Unlock()

	if r.closed {
		return false, false, errors.New("read from closed readahead")
	}

	if offset == r.imgSize {
		return false, false, io.EOF
	}

	if offset < 0 || offset > r.imgSize {
		return false, false, errors.New("read out of size")
	}

	if offset%r.blockSize != 0 {
		ctxlog.G(ctx).DPanic("Read not aligned to block size", zap.Int64("blocksize", r.blockSize),
			zap.Int64("offset", offset))
		return false, false, errors.New("read not aligned to block size")
	}

	if int64(len(data)) != r.blockSize {
		ctxlog.G(ctx).DPanic("Read size not equal to block size", zap.Int64("blocksize", r.blockSize),
			zap.Int("datalen", len(data)))
		return false, false, errors.New("read data len not equal to block size")
	}

	return r.readAt(ctx, offset, data)
}

// call always under mutex
func (r *readAtReadAhead) readAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	blockIndex := r.findBlock(offset)
	if blockIndex < 0 {
		r.fillCache(ctx, offset)
		blockIndex = r.findBlock(offset)
	}

	var block imgCacheBlock
	block, r.cached = cutImgCachedBlock(r.cached, blockIndex)
	if int64(len(block.Data)) == r.blockSize {
		copy(data, block.Data)
		r.blockPool.Put(&block.Data)
	}
	return block.Exists, block.Zero, block.Err
}

func (r *readAtReadAhead) findBlock(offset int64) int {
	for i := range r.cached {
		if r.cached[i].Offset == offset {
			return i
		}
	}
	return -1
}

func cutImgCachedBlock(slise []imgCacheBlock, index int) (imgCacheBlock, []imgCacheBlock) {
	block := slise[index]
	copy(slise[index:], slise[index+1:])
	slise = slise[:len(slise)-1]
	return block, slise
}

func (r *readAtReadAhead) Close(ctx context.Context, err error) error {
	r.mu.Lock()
	defer r.mu.Unlock()

	if r.closed {
		return errors.New("closed already")
	}

	r.closed = true
	if r.reader != nil {
		return r.reader.Close()
	}
	return nil
}

// always call under mutex
func (r *readAtReadAhead) fillCache(ctx context.Context, offset int64) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, nil) }()

	logger := ctxlog.G(ctx)
	logger.Debug("Fill cache", zap.Int64("offset", offset))
	newCached := make([]imgCacheBlock, 0, r.cacheBlockCount)
	var maxCachedOffset int64 = -1
	minOffsetFromOldCache := offset - r.blockSize*int64(r.cacheBlockCount-1)
	for _, block := range r.cached {
		if len(newCached) < (r.cacheBlockCount-1) && block.Offset >= minOffsetFromOldCache && block.Offset < offset {
			newCached = append(newCached, block)
			if block.Offset > maxCachedOffset {
				maxCachedOffset = block.Offset
			}
		}
	}
	r.cached = newCached

	if r.currentReaderOffset != offset {
		r.currentReaderOffset = offset
		r.reader.SetOffset(r.currentReaderOffset)
	}

	for len(r.cached) < r.cacheBlockCount && r.currentReaderOffset < r.imgSize {
		if r.readBlock == nil {
			r.readBlock = *r.blockPool.Get().(*[]byte)
		}
		var block = imgCacheBlock{Offset: r.currentReaderOffset}
		block.Exists, block.Zero, block.Err = readFull(ctx, r.reader, r.readBlock)
		if block.Err == io.EOF && r.currentReaderOffset+r.blockSize >= r.imgSize {
			block.Err = nil
		}
		logger.Debug("Read block from img", zap.Int64("offset", r.currentReaderOffset), zap.Error(block.Err))
		r.currentReaderOffset += r.blockSize
		if block.Exists && !block.Zero {
			block.Data = r.readBlock
			r.readBlock = nil
		}
		r.cached = append(r.cached, block)
	}
}

type reader interface {
	Read(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error)
}

//nolint:gocyclo
func readFull(ctx context.Context, reader reader, data []byte) (exists, zero bool, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	cnt := 0
	defer func() {
		span.LogFields(otlog.Int("read_iterations", cnt))
		tracing.Finish(span, err)
	}()

	readedBytes := 0
	zero = true
	exists = false
	needZeroContent := false // if some part of block exist and not zero - need full right content of data.

	for readedBytes < len(data) {
		cnt++
		if ctx.Err() != nil {
			return false, false, ctx.Err()
		}

		currentReadedBytes, currentExist, currentZero, err := reader.Read(ctx, data[readedBytes:])
		exists = exists || currentExist
		zero = zero && currentZero
		if !needZeroContent && exists && !zero {
			// First part with need content
			needZeroContent = true
			// zero begin of data, because if non zero flag - need right content for all bytes
			// optimized idiom https://github.com/golang/go/wiki/CompilerOptimizations#optimized-memclr
			zeroSlise := data[:readedBytes] // zeroed content before last block
			for i := range zeroSlise {
				zeroSlise[i] = 0
			}
		}

		if needZeroContent && (currentZero || !currentExist) {
			// force zero current data part
			// optimized idiom https://github.com/golang/go/wiki/CompilerOptimizations#optimized-memclr
			zeroSlise := data[readedBytes : readedBytes+currentReadedBytes]
			for i := range zeroSlise {
				zeroSlise[i] = 0
			}
		}

		readedBytes += currentReadedBytes
		if err != nil {
			// optimized idiom https://github.com/golang/go/wiki/CompilerOptimizations#optimized-memclr
			zeroSlise := data[readedBytes:] // zeroed content after readed bytes
			for i := range zeroSlise {
				zeroSlise[i] = 0
			}

			if err == io.EOF {
				return exists, zero, io.EOF
			}

			return false, false, err
		}
	}

	return exists, zero, nil
}
