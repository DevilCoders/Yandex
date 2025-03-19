package chunks

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
)

////////////////////////////////////////////////////////////////////////////////

// Interface for communicating with different chunk storages.
type Storage interface {
	ReadChunk(
		ctx context.Context,
		chunk *common.Chunk,
	) (err error)

	WriteChunk(
		ctx context.Context,
		referer string,
		chunkID string,
		data []byte,
		compression string,
	) (compressedDataLen int, err error)

	RefChunk(
		ctx context.Context,
		referer string,
		chunkID string,
	) (err error)

	UnrefChunk(
		ctx context.Context,
		referer string,
		chunkID string,
	) (err error)
}
