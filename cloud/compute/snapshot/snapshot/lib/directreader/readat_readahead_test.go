package directreader

import (
	"context"
	"errors"
	"io"
	"sort"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

type readerMock struct {
	read      func(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error)
	setOffset func(offset int64)
	close     func() error
}

var _ imgReadOffsetCloser = &readerMock{}

func (r *readerMock) Read(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error) {
	return r.read(ctx, data)
}

func (r *readerMock) SetOffset(offset int64) {
	r.setOffset(offset)
}

func (r *readerMock) Close() error {
	return r.close()
}

func TestCutImgCachedBlock(t *testing.T) {
	at := assert.New(t)
	var slise []imgCacheBlock
	var block imgCacheBlock

	slise = []imgCacheBlock{{Offset: 1}, {Offset: 2}, {Offset: 3}}
	block, slise = cutImgCachedBlock(slise, 0)
	at.Equal([]imgCacheBlock{{Offset: 2}, {Offset: 3}}, slise)
	at.Equal(imgCacheBlock{Offset: 1}, block)

	slise = []imgCacheBlock{{Offset: 1}, {Offset: 2}, {Offset: 3}}
	block, slise = cutImgCachedBlock(slise, 1)
	at.Equal([]imgCacheBlock{{Offset: 1}, {Offset: 3}}, slise)
	at.Equal(imgCacheBlock{Offset: 2}, block)

	slise = []imgCacheBlock{{Offset: 1}, {Offset: 2}, {Offset: 3}}
	block, slise = cutImgCachedBlock(slise, 2)
	at.Equal([]imgCacheBlock{{Offset: 1}, {Offset: 2}}, slise)
	at.Equal(imgCacheBlock{Offset: 3}, block)

	slise = []imgCacheBlock{{Offset: 1}}
	block, slise = cutImgCachedBlock(slise, 0)
	at.Equal([]imgCacheBlock{}, slise)
	at.Equal(imgCacheBlock{Offset: 1}, block)

}

func TestReaderAtFillCache(t *testing.T) {
	at := assert.New(t)
	ctx := ctxlog.WithLogger(context.Background(), zap.NewNop())

	var reader *readAtReadAhead
	sortCache := func() {
		sort.Slice(reader.cached, func(i, j int) bool {
			return reader.cached[i].Offset < reader.cached[j].Offset
		})
	}

	var offsetCnt int
	var dataForRead []imgCacheBlock
	var rmCloseErr error
	var rmCloseCnt int

	clear := func() {
		offsetCnt = 0
		dataForRead = []imgCacheBlock{}
		rmCloseErr = nil
		rmCloseCnt = 0
	}

	rm := &readerMock{
		setOffset: func(newOffset int64) {
			offsetCnt++
		},
		read: func(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error) {
			block := dataForRead[0]
			dataForRead = dataForRead[1:]
			copy(data, block.Data)
			return len(block.Data), block.Exists, block.Zero, block.Err
		},

		close: func() error {
			rmCloseCnt++
			return rmCloseErr
		},
	}

	//simple
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 3, 4, rm)

	reader.fillCache(ctx, 0)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 0},
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true, Offset: 3},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 6},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true, Offset: 9},
	}, reader.cached)
	at.EqualValues(12, reader.currentReaderOffset)

	// zero block
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{4, 5, 6}, Err: nil, Zero: true, Exists: true},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 3, 3, rm)

	reader.fillCache(ctx, 0)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 0},
		{Data: nil, Err: nil, Zero: true, Exists: true, Offset: 3},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 6},
	}, reader.cached)
	at.EqualValues(9, reader.currentReaderOffset)

	// unexisted block
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: false},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 3, 3, rm)

	reader.fillCache(ctx, 0)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 0},
		{Data: nil, Err: nil, Zero: false, Exists: false, Offset: 3},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 6},
	}, reader.cached)
	at.EqualValues(9, reader.currentReaderOffset)

	// partial read
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{3}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 3, 4, rm)

	reader.fillCache(ctx, 0)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 0},
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true, Offset: 3},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 6},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true, Offset: 9},
	}, reader.cached)
	at.EqualValues(12, reader.currentReaderOffset)

	// has one near block in cache
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 3, 4, rm)
	reader.cached = []imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 0},
	}
	reader.fillCache(ctx, 3)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 0},
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true, Offset: 3},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 6},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true, Offset: 9},
	}, reader.cached)
	at.EqualValues(12, reader.currentReaderOffset)

	// has one far block in cache
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{13, 14, 15}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 3, 4, rm)
	reader.cached = []imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 0},
	}
	reader.fillCache(ctx, 50)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true, Offset: 50},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 53},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true, Offset: 56},
		{Data: []byte{13, 14, 15}, Err: nil, Zero: false, Exists: true, Offset: 59},
	}, reader.cached)
	at.EqualValues(62, reader.currentReaderOffset)

	// has two near and two overlap blocks in cache
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{11, 12, 13}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{14, 15, 16}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 3, 4, rm)
	reader.cached = []imgCacheBlock{
		{Data: []byte{4, 5, 6}, Err: nil, Zero: false, Exists: true, Offset: 40},
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 43},
		{Data: []byte{10, 11, 12}, Err: nil, Zero: false, Exists: true, Offset: 50},
		{Data: []byte{13, 14, 15}, Err: nil, Zero: false, Exists: true, Offset: 52},
	}
	reader.fillCache(ctx, 50)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{7, 8, 9}, Err: nil, Zero: false, Exists: true, Offset: 43},
		{Data: []byte{1, 2, 3}, Err: nil, Zero: false, Exists: true, Offset: 50},
		{Data: []byte{11, 12, 13}, Err: nil, Zero: false, Exists: true, Offset: 53},
		{Data: []byte{14, 15, 16}, Err: nil, Zero: false, Exists: true, Offset: 56},
	}, reader.cached)
	at.EqualValues(59, reader.currentReaderOffset)

	// read end of image
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 2, 4, rm)
	reader.fillCache(ctx, 96)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true, Offset: 96},
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true, Offset: 98},
	}, reader.cached)
	at.EqualValues(100, reader.currentReaderOffset)

	// error while read
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: io.ErrUnexpectedEOF, Zero: false, Exists: true},
		{Data: []byte{3, 4}, Err: io.ErrUnexpectedEOF, Zero: false, Exists: true},
	}

	reader = newReadAtReadAhead(100, 2, 4, rm)
	reader.fillCache(ctx, 96)
	sortCache()

	at.Equal([]imgCacheBlock{}, dataForRead)
	at.Equal([]imgCacheBlock{
		{Data: nil, Err: io.ErrUnexpectedEOF, Zero: false, Exists: false, Offset: 96},
		{Data: nil, Err: io.ErrUnexpectedEOF, Zero: false, Exists: false, Offset: 98},
	}, reader.cached)
	at.EqualValues(100, reader.currentReaderOffset)
}

