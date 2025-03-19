package disk_service_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes/empty"
	"github.com/stretchr/testify/require"
	grpc_codes "google.golang.org/grpc/codes"
	grpc_status "google.golang.org/grpc/status"

	operation_proto "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/api"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/grpc/testcommon"
	"a.yandex-team.ru/cloud/disk_manager/pkg/client/codes"
)

////////////////////////////////////////////////////////////////////////////////

func TestDiskServiceCreateEmptyDisk(t *testing.T) {
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
		Size: 4096,
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
}

func TestDiskServiceShouldFailCreateDiskFromNonExistingImage(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: "xxx",
		},
		Size: 134217728,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.Error(t, err)
	require.Contains(t, err.Error(), "not found")

	status, ok := grpc_status.FromError(err)
	require.True(t, ok)

	statusDetails := status.Details()
	require.Equal(t, 1, len(statusDetails))

	errorDetails, ok := statusDetails[0].(*disk_manager.ErrorDetails)
	require.True(t, ok)

	require.Equal(t, int64(codes.BadSource), errorDetails.Code)
	require.False(t, errorDetails.Internal)
}

func TestDiskServiceCreateDiskFromImage(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()
	imageSize := uint64(64 * 1024 * 1024)
	imageCrc32, imageStorageSize := testcommon.CreateImage(
		t,
		ctx,
		imageID,
		imageSize,
		"dataplaneFolder",
		true, // pooled
	)

	diskID := t.Name()
	diskSize := 2 * imageSize

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
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

	stats, err := client.StatDisk(ctx, &disk_manager.StatDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.Equal(t, int64(imageStorageSize), stats.StorageSize)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	err = nbsClient.ValidateCrc32(
		diskID,
		imageSize,
		imageCrc32,
	)
	require.NoError(t, err)
}

func testDiskServiceCreateDiskFromImageWithForceNotLayered(t *testing.T, folderID string) {
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
		Pooled:     true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	diskID := t.Name()

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
		},
		Size: 134217728,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		ForceNotLayered: true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	err = nbsClient.ValidateCrc32(
		diskID,
		testcommon.GetImageFileSize(t),
		testcommon.GetImageFileCrc32(t),
	)
	require.NoError(t, err)
}

func TestDiskServiceCreateDiskFromImageWithForceNotLayered(t *testing.T) {
	testDiskServiceCreateDiskFromImageWithForceNotLayered(t, "folder")
}

func TestDiskServiceCreateDiskFromImageWithForceNotLayeredViaDataplaneTask(t *testing.T) {
	testDiskServiceCreateDiskFromImageWithForceNotLayered(t, "dataplaneFromURLFolder")
}

func TestDiskServiceCancelCreateDiskFromImage(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()
	imageSize := uint64(64 * 1024 * 1024)

	_, _ = testcommon.CreateImage(
		t,
		ctx,
		imageID,
		imageSize,
		"dataplaneFolder",
		true, // pooled
	)

	diskID := t.Name()
	diskSize := 2 * imageSize

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
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
	testcommon.CancelOperation(t, ctx, client, operation.Id)
}

func TestDiskServiceDeleteDiskWhenCreationIsInFlight(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()
	imageSize := uint64(64 * 1024 * 1024)

	_, _ = testcommon.CreateImage(
		t,
		ctx,
		imageID,
		imageSize,
		"dataplaneFolder",
		true, // pooled
	)

	// Need to add some variance to test.
	testcommon.WaitForRandomDuration(time.Millisecond, 2*time.Second)

	diskID := t.Name()
	diskSize := 2 * imageSize

	reqCtx := testcommon.GetRequestContext(t, ctx)
	createOp, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, createOp)

	<-time.After(time.Second)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err := client.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	_ = internal_client.WaitOperation(ctx, client, createOp.Id)
}

