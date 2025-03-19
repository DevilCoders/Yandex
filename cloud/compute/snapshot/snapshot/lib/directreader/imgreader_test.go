package directreader

import (
	"bytes"
	"context"
	"errors"
	"io"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"strconv"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

func TestHttpGetBody(t *testing.T) {
	at := assert.New(t)
	ctx := log.WithLogger(context.Background(), zaptest.NewLogger(t, zaptest.WrapOptions(zap.Development())))
	ctx, ctxCancel := context.WithCancel(ctx)
	defer ctxCancel()

	testURL := "http://test/image"
	const rangeOkStart = 1
	const rangeOkFinish = 3
	bodyOk := []byte{1, 2, 3}

	doFuncOk := func(req *http.Request) (*http.Response, error) {
		at.Equal("bytes=1-3", req.Header.Get("RANGE"))
		r := httptest.NewRecorder()
		r.WriteHeader(200)
		r.Header().Set("Content-Length", "3")
		_, _ = r.Write(bodyOk)
		res := r.Result()
		res.ContentLength = 3
		return res, nil
	}

	// OK
	r, err := httpGetBody(ctx, doFuncOk, testURL, rangeOkStart, rangeOkFinish, "")
	at.NoError(err)
	body, _ := ioutil.ReadAll(r)
	_ = r.Close()
	at.Equal(bodyOk, body)

	// 429 Retry
	doFuncRetryCalled := true
	doFuncRetryOk := func(req *http.Request) (*http.Response, error) {
		if !doFuncRetryCalled {
			return doFuncOk(req)
		}

		doFuncRetryCalled = false
		r := httptest.NewRecorder()
		r.WriteHeader(http.StatusTooManyRequests)
		return r.Result(), nil
	}
	r, err = httpGetBody(ctx, doFuncRetryOk, testURL, rangeOkStart, rangeOkFinish, "")
	at.NoError(err)
	body, _ = ioutil.ReadAll(r)
	_ = r.Close()
	at.Equal(bodyOk, body)
}

func TestImgReader(t *testing.T) {
	var err error
	var readedBytes int
	var exists bool
	var zero bool
	var buf []byte
	var needReadError bool
	var needRequestError bool
	var requests []*http.Request
	var reader *imgSimpleReader

	p := func(v int64) *int64 {
		return &v
	}

	logRequests := func(req *http.Request) (*http.Response, error) {
		requests = append(requests, req)

		if needRequestError {
			return nil, errors.New("test error")
		}

		var bytesReader io.ReadCloser
		if needReadError {
			bytesReader = ioutil.NopCloser(bytes.NewReader(nil)) // will return EOF error
		} else {
			data := make([]byte, 10)
			for i := range data {
				data[i] = byte(i + 1)
			}
			bytesReader = ioutil.NopCloser(bytes.NewReader(data))
		}

		rangeHeader := req.Header.Get("RANGE")
		rangeHeader = strings.TrimPrefix(rangeHeader, "bytes=")
		parts := strings.Split(rangeHeader, "-")
		start, _ := strconv.Atoi(parts[0])
		end, _ := strconv.Atoi(parts[1])

		return &http.Response{
			StatusCode:    http.StatusPartialContent,
			Body:          bytesReader,
			ContentLength: int64(end - start + 1),
		}, nil
	}

	clean := func() {
		buf = make([]byte, 5)
		needReadError = false
		needRequestError = false
		requests = nil
	}

	a := assert.New(t)
	ctx := log.WithLogger(context.Background(), zap.NewNop())

	// OK zero start image with data
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 10, RawImageOffset: p(100), Zero: true, Data: true, Depth: 0},
		},
	}, 0, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(true, zero)
	a.NoError(err)
	a.Equal(0, len(requests))

	// OK zero start image
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 10, RawImageOffset: nil, Zero: true, Data: false, Depth: 0},
		},
	}, 0, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(false, exists)
	a.Equal(true, zero)
	a.NoError(err)
	a.Equal(0, len(requests))

	// OK read from start
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: p(100), Zero: false, Data: true, Depth: 0},
		},
	}, 0, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal(1, len(requests))
	a.Equal("bytes=100-199", requests[0].Header.Get("range"))
	a.Equal([]byte{1, 2, 3, 4, 5}, buf)

	// OK read from middle
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: p(100), Zero: false, Data: true, Depth: 0},
		},
	}, 50, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal(1, len(requests))
	a.Equal("bytes=150-199", requests[0].Header.Get("range"))
	a.Equal([]byte{1, 2, 3, 4, 5}, buf)

	// OK read from middle, then SetOffset and read from start
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: p(100), Zero: false, Data: true, Depth: 0},
		},
	}, 50, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal(1, len(requests))
	a.Equal("bytes=150-199", requests[0].Header.Get("range"))
	a.Equal([]byte{1, 2, 3, 4, 5}, buf)

	reader.SetOffset(0)
	for i := range buf {
		buf[i] = 0
	}
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal(2, len(requests))
	a.Equal("bytes=150-199", requests[0].Header.Get("range"))
	a.Equal("bytes=100-199", requests[1].Header.Get("range"))
	a.Equal([]byte{1, 2, 3, 4, 5}, buf)

	// OK end of block
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: p(100), Zero: false, Data: true, Depth: 0},
		},
	}, 98, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(2, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal(1, len(requests))
	a.Equal("bytes=198-199", requests[0].Header.Get("range"))
	a.Equal([]byte{1, 2}, buf[:2])

	// OK block seq
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: p(100), Zero: false, Data: true, Depth: 0},
			{Start: 100, Length: 20, RawImageOffset: p(400), Zero: false, Data: true, Depth: 0},
		},
	}, 98, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(2, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal([]byte{1, 2}, buf[:2])

	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal([]byte{1, 2, 3, 4, 5}, buf)

	a.Equal(2, len(requests))
	a.Equal("bytes=198-199", requests[0].Header.Get("range"))
	a.Equal("bytes=400-419", requests[1].Header.Get("range"))

	// OK zero, than data
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: true, Data: false, Depth: 0},
			{Start: 100, Length: 20, RawImageOffset: p(400), Zero: false, Data: true, Depth: 0},
		},
	}, 98, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(2, readedBytes)
	a.Equal(false, exists)
	a.Equal(true, zero)
	a.NoError(err)
	a.Equal([]byte{0, 0}, buf[:2])

	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal([]byte{1, 2, 3, 4, 5}, buf)

	a.Equal(1, len(requests))
	a.Equal("bytes=400-419", requests[0].Header.Get("range"))

	// OK zero, read twice from one block
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: p(100), Zero: false, Data: true, Depth: 0},
			{Start: 100, Length: 20, RawImageOffset: p(400), Zero: false, Data: true, Depth: 0},
		},
	}, 50, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf[:2])
	a.Equal(2, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal([]byte{1, 2}, buf[:2])

	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(5, readedBytes)
	a.Equal(true, exists)
	a.Equal(false, zero)
	a.NoError(err)
	a.Equal([]byte{3, 4, 5, 6, 7}, buf)

	a.Equal(1, len(requests))
	a.Equal("bytes=150-199", requests[0].Header.Get("range"))

	// empty map
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url:   "aaa",
		items: []imageMapItem{},
	}, 0, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(0, readedBytes)
	a.Equal(false, exists)
	a.Equal(false, zero)
	a.Equal(io.EOF, err)
	a.Equal(0, len(requests))

	// map with zero length
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 0, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 0, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(0, readedBytes)
	a.Equal(false, exists)
	a.Equal(false, zero)
	a.Equal(io.EOF, err)
	a.Equal(0, len(requests))

	// out of image size
	clean()
	reader = newImgSimpleReader(BlocksMap{
		url: "aaa",
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 100, nil)
	reader.httpDo = logRequests
	readedBytes, exists, zero, err = reader.Read(ctx, buf)
	a.Equal(0, readedBytes)
	a.Equal(false, exists)
	a.Equal(false, zero)
	a.Equal(io.EOF, err)
	a.Equal(0, len(requests))

}

