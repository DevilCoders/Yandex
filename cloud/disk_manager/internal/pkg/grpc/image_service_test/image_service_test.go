package tests

import (
	"testing"

	"github.com/golang/protobuf/ptypes/empty"
	"github.com/stretchr/testify/require"
	grpc_status "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/grpc/testcommon"
	"a.yandex-team.ru/cloud/disk_manager/pkg/client/codes"
)

////////////////////////////////////////////////////////////////////////////////

func createImageFromImage(
	t *testing.T,
	srcFolderID string,
	dstFolderID string,
) {

	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID1 := t.Name() + "_image1"
	imageSize := uint64(40 * 1024 * 1024)

	imageCrc32, _ := testcommon.CreateImage(
		t,
		ctx,
		imageID1,
		imageSize,
		srcFolderID,
		false, // pooled
	)

	imageID2 := t.Name() + "_image2"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcImageId{
			SrcImageId: imageID1,
		},
		DstImageId: imageID2,
		FolderId:   dstFolderID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	response := disk_manager.CreateImageResponse{}
	err = internal_client.WaitResponse(ctx, client, operation.Id, &response)
	require.NoError(t, err)
	require.Equal(t, int64(imageSize), response.Size)

	meta := disk_manager.CreateImageMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &meta)
	require.NoError(t, err)
	require.Equal(t, float64(1), meta.Progress)

	diskID2 := t.Name() + "_disk2"
	diskSize := imageSize

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID2,
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID2,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)

	err = nbsClient.ValidateCrc32(diskID2, imageSize, imageCrc32)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation1, err := client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID1,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation1)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation2, err := client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID2,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation2)

	err = internal_client.WaitOperation(ctx, client, operation1.Id)
	require.NoError(t, err)
	err = internal_client.WaitOperation(ctx, client, operation2.Id)
	require.NoError(t, err)
}

func createImageFromSnapshot(
	t *testing.T,
	srcFolderID string,
	dstFolderID string,
) {

	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskSize := uint64(10 * 1024 * 4096)
	diskID := t.Name() + "_disk"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	_, _, err = testcommon.FillNbsDisk(nbsClient, diskID, diskSize)
	require.NoError(t, err)

	snapshotID := t.Name() + "_snapshot"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		SnapshotId: snapshotID,
		FolderId:   srcFolderID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	response := disk_manager.CreateSnapshotResponse{}
	err = internal_client.WaitResponse(ctx, client, operation.Id, &response)
	require.NoError(t, err)
	require.Equal(t, int64(diskSize), response.Size)

	imageID := t.Name() + "_image"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID,
		},
		DstImageId: imageID,
		FolderId:   dstFolderID,
		Pooled:     true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	imageResponse := disk_manager.CreateImageResponse{}
	err = internal_client.WaitResponse(ctx, client, operation.Id, &imageResponse)
	require.NoError(t, err)
	require.Equal(t, int64(diskSize), imageResponse.Size)

	meta := disk_manager.CreateImageMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &meta)
	require.NoError(t, err)
	require.Equal(t, float64(1), meta.Progress)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation1, err := client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation1)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation2, err := client.DeleteSnapshot(reqCtx, &disk_manager.DeleteSnapshotRequest{
		SnapshotId: snapshotID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation2)

	err = internal_client.WaitOperation(ctx, client, operation1.Id)
	require.NoError(t, err)
	err = internal_client.WaitOperation(ctx, client, operation2.Id)
	require.NoError(t, err)
}

func createImageFromURL(
	t *testing.T,
	url string,
	imageSize uint64,
	diskCRC32 uint32,
	folderID string,
) {

	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcUrl{
			SrcUrl: &disk_manager.ImageUrl{
				Url: url,
			},
		},
		DstImageId: imageID,
		FolderId:   folderID,
		Pooled:     true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	imageResponse := disk_manager.CreateImageResponse{}
	err = internal_client.WaitResponse(ctx, client, operation.Id, &imageResponse)
	require.NoError(t, err)
	require.Equal(t, int64(imageSize), imageResponse.Size)

	meta := disk_manager.CreateImageMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &meta)
	require.NoError(t, err)
	require.Equal(t, float64(1), meta.Progress)

	diskID := t.Name()
	diskSize := int64(imageSize)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
		},
		Size: diskSize,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	err = nbsClient.ValidateCrc32(
		diskID,
		imageSize,
		diskCRC32,
	)
	require.NoError(t, err)
}

func createRawImageFromURL(t *testing.T, dstFolderID string) {
	createImageFromURL(
		t,
		testcommon.GetImageFileURL(),
		testcommon.GetImageFileSize(t),
		testcommon.GetImageFileCrc32(t),
		dstFolderID,
	)
}

func createQCOW2ImageFromURL(t *testing.T, folderID string) {
	createImageFromURL(
		t,
		testcommon.GetQCOW2ImageFileURL(),
		testcommon.GetQCOW2ImageSize(t),
		testcommon.GetQCOW2ImageFileCrc32(t),
		folderID,
	)
}

func cancelCreateImageFromURL(t *testing.T, folderID string) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcUrl{
			SrcUrl: &disk_manager.ImageUrl{
				Url: testcommon.GetImageFileURL(),
			},
		},
		DstImageId: imageID,
		FolderId:   folderID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	testcommon.CancelOperation(t, ctx, client, operation.Id)
}

func createImageFromInvalidFile(t *testing.T, folderID string) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcUrl{
			SrcUrl: &disk_manager.ImageUrl{
				Url: testcommon.GetInvalidImageFileURL(),
			},
		},
		DstImageId: imageID,
		FolderId:   folderID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.Error(t, err)

	status, ok := grpc_status.FromError(err)
	require.True(t, ok)

	statusDetails := status.Details()
	require.Equal(t, 1, len(statusDetails))

	errorDetails, ok := statusDetails[0].(*disk_manager.ErrorDetails)
	require.True(t, ok)

	require.Equal(t, int64(codes.BadSource), errorDetails.Code)
	require.False(t, errorDetails.Internal)
	require.Equal(t, "url source not found", errorDetails.Message)
}