func TestDiskServiceCreateDisksFromImageWithConfiguredPool(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()
	imageSize := uint64(64 * 1024 * 1024)

	imageCrc32, _ := testcommon.CreateImage(
		t,
		ctx,
		imageID,
		imageSize,
		"dataplaneFolder",
		false, // pooled
	)

	privateClient, err := testcommon.CreatePrivateClient(ctx)
	require.NoError(t, err)
	defer privateClient.Close()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := privateClient.ConfigurePool(reqCtx, &api.ConfigurePoolRequest{
		ImageId:      imageID,
		ZoneId:       "zone",
		Capacity:     12,
		UseImageSize: true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	diskSize := 2 * imageSize

	var operations []*operation_proto.Operation
	for i := 0; i < 20; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx = testcommon.GetRequestContext(t, ctx)
		operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
			Src: &disk_manager.CreateDiskRequest_SrcImageId{
				SrcImageId: imageID,
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
		operations = append(operations, operation)
	}

	nbsClient := testcommon.CreateNbsClient(t, ctx)

	for i, operation := range operations {
		err := internal_client.WaitOperation(ctx, client, operation.Id)
		require.NoError(t, err)

		diskID := fmt.Sprintf("%v%v", t.Name(), i)
		err = nbsClient.ValidateCrc32(diskID, imageSize, imageCrc32)
		require.NoError(t, err)
	}

	operations = nil
	for i := 0; i < 20; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx = testcommon.GetRequestContext(t, ctx)
		operation, err = client.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, operation)
		operations = append(operations, operation)
	}

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	operations = append(operations, operation)

	for _, operation := range operations {
		err := internal_client.WaitOperation(ctx, client, operation.Id)
		require.NoError(t, err)
	}
}

func TestDiskServiceCreateDiskFromSnapshot(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name() + "_image"
	imageSize := uint64(64 * 1024 * 1024)

	imageCrc32, _ := testcommon.CreateImage(
		t,
		ctx,
		imageID,
		imageSize,
		"dataplaneFolder",
		false, // pooled
	)

	srcDiskID := t.Name() + "_src"
	diskSize := 2 * imageSize

	reqCtx := testcommon.GetRequestContext(t, ctx)
	// TODO: Don't use ForceNotLayered flag.
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: srcDiskID,
		},
		ForceNotLayered: true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)

	err = nbsClient.ValidateCrc32(srcDiskID, imageSize, imageCrc32)
	require.NoError(t, err)

	snapshotID := t.Name() + "_snapshot"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: srcDiskID,
		},
		SnapshotId: snapshotID,
		FolderId:   "folder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	dstDiskID := t.Name() + "_dst"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID,
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: dstDiskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	diskMeta := disk_manager.CreateDiskMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &diskMeta)
	require.NoError(t, err)
	require.Equal(t, float64(1), diskMeta.Progress)

	err = nbsClient.ValidateCrc32(dstDiskID, imageSize, imageCrc32)
	require.NoError(t, err)
}

func TestDiskServiceCreateDiskFromIncrementalSnapshot(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskSize := uint64(128 * 1024 * 1024)
	diskID1 := t.Name() + "1"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID1,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	crc32, _, err := testcommon.FillNbsDisk(nbsClient, diskID1, diskSize)
	require.NoError(t, err)

	snapshotID1 := t.Name() + "1"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID1,
		},
		SnapshotId:  snapshotID1,
		FolderId:    "dataplaneFolder",
		Incremental: true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	diskID2 := t.Name() + "2"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID1,
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

	err = nbsClient.ValidateCrc32(diskID1, diskSize, crc32)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID2, diskSize, crc32)
	require.NoError(t, err)
}

func TestDiskServiceCreateDiskFromSnapshotViaDataplaneTask(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskSize := uint64(32 * 1024 * 4096)
	diskID1 := t.Name() + "1"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID1,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	crc32, _, err := testcommon.FillNbsDisk(nbsClient, diskID1, diskSize)
	require.NoError(t, err)

	snapshotID1 := t.Name() + "1"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID1,
		},
		SnapshotId: snapshotID1,
		FolderId:   "dataplaneFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	snapshotMeta := disk_manager.CreateSnapshotMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &snapshotMeta)
	require.NoError(t, err)
	require.Equal(t, float64(1), snapshotMeta.Progress)

	diskID2 := t.Name() + "2"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID1,
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

	diskMeta := disk_manager.CreateDiskMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &diskMeta)
	require.NoError(t, err)
	require.Equal(t, float64(1), diskMeta.Progress)

	err = nbsClient.ValidateCrc32(diskID1, diskSize, crc32)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID2, diskSize, crc32)
	require.NoError(t, err)
}