func TestMovePosition(t *testing.T) {
	var r *imgSimpleReader
	a := assert.New(t)

	r = newImgSimpleReader(BlocksMap{
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 0, nil)
	r.movePosition(0)
	a.EqualValues(0, r.currentMapIndex)
	a.EqualValues(0, r.imagePosition)

	r = newImgSimpleReader(BlocksMap{
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 0, nil)
	r.movePosition(1)
	a.EqualValues(0, r.currentMapIndex)
	a.EqualValues(1, r.imagePosition)

	r = newImgSimpleReader(BlocksMap{
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
			{Start: 100, Length: 50, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 0, nil)
	r.movePosition(100)
	a.EqualValues(1, r.currentMapIndex)
	a.EqualValues(100, r.imagePosition)

	r = newImgSimpleReader(BlocksMap{
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
			{Start: 100, Length: 50, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 99, nil)
	a.EqualValues(0, r.currentMapIndex)
	a.EqualValues(99, r.imagePosition)
	r.movePosition(1)
	a.EqualValues(1, r.currentMapIndex)
	a.EqualValues(100, r.imagePosition)

	r = newImgSimpleReader(BlocksMap{
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
			{Start: 100, Length: 50, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 100, nil)
	a.EqualValues(1, r.currentMapIndex)
	a.EqualValues(100, r.imagePosition)
	r.movePosition(1)
	a.EqualValues(1, r.currentMapIndex)
	a.EqualValues(101, r.imagePosition)

	r = newImgSimpleReader(BlocksMap{
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
			{Start: 100, Length: 50, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 149, nil)
	a.EqualValues(1, r.currentMapIndex)
	a.EqualValues(149, r.imagePosition)
	r.movePosition(1)
	a.EqualValues(2, r.currentMapIndex)
	a.EqualValues(150, r.imagePosition)

	r = newImgSimpleReader(BlocksMap{
		items: []imageMapItem{
			{Start: 0, Length: 100, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
			{Start: 100, Length: 50, RawImageOffset: nil, Zero: false, Data: false, Depth: 0},
		},
	}, 150, nil)
	a.EqualValues(2, r.currentMapIndex)
	a.EqualValues(150, r.imagePosition)
	r.movePosition(1000)
	a.EqualValues(2, r.currentMapIndex)
	a.EqualValues(1150, r.imagePosition)
}
