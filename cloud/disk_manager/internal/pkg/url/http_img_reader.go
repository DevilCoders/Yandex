package url

import (
	"bytes"
	"compress/flate"
	"context"
	"fmt"
	"io"
	"net/http"
	"sort"

	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/mapitem"
)

////////////////////////////////////////////////////////////////////////////////

type httpImageReader struct {
	imageMap      ImageMap
	offset        uint64
	imageMapIndex int

	httpClient *http.Client
}

func (r *httpImageReader) SetOffset(offset uint64) {
	index := sort.Search(len(r.imageMap.items), func(i int) bool {
		return r.imageMap.items[i].Start+r.imageMap.items[i].Length > int64(offset)
	})
	r.imageMapIndex = index
	r.offset = offset
}

// Read current block of data until end. It can be less, then len(data).
// If return zero = true - content of data undefined, don't use/trust it and doesn't think about it will zeroes.
// It can return readBytes < len(data) and err == nil.
func (r *httpImageReader) Read(
	ctx context.Context,
	data []byte,
) (readBytes int, zero bool, err error) {

	if r.imageMapIndex >= len(r.imageMap.items) {
		return 0, false, io.EOF
	}

	mapItem := r.imageMap.items[r.imageMapIndex]
	zero = mapItem.Zero || !mapItem.Data

	bytesToRead := maxBytesToRead(mapItem, r.offset, len(data))

	if zero {
		readBytes = bytesToRead
		r.movePosition(readBytes)
		return
	}

	block := r.imageMap.items[r.imageMapIndex]

	if block.CompressedOffset != nil {
		return r.readCompressed(ctx, data[:bytesToRead])
	} else if block.RawImageOffset != nil {
		return r.readRaw(ctx, data[:bytesToRead])
	} else {
		return 0, false, &task_errors.NonRetriableError{
			Err: fmt.Errorf("unknown block type %v", block),
		}
	}
}

func (r *httpImageReader) movePosition(n int) {
	r.offset += uint64(n)
	for r.imageMapIndex < len(r.imageMap.items) &&
		r.imageMap.items[r.imageMapIndex].Start+r.imageMap.items[r.imageMapIndex].Length <= int64(r.offset) {
		r.imageMapIndex++
	}
}

func (r *httpImageReader) readRaw(
	ctx context.Context,
	data []byte,
) (readBytes int, zero bool, err error) {

	block := r.imageMap.items[r.imageMapIndex]

	offsetInBlock := currentOffsetInBlock(block, r.offset)

	start := *block.RawImageOffset + offsetInBlock
	end := *block.RawImageOffset + block.Length - 1

	blockReader, err := common.HTTPGetBody(
		ctx,
		r.httpClient,
		r.imageMap.url,
		start,
		end,
		r.imageMap.etag,
	)
	if err != nil {
		return 0, false, err
	}
	defer blockReader.Close()

	readBytes, err = r.readFull(blockReader, block.Length-offsetInBlock, data)
	if err != nil {
		return 0, false, err
	}

	zero = true
	for i := 0; i < readBytes; i++ {
		if data[i] != 0 {
			zero = false
			break
		}
	}

	r.movePosition(readBytes)
	return readBytes, zero, nil
}

func (r *httpImageReader) readCompressed(
	ctx context.Context,
	data []byte,
) (readBytes int, zero bool, err error) {

	block := r.imageMap.items[r.imageMapIndex]

	offsetInBlock := currentOffsetInBlock(block, r.offset)
	if offsetInBlock != 0 {
		return 0, false, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"can not read compressed block from non zero offset: block %v, offset %d",
				block,
				offsetInBlock),
		}
	}

	start := int64(*block.CompressedOffset)
	end := start + int64(*block.CompressedSize) - 1

	bodyReader, err := common.HTTPGetBody(
		ctx,
		r.httpClient,
		r.imageMap.url,
		start,
		end,
		r.imageMap.etag,
	)
	if err != nil {
		return 0, false, err
	}
	defer bodyReader.Close()

	buf := make([]byte, *block.CompressedSize)

	readBytes, err = r.readFull(bodyReader, int64(*block.CompressedSize), buf)
	if err != nil {
		return 0, false, err
	}

	zreader := flate.NewReader(bytes.NewReader(buf))
	readBytes, err = io.ReadFull(zreader, data)
	if err != nil {
		return 0, false, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"fail to inflate image range [%d:%d]: %v",
				start,
				end,
				err,
			),
		}
	}

	r.movePosition(readBytes)
	return readBytes, false, nil
}

func (r *httpImageReader) readFull(
	reader io.ReadCloser,
	expectedBytes int64,
	buf []byte,
) (int, error) {

	readBytes, err := io.ReadFull(reader, buf)
	if err != nil && !(err == io.EOF && int64(readBytes) == expectedBytes) {
		return 0, &task_errors.RetriableError{
			Err: fmt.Errorf(
				"error while read image content from opened connection: %v",
				err,
			),
		}
	}

	if readBytes == 0 {
		return 0, &task_errors.RetriableError{
			Err: fmt.Errorf("read 0 bytes from server, try again"),
		}
	}

	return readBytes, nil
}

func currentOffsetInBlock(item mapitem.ImageMapItem, currentPosition uint64) int64 {
	return int64(currentPosition) - item.Start
}

func maxBytesToRead(item mapitem.ImageMapItem, currentImagePosition uint64, requested int) int {
	availableBytesInBlock := item.Length - currentOffsetInBlock(item, currentImagePosition)

	if int64(requested) < availableBytesInBlock {
		return requested
	}

	return int(availableBytesInBlock)
}

////////////////////////////////////////////////////////////////////////////////

func newHTTPImgReader(imageMap ImageMap, httpClient *http.Client) *httpImageReader {
	return &httpImageReader{
		imageMap:   imageMap,
		httpClient: httpClient,
	}
}