func TestDiskServiceCreateDiskFromImageViaDataplaneTask(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskSize := uint64(32 * 1024 * 4096)
	diskID1 := t.Name() + "1"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID1,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	crc32, _, err := testcommon.FillNbsDisk(nbsClient, diskID1, diskSize)
	require.NoError(t, err)

	imageID := t.Name() + "_image"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID1,
			},
		},
		DstImageId: imageID,
		FolderId:   "dataplaneFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	meta := disk_manager.CreateImageMetadata{}
	err = internal_client.GetOperationMetadata(ctx, client, operation.Id, &meta)
	require.NoError(t, err)
	require.Equal(t, float64(1), meta.Progress)

	diskID2 := t.Name() + "2"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
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

	err = nbsClient.ValidateCrc32(diskID1, diskSize, crc32)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID2, diskSize, crc32)
	require.NoError(t, err)
}

func TestDiskServiceCreateDiskFromSnapshotOfOverlayDiskViaDataplaneTask(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageSize := uint64(64 * 1024 * 1024)

	diskID1 := t.Name() + "1"

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(imageSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID1,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)

	crc32, _, err := testcommon.FillNbsDisk(nbsClient, diskID1, imageSize)
	require.NoError(t, err)

	imageID := t.Name() + "_image"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID1,
			},
		},
		DstImageId: imageID,
		FolderId:   "dataplaneFolder",
		Pooled:     false,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	privateClient, err := testcommon.CreatePrivateClient(ctx)
	require.NoError(t, err)
	defer privateClient.Close()

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = privateClient.ConfigurePool(reqCtx, &api.ConfigurePoolRequest{
		ImageId:      imageID,
		ZoneId:       "zone",
		Capacity:     1,
		UseImageSize: true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	largeDiskSize := uint64(128 * 1024 * 1024 * 1024)
	diskID2 := t.Name() + "2"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
		},
		Size: int64(largeDiskSize),
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

	snapshotID := t.Name() + "_snapshot"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID2,
		},
		SnapshotId: snapshotID,
		FolderId:   "dataplaneFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	diskID3 := t.Name() + "3"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID,
		},
		Size: int64(largeDiskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID3,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID1, imageSize, crc32)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID2, imageSize, crc32)
	require.NoError(t, err)

	err = nbsClient.ValidateCrc32(diskID3, imageSize, crc32)
	require.NoError(t, err)
}

func TestDiskServiceCreateZonalDataplaneTaskInAnotherZone(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskSize := uint64(32 * 1024)
	diskID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(diskSize),
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "other",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	snapshotID := t.Name()

	// zoneId of DM test server is "zone", dataPlane requests for another zone shouldn't be executed
	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateSnapshot(reqCtx, &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: "other",
			DiskId: diskID,
		},
		SnapshotId: snapshotID,
		FolderId:   "dataplaneFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	opCtx, cancel := context.WithTimeout(ctx, 30*time.Second)
	defer cancel()
	err = internal_client.WaitOperation(opCtx, client, operation.Id)
	require.Error(t, err)
	require.Contains(t, err.Error(), "context deadline exceeded")

	imageID := t.Name() + "_image"

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: "other",
				DiskId: diskID,
			},
		},
		DstImageId: imageID,
		FolderId:   "dataplaneFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	opCtx, cancel = context.WithTimeout(ctx, 30*time.Second)
	defer cancel()
	err = internal_client.WaitOperation(opCtx, client, operation.Id)
	require.Error(t, err)
	require.Contains(t, err.Error(), "context deadline exceeded")
}

