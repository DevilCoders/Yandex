package tests

import (
	"context"
	"fmt"
	"os"
	"strconv"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	grpc_codes "google.golang.org/grpc/codes"
	grpc_status "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	nbs_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	snapshot_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

func createSaveStateFunc() snapshot.SaveStateFunc {
	return func(offset int64, progress float64) error {
		return nil
	}
}

func createNbsClient(t *testing.T, ctx context.Context) nbs.Client {
	rootCertsFile := os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE")

	factory, err := nbs.CreateFactory(
		ctx,
		&nbs_config.ClientConfig{
			Zones: map[string]*nbs_config.Zone{
				"zone": {
					Endpoints: []string{
						"fake",
						"fake",
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("DISK_MANAGER_RECIPE_NBS_PORT"),
						),
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("DISK_MANAGER_RECIPE_NBS_PORT"),
						),
					},
				},
			},
			RootCertsFile: &rootCertsFile,
		},
		metrics.CreateEmptyRegistry(),
	)
	require.NoError(t, err)

	client, err := factory.GetClient(ctx, "zone")
	require.NoError(t, err)

	return client
}

func createSnapshotClient(t *testing.T, ctx context.Context) snapshot.Client {
	zoneID := "zone"
	secure := true
	rootCertsFile := os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE")

	factory := snapshot.CreateFactory(
		&snapshot_config.ClientConfig{
			DefaultZoneId: &zoneID,
			Zones: map[string]*snapshot_config.Zone{
				"zone": {
					Endpoints: []string{
						"fake",
						"fake",
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("DISK_MANAGER_RECIPE_SNAPSHOT_PORT"),
						),
					},
				},
			},
			Secure:        &secure,
			RootCertsFile: &rootCertsFile,
		},
		metrics.CreateEmptyRegistry(),
	)

	client, err := factory.CreateClient(ctx)
	require.NoError(t, err)

	return client
}

func getImageFileURL() string {
	port := os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_SERVER_PORT")
	return fmt.Sprintf("http://localhost:%v", port)
}

func getImageFileSize(t *testing.T) uint64 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_SIZE"), 10, 64)
	require.NoError(t, err)
	return uint64(value)
}

func getImageFileCrc32(t *testing.T) uint32 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_CRC32"), 10, 32)
	require.NoError(t, err)
	return uint32(value)
}

func getDefaultTaskState(taskID string) snapshot.TaskState {
	return snapshot.TaskState{
		TaskID:           taskID,
		Offset:           0,
		HeartbeatTimeout: 5 * time.Second,
		PingPeriod:       time.Second,
		BackoffTimeout:   5 * time.Second,
	}
}

func getDefaultDiskParams(diskID string) nbs.CreateDiskParams {
	return nbs.CreateDiskParams{
		ID:          diskID,
		BlockSize:   4096,
		BlocksCount: 102400,
		Kind:        types.DiskKind_DISK_KIND_SSD,
		CloudID:     "cloud",
		FolderID:    "folder",
	}
}

////////////////////////////////////////////////////////////////////////////////

func TestCreateImageFromURL(t *testing.T) {
	ctx := createContext()

	client := createSnapshotClient(t, ctx)
	defer client.Close()

	taskID := fmt.Sprintf("%v_taskID", t.Name())
	state := snapshot.CreateImageFromURLState{
		SrcURL:     getImageFileURL(),
		Format:     "raw",
		DstImageID: t.Name(),
		FolderID:   "folder",
		State:      getDefaultTaskState(taskID),
	}

	info, err := client.CreateImageFromURL(ctx, state, createSaveStateFunc())
	require.NoError(t, err)
	require.NotEmpty(t, info)

	// Check idempotency.
	info, err = client.CreateImageFromURL(ctx, state, createSaveStateFunc())
	require.NoError(t, err)
	require.NotEmpty(t, info)

	// Task should be cleared.
	err = client.DeleteTask(ctx, taskID)
	status, ok := grpc_status.FromError(err)
	require.True(t, ok)
	require.Equal(t, grpc_codes.NotFound, status.Code())
}

