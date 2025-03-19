package url

import (
	"fmt"

	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/mapitem"
)

// ImageMap contains map of data blocks between logical offset and raw offset in image file.
type ImageMap struct {
	items []mapitem.ImageMapItem
	url   string
	etag  string
}

// Returns logical size of image, presented as map.
func (m *ImageMap) Size() uint64 {
	if len(m.items) == 0 {
		return 0
	}

	lastItem := m.items[len(m.items)-1]
	return uint64(lastItem.Start + lastItem.Length)
}

// If chunkSize is zero - returns entire ImageMap.
// Otherwise - returns |chunkSize|-sized ImageMap.
func (m *ImageMap) CutChunk(chunkSize uint64) (*ImageMap, error) {
	imageMap := ImageMap{
		items: make([]mapitem.ImageMapItem, 0),
		url:   m.url,
		etag:  m.etag,
	}

	if len(m.items) == 0 {
		return nil, nil
	}

	if chunkSize == 0 {
		imageMap.items = m.items
		m.items = nil
		return &imageMap, nil
	}

	lastItem := m.items[len(m.items)-1]
	if uint64(lastItem.Start+lastItem.Length-m.items[0].Start) < chunkSize {
		// not enough data for chunk
		return nil, nil
	}

	currSize := uint64(0)

	for currSize < chunkSize {
		if currSize+uint64(m.items[0].Length) <= chunkSize {
			imageMap.items = append(imageMap.items, m.items[0])
			currSize += uint64(m.items[0].Length)
			m.items = m.items[1:]
		} else {
			if m.items[0].CompressedOffset != nil {
				return nil, &task_errors.NonRetriableError{
					Err: fmt.Errorf("can not split compressed chunk"),
				}
			}

			neededBytes := int64(chunkSize - currSize)

			item := mapitem.ImageMapItem{
				Start:  m.items[0].Start,
				Length: neededBytes,
				Depth:  m.items[0].Depth,
				Zero:   m.items[0].Zero,
				Data:   m.items[0].Data,
			}

			if m.items[0].RawImageOffset != nil {
				item.RawImageOffset = new(int64)
				*item.RawImageOffset = *m.items[0].RawImageOffset
			}

			imageMap.items = append(imageMap.items, item)

			m.items[0].Start += neededBytes
			m.items[0].Length -= neededBytes
			if m.items[0].RawImageOffset != nil {
				*m.items[0].RawImageOffset += neededBytes
			}

			currSize += uint64(neededBytes)
		}
	}

	return &imageMap, nil
}