func TestReaderAt_ReadAt(t *testing.T) {
	at := assert.New(t)
	ctx := ctxlog.WithLogger(context.Background(), zap.NewNop())

	var reader *readAtReadAhead
	var data []byte
	sortCache := func() {
		sort.Slice(reader.cached, func(i, j int) bool {
			return reader.cached[i].Offset < reader.cached[j].Offset
		})
	}

	var offset int64
	var offsetCnt int
	var dataForRead []imgCacheBlock
	var rmCloseErr error
	var rmCloseCnt int

	clear := func() {
		offset = 0
		offsetCnt = 0
		dataForRead = []imgCacheBlock{}
		rmCloseErr = nil
		rmCloseCnt = 0
	}

	rm := &readerMock{
		setOffset: func(newOffset int64) {
			offset = newOffset
			offsetCnt++
		},
		read: func(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error) {
			block := dataForRead[0]
			dataForRead = dataForRead[1:]
			copy(data, block.Data)
			return len(block.Data), block.Exists, block.Zero, block.Err
		},

		close: func() error {
			rmCloseCnt++
			return rmCloseErr
		},
	}

	var exist, zero bool
	var err error

	// read from start
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true},
	}
	data = make([]byte, 2)
	reader = newReadAtReadAhead(100, 2, 4, rm)
	exist, zero, err = reader.ReadAt(ctx, 0, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.EqualValues(0, offset)
	at.EqualValues(1, offsetCnt)
	at.Equal([]byte{1, 2}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true, Offset: 2},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true, Offset: 4},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true, Offset: 6},
	}, reader.cached)

	// read from middle
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true},
	}
	data = make([]byte, 2)
	reader = newReadAtReadAhead(100, 2, 4, rm)
	exist, zero, err = reader.ReadAt(ctx, 50, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.Equal([]byte{1, 2}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true, Offset: 52},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true, Offset: 54},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true, Offset: 56},
	}, reader.cached)

	// read from start, then read from cache
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true},
	}
	data = make([]byte, 2)
	reader = newReadAtReadAhead(100, 2, 4, rm)
	exist, zero, err = reader.ReadAt(ctx, 0, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.EqualValues(0, offset)
	at.EqualValues(1, offsetCnt)
	at.Equal([]byte{1, 2}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true, Offset: 2},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true, Offset: 4},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true, Offset: 6},
	}, reader.cached)

	exist, zero, err = reader.ReadAt(ctx, 4, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.EqualValues(0, offset)
	at.EqualValues(1, offsetCnt)
	at.Equal([]byte{5, 6}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true, Offset: 2},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true, Offset: 6},
	}, reader.cached)

	// read from start, then read from middle
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true},
	}
	data = make([]byte, 2)
	reader = newReadAtReadAhead(100, 2, 4, rm)
	exist, zero, err = reader.ReadAt(ctx, 0, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.EqualValues(0, offset)
	at.EqualValues(1, offsetCnt)
	at.Equal([]byte{1, 2}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true, Offset: 2},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true, Offset: 4},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true, Offset: 6},
	}, reader.cached)

	dataForRead = []imgCacheBlock{
		{Data: []byte{9, 10}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{11, 12}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{13, 14}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{15, 16}, Err: nil, Zero: false, Exists: true},
	}
	exist, zero, err = reader.ReadAt(ctx, 50, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.EqualValues(50, offset)
	at.EqualValues(2, offsetCnt)
	at.Equal([]byte{9, 10}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{11, 12}, Err: nil, Zero: false, Exists: true, Offset: 52},
		{Data: []byte{13, 14}, Err: nil, Zero: false, Exists: true, Offset: 54},
		{Data: []byte{15, 16}, Err: nil, Zero: false, Exists: true, Offset: 56},
	}, reader.cached)

	// read from middle, then read from start
	clear()
	dataForRead = []imgCacheBlock{
		{Data: []byte{1, 2}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true},
	}
	data = make([]byte, 2)
	reader = newReadAtReadAhead(100, 2, 4, rm)
	exist, zero, err = reader.ReadAt(ctx, 50, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.EqualValues(50, offset)
	at.EqualValues(1, offsetCnt)
	at.Equal([]byte{1, 2}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{3, 4}, Err: nil, Zero: false, Exists: true, Offset: 52},
		{Data: []byte{5, 6}, Err: nil, Zero: false, Exists: true, Offset: 54},
		{Data: []byte{7, 8}, Err: nil, Zero: false, Exists: true, Offset: 56},
	}, reader.cached)

	dataForRead = []imgCacheBlock{
		{Data: []byte{9, 10}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{11, 12}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{13, 14}, Err: nil, Zero: false, Exists: true},
		{Data: []byte{15, 16}, Err: nil, Zero: false, Exists: true},
	}
	exist, zero, err = reader.ReadAt(ctx, 0, data)
	at.True(exist)
	at.False(zero)
	at.NoError(err)
	at.EqualValues(0, offset)
	at.EqualValues(2, offsetCnt)
	at.Equal([]byte{9, 10}, data)
	sortCache()
	at.Equal([]imgCacheBlock{
		{Data: []byte{11, 12}, Err: nil, Zero: false, Exists: true, Offset: 2},
		{Data: []byte{13, 14}, Err: nil, Zero: false, Exists: true, Offset: 4},
		{Data: []byte{15, 16}, Err: nil, Zero: false, Exists: true, Offset: 6},
	}, reader.cached)
}

