package nbs

import (
	"fmt"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

func min(x, y uint64) uint64 {
	if x > y {
		return y
	}

	return x
}

func validate(blockCount uint64, chunkSize uint32, blockSize uint32) error {
	if chunkSize == 0 {
		return &errors.NonRetriableError{
			Err: fmt.Errorf("chunkSize should not be zero"),
		}
	}

	if blockSize == 0 {
		return &errors.NonRetriableError{
			Err: fmt.Errorf("blockSize should not be zero"),
		}
	}

	if chunkSize%blockSize != 0 {
		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"chunkSize should be multiple of blockSize, chunkSize=%v, blockSize=%v",
				chunkSize,
				blockSize,
			),
		}
	}

	blocksInChunk := uint64(chunkSize / blockSize)

	if blockCount%blocksInChunk != 0 {
		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"blockCount should be multiple of blocksInChunk, blockCount=%v, blocksInChunk=%v",
				blockCount,
				blocksInChunk,
			),
		}
	}

	return nil
}
