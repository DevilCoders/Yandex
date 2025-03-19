package directreader

import (
	"context"
	"errors"
	"fmt"
	"io"
	"net/http"
	"sort"

	otlog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

// read From start position until end of image
// main reason for this abstraction: direct continuesly read of large blocks in image.
type imgSimpleReader struct {
	m               BlocksMap
	imagePosition   int64
	currentMapIndex int

	closed      bool
	blockReader io.ReadCloser
	httpDo      func(req *http.Request) (*http.Response, error) // need for testing now and customization in future
}

func newImgSimpleReader(m BlocksMap, imageOffset int64, httpClient *http.Client) *imgSimpleReader {
	do := http.DefaultClient.Do
	if httpClient != nil {
		do = httpClient.Do
	}
	res := &imgSimpleReader{
		m:      m,
		httpDo: do,
	}
	res.SetOffset(imageOffset)
	return res
}

// Close reader
func (r *imgSimpleReader) Close() error {
	if r.closed {
		return errors.New("reader closed already")
	}
	r.closed = true

	if r.blockReader != nil {
		return r.blockReader.Close()
	}

	return nil
}

func (r *imgSimpleReader) SetOffset(offset int64) {
	index := sort.Search(len(r.m.items), func(i int) bool {
		return r.m.items[i].Start+r.m.items[i].Length > offset
	})
	r.currentMapIndex = index
	r.imagePosition = offset
	if r.blockReader != nil {
		_ = r.blockReader.Close()
		r.blockReader = nil
	}
}

// Read current block of data until end. It can be less, then len(data).
// if return zero = true - content of data undefined, don't use/trust it and doesn't think about it will zeroes.
// it can return readedBytes < len(data) and err == nil.
func (r *imgSimpleReader) Read(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error) {

	if r.closed {
		return 0, false, false, errors.New("read from closed img reader")
	}
	if r.currentMapIndex >= len(r.m.items) {
		return 0, false, false, io.EOF
	}

	mapItem := r.m.items[r.currentMapIndex]
	exists = mapItem.Data
	zero = mapItem.Zero || !exists

	bytesToRead := maxBytesToRead(mapItem, r.imagePosition, len(data))

	if zero {
		readedBytes = bytesToRead
		r.movePosition(readedBytes)
		return
	}

	return r.readFromImage(ctx, data[:bytesToRead])
}

func (r *imgSimpleReader) movePosition(n int) {
	r.imagePosition += int64(n)
	for r.currentMapIndex < len(r.m.items) &&
		r.m.items[r.currentMapIndex].Start+r.m.items[r.currentMapIndex].Length <= r.imagePosition {
		r.currentMapIndex++
	}
}

func (r *imgSimpleReader) readFromImage(ctx context.Context, data []byte) (readedBytes int, exists, zero bool, err error) {
	block := r.m.items[r.currentMapIndex]
	offsetInBlock := currentOffsetInBlock(block, r.imagePosition)
	firstByteIndexInImage := *block.RawImageOffset + offsetInBlock
	lastByteIndexInImage := *block.RawImageOffset + block.Length - 1

	var outerErr error
	_ = misc.Retry(ctx, "read-image-content", func() error {
		if r.blockReader == nil {
			r.blockReader, outerErr = httpGetBody(ctx, r.httpDo, r.m.url, firstByteIndexInImage, lastByteIndexInImage, r.m.etag)
			if outerErr != nil {
				// no retry because httpGetBody retry itself
				return err
			}
		}

		readedBytes, outerErr = fullReadOrCloseCtx(ctx, r.blockReader, data)
		if outerErr != nil && !(outerErr == io.EOF && offsetInBlock+int64(readedBytes) == block.Length) {
			log.G(ctx).Error("Error while read image content from opened connection. Close connection and retry next read with new connection.",
				zap.Int64("first_byte_in_image", firstByteIndexInImage), zap.Int64("last_byte_in_image", lastByteIndexInImage),
				zap.Int64("logical_position", r.imagePosition), zap.Int("length", len(data)),
				zap.Int64("logical_last_byte_index", r.imagePosition+int64(len(data))-1),
				zap.Error(outerErr))
			localErr := r.blockReader.Close()
			log.G(ctx).Debug("Image connection closed", zap.Error(localErr))
			r.blockReader = nil
		}

		if offsetInBlock+int64(readedBytes) == block.Length && r.blockReader != nil {
			localErr := r.blockReader.Close()
			if localErr != nil {
				log.G(ctx).Error("Can't close read image connection", zap.Int64("position", r.imagePosition),
					zap.Error(localErr))
			}
			r.blockReader = nil
		}

		if readedBytes > 0 {
			outerErr = nil
			return nil
		}
		return misc.ErrInternalRetry
	})

	if outerErr != nil {
		return 0, false, false, outerErr
	}

	zero = true
	for i := 0; i < readedBytes; i++ {
		if data[i] != 0 {
			zero = false
			break
		}
	}

	r.movePosition(readedBytes)
	return readedBytes, true, zero, nil
}

func currentOffsetInBlock(block imageMapItem, currentPosition int64) int64 {
	return currentPosition - block.Start
}

func maxBytesToRead(block imageMapItem, currentImagePosition int64, requested int) int {

	availableBytesInBlock := block.Length - currentOffsetInBlock(block, currentImagePosition)

	if int64(requested) < availableBytesInBlock {
		return requested
	}
	return int(availableBytesInBlock)

}

// return exactly one of reader/error as not nil
// reader can stop it read by call Close from other goroutine
func httpGetBody(ctx context.Context, do func(req *http.Request) (*http.Response, error), url string, start, finish int64, etag string) (reader io.ReadCloser, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	requestedContentLength := finish - start + 1
	span.LogFields(otlog.Int64("start", start), otlog.Int64("finish", finish), otlog.Int64("length", finish-start+1))
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		// skip log url because it can private
		log.G(ctx).DPanic("Can't create http get request", zap.Error(err))
		return nil, err
	}

	if finish < start || start < 0 {
		log.G(ctx).DPanic("Bad start finish range for http range request", zap.Int64("start", start),
			zap.Int64("finish", finish))
		return nil, errors.New("bad start-finish range for http request")
	}

	requestContext, requestCancelContext := context.WithCancel(context.Background())

	req = req.WithContext(requestContext)
	defer func() {
		if err != nil {
			log.G(ctx).Debug("Canceling ctx due to error in httpGetBody in imagereader...")
			requestCancelContext()
		}
	}()

	req.Header.Set("RANGE", fmt.Sprintf("bytes=%v-%v", start, finish))
	var resp *http.Response

	var outerErr error
	_ = misc.Retry(ctx, "http-request", func() error {
		resp, outerErr = do(req)
		if outerErr != nil {
			// skip log url because it can private
			log.G(ctx).Error("Error while http request range", zap.Int64("start", start),
				zap.Int64("finish", finish))
			return misc.ErrInternalRetry
		}
		logger := log.G(ctx).With(zap.Int("status_code", resp.StatusCode), zap.String("status", resp.Status), zap.Reflect("headers", resp.Header))

		// Retry temporary client errors
		if resp.StatusCode == http.StatusTooManyRequests || resp.StatusCode == http.StatusLocked || resp.StatusCode == http.StatusRequestTimeout {
			logger.Error("Client has temporary error while http range request")
			outerErr = errors.New("client temporary error while http range request")
			return misc.ErrInternalRetry
		}

		// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
		if resp.StatusCode >= 400 && resp.StatusCode <= 499 {
			logger.Error("Client error while http range request")
			outerErr = misc.ErrUnreachableSource
			if resp.StatusCode == 404 {
				outerErr = misc.ErrSourceURLNotFound
			}
			return outerErr
		}

		// Retry server errors
		if resp.StatusCode >= 500 && resp.StatusCode <= 599 {
			logger.Error("Server error while http range request.")

			outerErr = errors.New("server error while http range request")
			return misc.ErrInternalRetry
		}

		if resp.Header.Get("Etag") != etag {
			logger.Error("Wrong etag found", zap.String("etag", resp.Header.Get("Etag")))
			outerErr = misc.ErrSourceChanged
			return misc.ErrSourceChanged
		}
		return nil
	})
	if outerErr != nil {
		return nil, outerErr
	}

	if resp.StatusCode == http.StatusRequestedRangeNotSatisfiable {
		log.G(ctx).Error("Requested range doesn't satisfiable")
		return nil, errors.New("requested range doesn't satisfiable")
	}
	if resp.ContentLength != requestedContentLength {
		log.G(ctx).Error("Server return bad length of data", zap.Int64("start", start),
			zap.Int64("finish", finish), zap.Int64("need_len", requestedContentLength),
			zap.Int64("actual_content_length", resp.ContentLength))
		return nil, errors.New("request bad content length")
	}
	return httpReaderCancelContext{ctxCancel: func() {
		log.G(ctx).Debug("Canceling ctx due to Cancel() of httpReaderCancelContext ...")
		requestCancelContext()
	}, ioReadCloser: resp.Body}, nil
}

type httpReaderCancelContext struct {
	ctxCancel    func()        // context cancel function, paired with request context - for force stop reader
	ioReadCloser io.ReadCloser // body of http response
}

func (r httpReaderCancelContext) Read(p []byte) (n int, err error) {
	return r.ioReadCloser.Read(p)
}

func (r httpReaderCancelContext) Close() error {
	r.ctxCancel() // force stop request
	return r.ioReadCloser.Close()
}

// Close ioReadCloser if ctx canceled earlier then read finished
func fullReadOrCloseCtx(ctx context.Context, ioReadCloser io.ReadCloser, data []byte) (bytes int, err error) {
	readDoneCtx, readDone := context.WithCancel(context.Background())
	defer readDone()

	go func() {
		select {
		case <-readDoneCtx.Done():
			return
		case <-ctx.Done():
			closeErr := ioReadCloser.Close()
			if closeErr != nil {
				log.G(ctx).Error("Can't close image reader", zap.Error(closeErr))
			}
		}
	}()

	bytes, err = ioReadCloser.Read(data)

	if e := ctx.Err(); e != nil {
		err = e
	}

	return
}
