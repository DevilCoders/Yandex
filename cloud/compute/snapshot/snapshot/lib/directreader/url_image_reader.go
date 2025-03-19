package directreader

import (
	"context"
	"errors"
	"net/http"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const defaultBlockSize = 4 * misc.MB

// SequenceURLImageReader implement device.Src interface for direct read blocks from url
// If ReaderAt did in sequence window (readerCount*blockSize bytes)*2 (formula can change) - next ReaderAt wait until previous completed
// If ReaderAt did out of sequence window - initialize new underline readahead reader and return result
// If ReaderAt closed by context - next ReadAt will reinitialize underline reader and sequence window
type SequenceURLImageReader struct {
	readerCount int64 // for hint help
	blockMap    BlocksMap
	blockSize   int64
	reader      *readAtReadAhead

	mu     sync.RWMutex
	closed bool
}

// NewSequenceURLImageReader return Object, available to use
// url - url of image
// blockMap - map of blocks
// blockSize - size of block for report, no used internal. -1 mean default block size.
func NewSequenceURLImageReader(blockMap BlocksMap, blockSize int64, httpClient *http.Client) *SequenceURLImageReader {
	if blockSize == -1 {
		blockSize = defaultBlockSize
	}
	const readerCount = 4
	return &SequenceURLImageReader{
		readerCount: readerCount,
		blockMap:    blockMap,
		blockSize:   blockSize,
		reader:      newReadAtReadAhead(blockMap.Size(), blockSize, readerCount*2, newImgSimpleReader(blockMap, 0, httpClient)),
	}
}

// GetBlockSize return block size in bytes, used for SequenceURLImageReader
func (r *SequenceURLImageReader) GetBlockSize() int64 {
	return r.blockSize
}

// GetSize return logical size of image in bytes, it round up by block size.
// as ndbSrc
func (r *SequenceURLImageReader) GetSize() int64 {
	realSize := r.blockMap.Size()
	if realSize%r.blockSize == 0 {
		return realSize
	}
	return ((realSize / r.blockSize) + 1) * r.blockSize
}

// ReadAt MUST READ ALL DATA from start point withoit skip any blocks
// it may be from many goroutines safe, but all parts must read one by one.
// later ReadAt will block until earler consume they data.
func (r *SequenceURLImageReader) ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	r.mu.Lock()
	closed := r.closed
	r.mu.Unlock()

	logger := log.G(ctx)
	if closed {
		logger.Error("Try read data from closed http image reader")
		return false, false, errors.New("read from closed http reader")
	}
	t := time.Now()
	exists, zero, err = r.reader.ReadAt(ctx, offset, data)
	misc.ImageReadSpeed.ObserveContext(ctx, misc.SpeedSince(len(data), t))
	return
}

// Hint is a hint to be sent from Src to Dst.
type Hint struct {
	Size        int64
	ReaderCount int
}

// GetHint return hint for readers
func (r *SequenceURLImageReader) GetHint() Hint {
	return Hint{ReaderCount: int(r.readerCount), Size: r.GetSize()}
}

// Close SequenceURLImageReader
func (r *SequenceURLImageReader) Close(ctx context.Context, err error) error {
	r.mu.Lock()
	defer r.mu.Unlock()

	if r.closed {
		return errors.New("Close already closed device")
	}

	var errString string
	if err == nil {
		errString = "<nil>"
	} else {
		errString = err.Error()
	}
	log.G(ctx).Debug("Close SequenceURLImageReader with error", zap.String("with_error", errString))
	r.closed = true

	return r.reader.Close(ctx, err)
}

// CanReadDirect Check if reader can direct read full image
func (r *SequenceURLImageReader) CanReadDirect() bool {
	return r.blockMap.CanReadDirect()
}
