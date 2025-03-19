package url

import (
	"context"
	"fmt"
	"net/http"

	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/mapitem"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/qcow2"
	error_codes "a.yandex-team.ru/cloud/disk_manager/pkg/client/codes"
)

////////////////////////////////////////////////////////////////////////////////

type imageMapReader interface {
	ReadImageMap(context.Context) (<-chan mapitem.ImageMapItem, <-chan error)
	Size() uint64
}

type ImageMapReader struct {
	url           string
	httpClient    *http.Client
	etag          string
	contentLength int64

	reader imageMapReader
}

type ChunkMapEntry struct {
	imageMap   *ImageMap
	chunkIndex uint32
}

func (i *ImageMapReader) Size() uint64 {
	return i.reader.Size()
}

func (i *ImageMapReader) readHeader(
	ctx context.Context,
) error {

	resp, err := i.httpClient.Head(i.url)
	if err != nil {
		// NBS-3324: should it be retriable?
		return &task_errors.RetriableError{
			Err: fmt.Errorf("failed to head request: %v", err),
		}
	}

	err = resp.Body.Close()
	if err != nil {
		// NBS-3324: should it be retriable?
		return &task_errors.RetriableError{
			Err: fmt.Errorf("failed to close request body: %v", err),
		}
	}

	if resp.StatusCode >= 400 && resp.StatusCode <= 499 {
		return &task_errors.DetailedError{
			Err: fmt.Errorf("url source not found: code %d", resp.StatusCode),
			Details: &task_errors.ErrorDetails{
				Code:     error_codes.BadSource,
				Message:  "url source not found",
				Internal: false,
			},
		}
	}

	if resp.StatusCode >= 500 && resp.StatusCode <= 599 {
		return &task_errors.RetriableError{
			Err: fmt.Errorf(
				"server error while http range request: code %d",
				resp.StatusCode,
			),
		}
	}

	i.etag = resp.Header.Get("Etag")
	i.contentLength = resp.ContentLength

	qcow2Reader := qcow2.NewQCOW2Reader(
		i.url,
		i.httpClient,
		uint64(i.contentLength),
		i.etag,
	)

	isQCOW2, err := qcow2Reader.ReadHeader(ctx)
	if err != nil {
		return err
	}

	if isQCOW2 {
		i.reader = qcow2Reader
	} else {
		i.reader = NewRawImageReader(i.contentLength)
	}

	return nil
}

func (i *ImageMapReader) Read(
	ctx context.Context,
	chunkSize uint64,
) (<-chan ChunkMapEntry, <-chan error) {

	entries := make(chan ChunkMapEntry)
	errors := make(chan error, 1)

	items, mapErrors := i.reader.ReadImageMap(ctx)

	go func() {
		defer close(entries)
		defer close(errors)

		var imageMap ImageMap
		imageMap.url = i.url
		imageMap.etag = i.etag

		var more bool
		var item mapitem.ImageMapItem

		chunkIndex := uint32(0)
		more = true

		for more {
			select {
			case item, more = <-items:
				if !more {
					break
				}

				imageMap.items = append(imageMap.items, item)

				for {
					imap, err := imageMap.CutChunk(chunkSize)
					if err != nil {
						errors <- err
						return
					}

					if imap == nil {
						break
					}

					entry := ChunkMapEntry{
						imageMap:   imap,
						chunkIndex: chunkIndex,
					}

					select {
					case entries <- entry:
						chunkIndex++
					case <-ctx.Done():
						errors <- ctx.Err()
						return
					}
				}
			case <-ctx.Done():
				errors <- ctx.Err()
				return
			}
		}

		imap, err := imageMap.CutChunk(0)
		if err != nil {
			errors <- err
			return
		}

		if imap != nil {
			entry := ChunkMapEntry{
				imageMap:   &imageMap,
				chunkIndex: chunkIndex,
			}

			select {
			case entries <- entry:
			case <-ctx.Done():
				errors <- ctx.Err()
				return
			}
		}

		err = <-mapErrors
		if err != nil {
			errors <- err
			return
		}
	}()

	return entries, errors
}

func (i *ImageMapReader) ReadingQCOW2() bool {
	_, readingQCOW2 := i.reader.(*qcow2.QCOW2Reader)
	return readingQCOW2
}

////////////////////////////////////////////////////////////////////////////////

func NewImageMapReader(
	ctx context.Context,
	url string,
) (*ImageMapReader, error) {

	reader := ImageMapReader{
		url:        url,
		httpClient: http.DefaultClient,
	}

	err := reader.readHeader(ctx)
	if err != nil {
		return nil, err
	}

	return &reader, nil
}