////////////////////////////////////////////////////////////////////////////////

func TestImageServiceCreateImageFromURL(t *testing.T) {
	createRawImageFromURL(t, "folder")
}

func TestImageServiceCreateImageFromURLViaDataplaneTask(t *testing.T) {
	createRawImageFromURL(t, "dataplaneFromURLFolder")
}

func TestImageServiceCreateQCOW2ImageFromURLViaDataplaneTask(t *testing.T) {
	createQCOW2ImageFromURL(t, "dataplaneFromURLFolder")
}

func TestImageServiceShouldFailCreateImageFromInvalidFile(t *testing.T) {
	createImageFromInvalidFile(t, "folder")
}

func TestImageServiceShouldFailCreateImageFromInvalidFileViaDataplaneTask(t *testing.T) {
	createImageFromInvalidFile(t, "dataplaneFromURLFolder")
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcUrl{
			SrcUrl: &disk_manager.ImageUrl{
				Url: testcommon.GetInvalidImageFileURL(),
			},
		},
		DstImageId: imageID,
		FolderId:   "dataplaneFromURLFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.Error(t, err)
	require.Contains(t, err.Error(), "url source not found")
}

func TestImageServiceCancelCreateImageFromURL(t *testing.T) {
	cancelCreateImageFromURL(t, "folder")
}

func TestImageServiceCancelCreateImageFromURLViaDataplaneTask(t *testing.T) {
	cancelCreateImageFromURL(t, "dataplaneFromURLFolder")
}

func TestImageServiceCreateImageFromImage(t *testing.T) {
	createImageFromImage(t, "folder", "folder")
}

func TestImageServiceCreateImageFromImageViaDataplaneTask(t *testing.T) {
	createImageFromImage(t, "dataplaneFolder", "dataplaneFolder")
}

func TestImageServiceCreateDataplaneImageFromOldImage(t *testing.T) {
	createImageFromImage(t, "folder", "dataplaneFolder")
}

func TestImageServiceCreateOldImageFromDataplaneImage(t *testing.T) {
	createImageFromImage(t, "dataplaneFolder", "folder")
}

func TestImageServiceCancelCreateImageFromImage(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID1 := t.Name() + "1"
	imageSize := uint64(64 * 1024 * 1024)

	_, _ = testcommon.CreateImage(
		t,
		ctx,
		imageID1,
		imageSize,
		"dataplaneFolder",
		false, // pooled
	)

	imageID2 := t.Name() + "2"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcImageId{
			SrcImageId: imageID1,
		},
		DstImageId: imageID2,
		FolderId:   "dataplaneFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	testcommon.CancelOperation(t, ctx, client, operation.Id)
}

func TestImageServiceCreateImageFromSnapshot(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name() + "_disk"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: 4194304,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	snapshotID := t.Name() + "_snapshot"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		SnapshotId: snapshotID,
		FolderId:   "folder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	imageID := t.Name() + "_image"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID,
		},
		DstImageId: imageID,
		FolderId:   "folder",
		Pooled:     true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}

func TestImageServiceCreateImageFromSnapshotViaDataplaneTask(t *testing.T) {
	createImageFromSnapshot(t, "dataplaneFolder", "dataplaneFolder")
}

func TestImageServiceCreateDataplaneImageFromOldSnapshot(t *testing.T) {
	createImageFromSnapshot(t, "folder", "dataplaneFolder")
}

func TestImageServiceCreateOldImageFromDataplaneSnapshot(t *testing.T) {
	createImageFromSnapshot(t, "dataplaneFolder", "folder")
}

func TestImageServiceCancelCreateImageFromSnapshot(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: 4194304,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	snapshotID := t.Name() + "_snapshot"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		SnapshotId: snapshotID,
		FolderId:   "folder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	imageID := t.Name() + "_image"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID,
		},
		DstImageId: imageID,
		FolderId:   "folder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	testcommon.CancelOperation(t, ctx, client, operation.Id)
}

func TestImageServiceCreateImageFromDisk(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: 4194304,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	imageID := t.Name()

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		},
		DstImageId: imageID,
		FolderId:   "folder",
		Pooled:     true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	testcommon.RequireCheckpointsAreEmpty(t, ctx, diskID)
}

func TestImageServiceCreateImageFromDiskViaDataplaneTask(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name()
	diskSize := uint64(4194304)

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	imageID := t.Name()

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		},
		DstImageId: imageID,
		FolderId:   "dataplaneFolder",
		Pooled:     true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	response := disk_manager.CreateImageResponse{}
	err = internal_client.WaitResponse(ctx, client, operation.Id, &response)
	require.NoError(t, err)
	require.Equal(t, int64(diskSize), response.Size)

	meta := disk_manager.CreateImageMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &meta)
	require.NoError(t, err)
	require.Equal(t, float64(1), meta.Progress)

	testcommon.RequireCheckpointsAreEmpty(t, ctx, diskID)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}

func TestImageServiceCancelCreateImageFromDisk(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: 4194304,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	imageID := t.Name()

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		},
		DstImageId: imageID,
		FolderId:   "folder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	testcommon.CancelOperation(t, ctx, client, operation.Id)
}

func TestImageServiceDeleteImage(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()
	imageSize := uint64(40 * 1024 * 1024)

	_, _ = testcommon.CreateImage(
		t,
		ctx,
		imageID,
		imageSize,
		"dataplaneFolder",
		false, // pooled
	)

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}