func TestDiskServiceResizeDisk(t *testing.T) {
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
		Size: 4096,
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

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.ResizeDisk(reqCtx, &disk_manager.ResizeDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		Size: 40960,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}

func TestDiskServiceAlterDisk(t *testing.T) {
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
		Size: 4096,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		CloudId:  "cloud",
		FolderId: "folder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.AlterDisk(reqCtx, &disk_manager.AlterDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		CloudId:  "newCloud",
		FolderId: "newFolder",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}

func TestDiskServiceAssignDisk(t *testing.T) {
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
		Size: 4096,
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

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.AssignDisk(reqCtx, &disk_manager.AssignDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
		InstanceId: "InstanceId",
		Host:       "Host",
		Token:      "Token",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}

func TestDiskServiceDescribeDiskModel(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	model, err := client.DescribeDiskModel(ctx, &disk_manager.DescribeDiskModelRequest{
		ZoneId:        "zone",
		BlockSize:     4096,
		Size:          1000000 * 4096,
		Kind:          disk_manager.DiskKind_DISK_KIND_SSD,
		TabletVersion: 1,
	})
	require.NoError(t, err)
	require.Equal(t, int64(4096), model.BlockSize)
	require.Equal(t, int64(1000000*4096), model.Size)
	require.Equal(t, disk_manager.DiskKind_DISK_KIND_SSD, model.Kind)

	model, err = client.DescribeDiskModel(ctx, &disk_manager.DescribeDiskModelRequest{
		BlockSize:     4096,
		Size:          1000000 * 4096,
		Kind:          disk_manager.DiskKind_DISK_KIND_SSD,
		TabletVersion: 1,
	})
	require.NoError(t, err)
	require.Equal(t, int64(4096), model.BlockSize)
	require.Equal(t, int64(1000000*4096), model.Size)
	require.Equal(t, disk_manager.DiskKind_DISK_KIND_SSD, model.Kind)
}

func TestDiskServiceUnauthorizedGetOperation(t *testing.T) {
	ctx := testcommon.CreateContextWithToken("TestTokenDisksOnly")

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	// Creating disk should have completed successfully.
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: 4096,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	_, err = client.GetOperation(reqCtx, &disk_manager.GetOperationRequest{
		OperationId: operation.Id,
	})
	require.Error(t, err)
	require.Equalf(
		t,
		grpc_codes.PermissionDenied,
		grpc_status.Code(err),
		"actual error %v",
		err,
	)
}

func TestDiskServiceUnauthenticatedCreateDisk(t *testing.T) {
	ctx := testcommon.CreateContextWithToken("NoTestToken")

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := t.Name()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	_, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: 4096,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.Error(t, err)
	require.Equalf(
		t,
		grpc_codes.Unauthenticated,
		grpc_status.Code(err),
		"actual error %v",
		err,
	)
}

func TestDiskServiceMigrateDisk(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	srcDiskID := disk_manager.DiskId{
		DiskId: t.Name(),
		ZoneId: "zone",
	}

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size:   4096,
		Kind:   disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &srcDiskID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.MigrateDisk(reqCtx, &disk_manager.MigrateDiskRequest{
		DiskId:    &srcDiskID,
		DstZoneId: "zone2",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.Error(t, err)
	require.ErrorContains(t, err, "not implemented")

	reqCtx = testcommon.GetRequestContext(t, ctx)
	err = client.SendMigrationSignal(reqCtx, &disk_manager.SendMigrationSignalRequest{
		OperationId: operation.Id,
		Signal:      disk_manager.SendMigrationSignalRequest_FINISH_REPLICATION,
	})
	require.Error(t, err)
	require.ErrorContains(t, err, "not implemented")

	reqCtx = testcommon.GetRequestContext(t, ctx)
	err = client.SendMigrationSignal(reqCtx, &disk_manager.SendMigrationSignalRequest{
		OperationId: operation.Id,
		Signal:      disk_manager.SendMigrationSignalRequest_FINISH_MIGRATION,
	})
	require.Error(t, err)
	require.ErrorContains(t, err, "not implemented")
}
