package tests

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"strconv"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/mapitem"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/qcow2"
	"a.yandex-team.ru/library/go/test/yatest"
)

////////////////////////////////////////////////////////////////////////////////

func getQCOW2ImageFileURLUbuntu1604() string {
	port := os.Getenv("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1604_IMAGE_FILE_SERVER_PORT")
	return fmt.Sprintf("http://localhost:%v", port)
}

func getQCOW2ImageSizeUbuntu1604(t *testing.T) uint64 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1604_IMAGE_FILE_SIZE"), 10, 64)
	require.NoError(t, err)
	return value
}

func getQCOW2ExpectedMapFileUbuntu1604() string {
	// Result of 'qemu-img map --output=json ubuntu1604-ci-stable'.
	return yatest.SourcePath("cloud/disk_manager/internal/pkg/url/qcow2/tests/data/qemuimg_map_ubuntu1604.json")
}

func getQCOW2ImageFileURLUbuntu1804() string {
	port := os.Getenv("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1804_IMAGE_FILE_SERVER_PORT")
	return fmt.Sprintf("http://localhost:%v", port)
}

func getQCOW2ImageSizeUbuntu1804(t *testing.T) uint64 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_QCOW2_UBUNTU1804_IMAGE_FILE_SIZE"), 10, 64)
	require.NoError(t, err)
	return value
}

func getQCOW2ExpectedMapFileUbuntu1804() string {
	// Result of 'qemu-img map --output=json ubuntu-18.04-minimal-cloudimg-amd64.img'.
	return yatest.SourcePath("cloud/disk_manager/internal/pkg/url/qcow2/tests/data/qemuimg_map_ubuntu1804.json")
}

func loadActualMapItems(t *testing.T, filepath string) []mapitem.ImageMapItem {
	var items []mapitem.ImageMapItem

	file, err := os.Open(filepath)
	require.NoError(t, err)

	data, err := ioutil.ReadAll(file)
	require.NoError(t, err)

	err = json.Unmarshal(data, &items)
	require.NoError(t, err)

	return items
}

func mergeCompressedImageMapItems(items []mapitem.ImageMapItem) []mapitem.ImageMapItem {
	var newItems []mapitem.ImageMapItem

	currentItem := items[0]

	for _, item := range items[1:] {
		if currentItem.SameContentTypeTo(item) &&
			currentItem.IsPrevTo(item) &&
			currentItem.RawIsPrevTo(item) {

			currentItem.Length += item.Length
		} else {
			currentItem.CompressedOffset = nil
			currentItem.CompressedSize = nil
			newItems = append(newItems, currentItem)
			currentItem = item
		}
	}

	currentItem.CompressedOffset = nil
	currentItem.CompressedSize = nil
	newItems = append(newItems, currentItem)

	return newItems
}

func mapAll(
	t *testing.T,
	reader *qcow2.QCOW2Reader,
	ctx context.Context,
) []mapitem.ImageMapItem {

	var mapItems []mapitem.ImageMapItem
	var err error

	for {
		items, errors := reader.ReadImageMap(ctx)

		more := true
		var item mapitem.ImageMapItem

		for more {
			select {
			case item, more = <-items:
				if !more {
					break
				}

				mapItems = append(mapItems, item)
			case <-ctx.Done():
				require.NoError(t, err)
			}
		}

		err = <-errors
		if !task_errors.CanRetry(err) {
			break
		}
	}

	require.NoError(t, err)

	return mapItems
}

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

////////////////////////////////////////////////////////////////////////////////

func testQCOW2MapImage(t *testing.T, url string, imageSize uint64, expectedMapFile string) {
	ctx := createContext()

	resp, err := http.DefaultClient.Head(url)
	require.NoError(t, err)
	require.Equal(t, http.StatusOK, resp.StatusCode)
	err = resp.Body.Close()
	require.NoError(t, err)

	etag := resp.Header.Get("Etag")

	qcow2Reader := qcow2.NewQCOW2Reader(url, http.DefaultClient, imageSize, etag)

	isQCOW2 := false

	for {
		isQCOW2, err = qcow2Reader.ReadHeader(ctx)
		if !task_errors.CanRetry(err) {
			break
		}
	}
	require.True(t, isQCOW2)
	require.NoError(t, err)

	mapItems := mapAll(t, qcow2Reader, ctx)
	mapItems = mergeCompressedImageMapItems(mapItems)
	expectedMapItems := loadActualMapItems(t, expectedMapFile)

	require.Equal(t, len(expectedMapItems), len(mapItems))
	require.Equal(t, expectedMapItems, mapItems)
}

func TestQCOW2MapImageUbuntu1604(t *testing.T) {
	testQCOW2MapImage(
		t,
		getQCOW2ImageFileURLUbuntu1604(),
		getQCOW2ImageSizeUbuntu1604(t),
		getQCOW2ExpectedMapFileUbuntu1604(),
	)
}

func TestQCOW2MapImageUbuntu1804(t *testing.T) {
	testQCOW2MapImage(
		t,
		getQCOW2ImageFileURLUbuntu1804(),
		getQCOW2ImageSizeUbuntu1804(t),
		getQCOW2ExpectedMapFileUbuntu1804(),
	)
}
