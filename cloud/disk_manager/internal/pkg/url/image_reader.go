package url

import (
	"context"
	"fmt"
	"io"
	"net/http"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

func memsetZeros(data []byte) {
	// Optimized idiom https://github.com/golang/go/wiki/CompilerOptimizations#optimized-memclr.
	for i := range data {
		data[i] = 0
	}
}

////////////////////////////////////////////////////////////////////////////////

type ImageReader struct {
	imgSize   uint64
	blockSize uint64

	reader *httpImageReader
}

func (r *ImageReader) GetBlockSize() uint64 {
	return r.blockSize
}

func (r *ImageReader) Read(
	ctx context.Context,
	offset uint64,
	chunk *common.Chunk,
) error {

	if offset == r.imgSize {
		return io.EOF
	}

	if offset > r.imgSize {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"read out of size: offset %d, imgSize %d",
				offset,
				r.imgSize,
			),
		}
	}

	if offset%r.blockSize != 0 {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"read not aligned to block size: offset %d, blockSize %d",
				offset,
				r.blockSize,
			),
		}
	}

	if uint64(len(chunk.Data)) != r.blockSize {
		return &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"read size not equal to block size: size %d, blockSize %d",
				len(chunk.Data),
				r.blockSize,
			),
		}
	}

	return r.read(ctx, offset, chunk)
}

func (r *ImageReader) read(
	ctx context.Context,
	offset uint64,
	chunk *common.Chunk,
) error {

	r.reader.SetOffset(offset)

	readBytes := 0
	chunk.Zero = true
	// Need to zero all unused bytes in data if there are some non-zeroes in item
	needZeroContent := false

	for readBytes < len(chunk.Data) {
		// TODO: may be many small http reads, optimize it https://st.yandex-team.ru/NBS-3251
		currReadBytes, currZero, err := r.reader.Read(ctx, chunk.Data[readBytes:])
		if err != nil && err != io.EOF {
			// In case of io.EOF we must set correct zero values.
			return err
		}
		if err == io.EOF && offset+r.blockSize < r.imgSize {
			return &task_errors.NonRetriableError{
				Err: fmt.Errorf(
					"reached end of file while reading [%d:%d]",
					offset,
					offset+r.blockSize,
				),
			}
		}

		chunk.Zero = chunk.Zero && currZero
		if !needZeroContent && !chunk.Zero {
			needZeroContent = true
			// Zero content before last block.
			memsetZeros(chunk.Data[:readBytes])
		}

		if needZeroContent && currZero {
			// Zero current data part.
			memsetZeros(chunk.Data[readBytes : readBytes+currReadBytes])
		}

		readBytes += currReadBytes
		if err == io.EOF {
			memsetZeros(chunk.Data[readBytes:])
			return nil
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewImageReader(
	imageMap ImageMap,
	blockSize uint64,
	httpClient *http.Client,
) *ImageReader {

	return &ImageReader{
		imgSize:   imageMap.Size(),
		blockSize: blockSize,
		reader:    newHTTPImgReader(imageMap, httpClient),
	}
}