type readCloserWriterOperation struct {
	Context   context.Context
	Operation string
	Data      []byte
}

type readCloserWriterMockReadResult struct {
	ReadedBytes int
	Exist       bool
	Zero        bool
	Err         error

	Data []byte

	Delay time.Duration
}

type readCloserWriterMock struct {
	mu          sync.Mutex
	operations  []readCloserWriterOperation
	readResults []readCloserWriterMockReadResult
}

func (r *readCloserWriterMock) Read(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error) {
	r.mu.Lock()
	defer r.mu.Unlock()

	r.operations = append(r.operations, readCloserWriterOperation{Context: ctx, Operation: "read", Data: data})

	if r.readResults == nil {
		return len(data), true, true, nil
	}

	if len(r.readResults) == 0 {
		panic("many reads")
	}
	res := r.readResults[0]

	if res.Delay > 0 {
		time.Sleep(res.Delay)
	}

	r.readResults = r.readResults[1:]
	copy(data, res.Data)
	return res.ReadedBytes, res.Exist, res.Zero, res.Err
}

func (r *readCloserWriterMock) Close() error {
	r.mu.Lock()
	defer r.mu.Unlock()

	r.operations = append(r.operations, readCloserWriterOperation{Operation: "close"})
	return nil
}

func TestReadFull(t *testing.T) {
	var mock *readCloserWriterMock
	var data []byte
	var exist bool
	var zero bool
	var err error

	clean := func() {
		mock = &readCloserWriterMock{}
		data = []byte{9, 8, 7, 6, 5}
	}

	ctx := ctxlog.WithLogger(context.Background(), zap.NewNop())
	a := assert.New(t)

	// ok simple
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 5, Exist: false, Zero: false, Err: nil, Data: nil},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(false, exist)
	a.Equal(false, zero)
	a.Equal(nil, err)

	// ok simple zero only true
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 5, Exist: false, Zero: true, Err: nil, Data: nil},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(false, exist)
	a.Equal(true, zero)
	a.Equal(nil, err)

	// ok simple exist only true
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 5, Exist: true, Zero: false, Err: nil, Data: []byte{1, 2, 3, 4, 5}},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(ctx, mock.operations[0].Context)
	a.Equal(true, exist)
	a.Equal(false, zero)
	a.Equal(nil, err)
	a.Equal([]byte{1, 2, 3, 4, 5}, data)

	// ok simple exist and zero true
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 5, Exist: true, Zero: true, Err: nil, Data: nil},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(true, exist)
	a.Equal(true, zero)
	a.Equal(nil, err)

	// ok non exist, then exist
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 1, Exist: false, Zero: false, Err: nil, Data: nil},
		{ReadedBytes: 4, Exist: true, Zero: false, Err: nil, Data: []byte{1, 2, 3, 4}},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(true, exist)
	a.Equal(false, zero)
	a.Equal(nil, err)
	a.Equal([]byte{0, 1, 2, 3, 4}, data)

	// ok non zero, then zero
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 4, Exist: true, Zero: false, Err: nil, Data: []byte{1, 2, 3, 4}},
		{ReadedBytes: 1, Exist: false, Zero: true, Err: nil, Data: nil},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(true, exist)
	a.Equal(false, zero)
	a.Equal(nil, err)
	a.Equal([]byte{1, 2, 3, 4, 0}, data)

	// ok zero in middle
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 2, Exist: true, Zero: false, Err: nil, Data: []byte{1, 2}},
		{ReadedBytes: 1, Exist: true, Zero: true, Err: nil, Data: []byte{}},
		{ReadedBytes: 2, Exist: true, Zero: false, Err: nil, Data: []byte{3, 4}},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(true, exist)
	a.Equal(false, zero)
	a.Equal(nil, err)
	a.Equal([]byte{1, 2, 0, 3, 4}, data)

	// ok data between zeroes
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 2, Exist: true, Zero: true, Err: nil, Data: []byte{}},
		{ReadedBytes: 1, Exist: true, Zero: false, Err: nil, Data: []byte{2}},
		{ReadedBytes: 2, Exist: true, Zero: true, Err: nil, Data: []byte{}},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(true, exist)
	a.Equal(false, zero)
	a.Equal(nil, err)
	a.Equal([]byte{0, 0, 2, 0, 0}, data)

	// error
	clean()
	mock.readResults = []readCloserWriterMockReadResult{
		{ReadedBytes: 4, Exist: true, Zero: false, Err: err, Data: []byte{1, 2, 3, 4}},
		{ReadedBytes: 0, Exist: false, Zero: true, Err: errors.New("asd"), Data: nil},
	}
	exist, zero, err = readFull(ctx, mock, data)
	a.Equal(false, exist)
	a.Equal(false, zero)
	a.EqualError(err, "asd")
}
