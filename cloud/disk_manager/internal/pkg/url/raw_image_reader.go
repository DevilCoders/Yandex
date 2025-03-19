package url

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/mapitem"
)

////////////////////////////////////////////////////////////////////////////////

type RawImageReader struct {
	contentLength int64
}

func (r *RawImageReader) Size() uint64 {
	return uint64(r.contentLength)
}

func (r *RawImageReader) ReadImageMap(
	ctx context.Context,
) (<-chan mapitem.ImageMapItem, <-chan error) {

	items := make(chan mapitem.ImageMapItem, 1)
	errors := make(chan error)

	// Raw format - 1 continuous block.
	items <- mapitem.ImageMapItem{
		Start:          0,
		Length:         r.contentLength,
		Depth:          0,
		Zero:           false,
		Data:           true,
		RawImageOffset: new(int64),
	}

	close(items)
	close(errors)

	return items, errors
}

////////////////////////////////////////////////////////////////////////////////

func NewRawImageReader(contentLength int64) *RawImageReader {
	return &RawImageReader{
		contentLength: contentLength,
	}
}