func TestCreateImageFromImage(t *testing.T) {
	ctx := createContext()

	client := createSnapshotClient(t, ctx)
	defer client.Close()

	firstImageID := fmt.Sprintf("%v_createFirstImage", t.Name())
	createFirstImageTaskID := firstImageID
	info, err := client.CreateImageFromURL(
		ctx,
		snapshot.CreateImageFromURLState{
			SrcURL:     getImageFileURL(),
			Format:     "raw",
			DstImageID: firstImageID,
			FolderID:   "folder",
			State:      getDefaultTaskState(createFirstImageTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	secondImageID := fmt.Sprintf("%v_createSecondImage", t.Name())
	// Second iteration is an idempotency check.
	for i := 0; i < 2; i++ {
		createSecondImageTaskID := secondImageID
		info, err = client.CreateImageFromImage(
			ctx,
			snapshot.CreateImageFromImageState{
				SrcImageID: firstImageID,
				DstImageID: secondImageID,
				FolderID:   "folder",
				State:      getDefaultTaskState(createSecondImageTaskID),
			},
			createSaveStateFunc(),
		)
		require.NoError(t, err)
		require.NotEmpty(t, info)
	}
}

func TestCreateImageFromEmptyDisk(t *testing.T) {
	ctx := createContext()
	nbsClient := createNbsClient(t, ctx)

	diskID := t.Name()
	err := nbsClient.Create(ctx, getDefaultDiskParams(diskID))
	require.NoError(t, err)

	snapshotID := t.Name()
	diskCheckpointID := snapshotID
	err = nbsClient.CreateCheckpoint(ctx, diskID, diskCheckpointID)
	require.NoError(t, err)

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	taskID := fmt.Sprintf("%v_taskID", t.Name())
	info, err := snapshotClient.CreateImageFromDisk(
		ctx,
		snapshot.CreateImageFromDiskState{
			SrcDisk:             &types.Disk{DiskId: diskID, ZoneId: "zone"},
			SrcDiskCheckpointID: diskCheckpointID,
			DstImageID:          snapshotID,
			FolderID:            "folder",
			State:               getDefaultTaskState(taskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)
}

func TestCreateSnapshotFromEmptyDisk(t *testing.T) {
	ctx := createContext()
	nbsClient := createNbsClient(t, ctx)

	diskID := t.Name()
	err := nbsClient.Create(ctx, getDefaultDiskParams(diskID))
	require.NoError(t, err)

	snapshotID := t.Name()
	diskCheckpointID := snapshotID
	err = nbsClient.CreateCheckpoint(ctx, diskID, diskCheckpointID)
	require.NoError(t, err)

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	taskID := fmt.Sprintf("%v_task", t.Name())
	info, err := snapshotClient.CreateSnapshotFromDisk(
		ctx,
		snapshot.CreateSnapshotFromDiskState{
			SrcDisk:             &types.Disk{DiskId: diskID, ZoneId: "zone"},
			SrcDiskCheckpointID: diskCheckpointID,
			DstSnapshotID:       snapshotID,
			FolderID:            "folder",
			State:               getDefaultTaskState(taskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)
}

func TestCreateImageFromSnapshotOfEmptyDisk(t *testing.T) {
	ctx := createContext()
	nbsClient := createNbsClient(t, ctx)

	err := nbsClient.Create(ctx, getDefaultDiskParams(t.Name()))
	require.NoError(t, err)

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	snapshotID := fmt.Sprintf("%v_snapshot", t.Name())
	createSnapshotTaskID := fmt.Sprintf("%v_createSnapshot", t.Name())
	info, err := snapshotClient.CreateSnapshotFromDisk(
		ctx,
		snapshot.CreateSnapshotFromDiskState{
			SrcDisk:       &types.Disk{DiskId: t.Name(), ZoneId: "zone"},
			DstSnapshotID: snapshotID,
			FolderID:      "folder",
			State:         getDefaultTaskState(createSnapshotTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	imageID := fmt.Sprintf("%v_image", t.Name())
	createImageTaskID := fmt.Sprintf("%v_createImage", t.Name())
	info, err = snapshotClient.CreateImageFromSnapshot(
		ctx,
		snapshot.CreateImageFromSnapshotState{
			SrcSnapshotID: snapshotID,
			DstImageID:    imageID,
			FolderID:      "folder",
			State:         getDefaultTaskState(createImageTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)
}

func TestTransferFromImageToDisk(t *testing.T) {
	ctx := createContext()

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	createImageTaskID := fmt.Sprintf("%v_createImage", t.Name())
	info, err := snapshotClient.CreateImageFromURL(
		ctx,
		snapshot.CreateImageFromURLState{
			SrcURL:     getImageFileURL(),
			Format:     "raw",
			DstImageID: t.Name(),
			FolderID:   "folder",
			State:      getDefaultTaskState(createImageTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	nbsClient := createNbsClient(t, ctx)
	diskID := t.Name()

	err = nbsClient.Create(ctx, getDefaultDiskParams(diskID))
	require.NoError(t, err)

	transferTaskID := fmt.Sprintf("%v_transfer", t.Name())
	err = snapshotClient.TransferFromImageToDisk(
		ctx,
		snapshot.TransferFromImageToDiskState{
			SrcImageID: t.Name(),
			DstDisk:    &types.Disk{ZoneId: "zone", DiskId: diskID},
			State:      getDefaultTaskState(transferTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID, getImageFileSize(t), getImageFileCrc32(t))
	require.NoError(t, err)
}

func TestTransferFromImageToDiskRetriesMountVolumeErrors(t *testing.T) {
	ctx := createContext()

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	createImageTaskID := fmt.Sprintf("%v_createImage", t.Name())
	info, err := snapshotClient.CreateImageFromURL(
		ctx,
		snapshot.CreateImageFromURLState{
			SrcURL:     getImageFileURL(),
			Format:     "raw",
			DstImageID: t.Name(),
			FolderID:   "folder",
			State:      getDefaultTaskState(createImageTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	nbsClient := createNbsClient(t, ctx)

	diskID := t.Name()
	err = nbsClient.Create(ctx, getDefaultDiskParams(diskID))
	require.NoError(t, err)

	unmount, err := nbsClient.MountForReadWrite(diskID)
	require.NoError(t, err)
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		<-time.After(2 * time.Second)
		unmount()
		wg.Done()
	}()

	transferTaskID := fmt.Sprintf("%v_transfer", t.Name())
	err = snapshotClient.TransferFromImageToDisk(
		ctx,
		snapshot.TransferFromImageToDiskState{
			SrcImageID: t.Name(),
			DstDisk:    &types.Disk{ZoneId: "zone", DiskId: diskID},
			State:      getDefaultTaskState(transferTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)

	wg.Wait()
	err = nbsClient.ValidateCrc32(diskID, getImageFileSize(t), getImageFileCrc32(t))
	require.NoError(t, err)
}

func TestTransferFromSnapshotToDisk(t *testing.T) {
	ctx := createContext()
	nbsClient := createNbsClient(t, ctx)

	srcDiskID := fmt.Sprintf("%v_srcDisk", t.Name())
	err := nbsClient.Create(ctx, getDefaultDiskParams(srcDiskID))
	require.NoError(t, err)

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	snapshotID := fmt.Sprintf("%v_snapshot", t.Name())
	createSnapshotTaskID := fmt.Sprintf("%v_createSnapshot", t.Name())
	info, err := snapshotClient.CreateSnapshotFromDisk(
		ctx,
		snapshot.CreateSnapshotFromDiskState{
			SrcDisk:       &types.Disk{DiskId: srcDiskID, ZoneId: "zone"},
			DstSnapshotID: snapshotID,
			FolderID:      "folder",
			State:         getDefaultTaskState(createSnapshotTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	dstDiskID := fmt.Sprintf("%v_dstDisk", t.Name())
	err = nbsClient.Create(ctx, getDefaultDiskParams(dstDiskID))
	require.NoError(t, err)

	transferTaskID := fmt.Sprintf("%v_transfer", t.Name())
	err = snapshotClient.TransferFromSnapshotToDisk(
		ctx,
		snapshot.TransferFromSnapshotToDiskState{
			SrcSnapshotID: snapshotID,
			DstDisk:       &types.Disk{ZoneId: "zone", DiskId: dstDiskID},
			State:         getDefaultTaskState(transferTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
}

func TestDeleteImage(t *testing.T) {
	ctx := createContext()

	client := createSnapshotClient(t, ctx)
	defer client.Close()

	deleteImageTaskID := fmt.Sprintf("%v_deleteImage", t.Name())
	state := snapshot.DeleteImageState{
		ImageID: t.Name(),
		State:   getDefaultTaskState(deleteImageTaskID),
	}

	// Image is not created yet, so deletion should be good.
	err := client.DeleteImage(ctx, state, createSaveStateFunc())
	require.NoError(t, err)

	createImageTaskID := fmt.Sprintf("%v_createImage", t.Name())
	info, err := client.CreateImageFromURL(
		ctx,
		snapshot.CreateImageFromURLState{
			SrcURL:     getImageFileURL(),
			Format:     "raw",
			DstImageID: t.Name(),
			FolderID:   "folder",
			State:      getDefaultTaskState(createImageTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	err = client.DeleteImage(ctx, state, createSaveStateFunc())
	require.NoError(t, err)

	// Check idempotency.
	err = client.DeleteImage(ctx, state, createSaveStateFunc())
	require.NoError(t, err)

	// Tasks should be cleared.

	err = client.DeleteTask(ctx, createImageTaskID)
	status, ok := grpc_status.FromError(err)
	require.True(t, ok)
	require.Equal(t, grpc_codes.NotFound, status.Code())

	err = client.DeleteTask(ctx, deleteImageTaskID)
	status, ok = grpc_status.FromError(err)
	require.True(t, ok)
	require.Equal(t, grpc_codes.NotFound, status.Code())
}

func TestDeleteSnapshot(t *testing.T) {
	ctx := createContext()

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	snapshotID := fmt.Sprintf("%v_snapshot", t.Name())
	deleteSnapshotTaskID := fmt.Sprintf("%v_deleteSnapshot", t.Name())
	state := snapshot.DeleteSnapshotState{
		SnapshotID: snapshotID,
		State:      getDefaultTaskState(deleteSnapshotTaskID),
	}

	// Snapshot is not created yet, so deletion should be good.
	err := snapshotClient.DeleteSnapshot(ctx, state, createSaveStateFunc())
	require.NoError(t, err)

	nbsClient := createNbsClient(t, ctx)
	srcDiskID := fmt.Sprintf("%v_srcDisk", t.Name())
	err = nbsClient.Create(ctx, getDefaultDiskParams(srcDiskID))
	require.NoError(t, err)

	createSnapshotTaskID := fmt.Sprintf("%v_createSnapshot", t.Name())
	info, err := snapshotClient.CreateSnapshotFromDisk(
		ctx,
		snapshot.CreateSnapshotFromDiskState{
			SrcDisk:       &types.Disk{DiskId: srcDiskID, ZoneId: "zone"},
			DstSnapshotID: snapshotID,
			FolderID:      "folder",
			State:         getDefaultTaskState(createSnapshotTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	err = snapshotClient.DeleteSnapshot(ctx, state, createSaveStateFunc())
	require.NoError(t, err)

	// Check idempotency.
	err = snapshotClient.DeleteSnapshot(ctx, state, createSaveStateFunc())
	require.NoError(t, err)

	// Task should be cleared.
	err = snapshotClient.DeleteTask(ctx, createSnapshotTaskID)
	status, ok := grpc_status.FromError(err)
	require.True(t, ok)
	require.Equal(t, grpc_codes.NotFound, status.Code())
}

func TestShouldNotResetProgressOnTaskStatusErrors(t *testing.T) {
	ctx := createContext()

	snapshotClient := createSnapshotClient(t, ctx)
	defer snapshotClient.Close()

	createImageTaskID := fmt.Sprintf("%v_createImage", t.Name())
	info, err := snapshotClient.CreateImageFromURL(
		ctx,
		snapshot.CreateImageFromURLState{
			SrcURL:     getImageFileURL(),
			Format:     "raw",
			DstImageID: t.Name(),
			FolderID:   "folder",
			State:      getDefaultTaskState(createImageTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.NotEmpty(t, info)

	nbsClient := createNbsClient(t, ctx)
	diskID := t.Name()

	err = nbsClient.Create(ctx, getDefaultDiskParams(diskID))
	require.NoError(t, err)

	transferTaskID := fmt.Sprintf("%v_transfer", t.Name())

	checkpointIdx := 0
	errorsLeft := 5
	currentOffset := int64(0)
	checkpointFunc := func(offset int64, progress float64) error {
		require.LessOrEqual(t, currentOffset, offset)
		currentOffset = offset

		if errorsLeft != 0 && checkpointIdx%2 != 0 {
			// Induce GetTaskStatus error for any odd checkpoint.
			_ = snapshotClient.DeleteTask(ctx, transferTaskID)
			errorsLeft--
		}

		checkpointIdx++
		return nil
	}

	err = snapshotClient.TransferFromImageToDisk(
		ctx,
		snapshot.TransferFromImageToDiskState{
			SrcImageID: t.Name(),
			DstDisk:    &types.Disk{ZoneId: "zone", DiskId: diskID},
			State:      getDefaultTaskState(transferTaskID),
		},
		checkpointFunc,
	)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID, getImageFileSize(t), getImageFileCrc32(t))
	require.NoError(t, err)
}

func TestCheckResourceInfo(t *testing.T) {
	ctx := createContext()

	var err error
	for {
		client := createSnapshotClient(t, ctx)
		defer client.Close()

		_, err = client.CheckResourceReady(ctx, "unexisting")
		if !errors.CanRetry(err) {
			break
		}
	}
	require.True(t, errors.IsPublic(err), err.Error())

	imageID := t.Name()

	client := createSnapshotClient(t, ctx)
	defer client.Close()

	createImageTaskID := fmt.Sprintf("%v_createImage", t.Name())
	info, err := client.CreateImageFromURL(
		ctx,
		snapshot.CreateImageFromURLState{
			SrcURL:     getImageFileURL(),
			Format:     "raw",
			DstImageID: imageID,
			FolderID:   "folder",
			State:      getDefaultTaskState(createImageTaskID),
		},
		createSaveStateFunc(),
	)
	require.NoError(t, err)
	require.Equal(t, int64(getImageFileSize(t)), info.Size)
	require.Less(t, int64(0), info.StorageSize)

	var actualInfo snapshot.ResourceInfo
	for {
		client := createSnapshotClient(t, ctx)
		defer client.Close()

		actualInfo, err = client.CheckResourceReady(ctx, imageID)
		if !errors.CanRetry(err) {
			break
		}
	}
	require.NoError(t, err)
	require.Equal(t, info, actualInfo)
}
