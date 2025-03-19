package tests

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	operation_proto "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/api"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/grpc/testcommon"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
)

////////////////////////////////////////////////////////////////////////////////

func TestPrivateServiceScheduleBlankOperation(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreatePrivateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := client.ScheduleBlankOperation(reqCtx)
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}

func TestPrivateServiceRetireBaseDisks(t *testing.T) {
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
		ImageId:  imageID,
		ZoneId:   "zone",
		Capacity: 11,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	var operations []*operation_proto.Operation

	diskCount := 21
	for i := 0; i < diskCount; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

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
		})
		require.NoError(t, err)
		require.NotEmpty(t, operation)
		operations = append(operations, operation)
	}

	// Need to add some variance to test.
	testcommon.WaitForRandomDuration(time.Millisecond, 2*time.Second)

	// Should wait for first disk creation in order to ensure that pool is
	// created.
	err = internal_client.WaitOperation(ctx, client, operations[0].Id)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = privateClient.RetireBaseDisks(reqCtx, &api.RetireBaseDisksRequest{
		ImageId: imageID,
		ZoneId:  "zone",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	operations = append(operations, operation)

	errs := make(chan error)

	for i := 0; i < diskCount; i++ {
		operationID := operations[i].Id

		nbsClient := testcommon.CreateNbsClient(t, ctx)
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		go func() {
			err := internal_client.WaitOperation(ctx, client, operationID)
			if err != nil {
				errs <- err
				return
			}

			err = nbsClient.ValidateCrc32(diskID, imageSize, imageCrc32)
			errs <- err
		}()
	}

	for i := 0; i < diskCount; i++ {
		err := <-errs
		require.NoError(t, err)
	}

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	operations = append(operations, operation)

	for i := 0; i < diskCount; i++ {
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

	for _, operation := range operations {
		err := internal_client.WaitOperation(ctx, client, operation.Id)
		require.NoError(t, err)
	}
}

func TestPrivateServiceRetireBaseDisksUsingBaseDiskAsSrc(t *testing.T) {
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

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeleteImage(reqCtx, &disk_manager.DeleteImageRequest{
		ImageId: imageID,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	privateClient, err := testcommon.CreatePrivateClient(ctx)
	require.NoError(t, err)
	defer privateClient.Close()

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = privateClient.RetireBaseDisks(reqCtx, &api.RetireBaseDisksRequest{
		ImageId:          imageID,
		ZoneId:           "zone",
		UseBaseDiskAsSrc: true,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	logging.Info(ctx, "RetireBaseDisks operation: %v", operation.Id)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	nbsClient := testcommon.CreateNbsClient(t, ctx)
	err = nbsClient.ValidateCrc32(diskID, imageSize, imageCrc32)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
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

func TestPrivateServiceOptimizeBaseDisks(t *testing.T) {
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
		true, // pooled
	)

	var operations []*operation_proto.Operation

	diskCount := 21
	for i := 0; i < diskCount; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx := testcommon.GetRequestContext(t, ctx)
		operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
			Src: &disk_manager.CreateDiskRequest_SrcImageId{
				SrcImageId: imageID,
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
		operations = append(operations, operation)
	}

	// Need to add some variance to test.
	testcommon.WaitForRandomDuration(time.Millisecond, 2*time.Second)

	// Should wait for first disk creation in order to ensure that pool is
	// created.
	err = internal_client.WaitOperation(ctx, client, operations[0].Id)
	require.NoError(t, err)

	privateClient, err := testcommon.CreatePrivateClient(ctx)
	require.NoError(t, err)
	defer privateClient.Close()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := privateClient.OptimizeBaseDisks(reqCtx)
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	operations = append(operations, operation)

	errs := make(chan error)

	for i := 0; i < diskCount; i++ {
		operationID := operations[i].Id

		nbsClient := testcommon.CreateNbsClient(t, ctx)
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		go func() {
			err := internal_client.WaitOperation(ctx, client, operationID)
			if err != nil {
				errs <- err
				return
			}

			err = nbsClient.ValidateCrc32(diskID, imageSize, imageCrc32)
			errs <- err
		}()
	}

	for i := 0; i < diskCount; i++ {
		err := <-errs
		require.NoError(t, err)
	}

	for _, operation := range operations {
		err := internal_client.WaitOperation(ctx, client, operation.Id)
		require.NoError(t, err)
	}
}

func TestPrivateServiceConfigurePool(t *testing.T) {
	ctx := testcommon.CreateContext()

	client, err := testcommon.CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	imageID := t.Name()

	privateClient, err := testcommon.CreatePrivateClient(ctx)
	require.NoError(t, err)
	defer privateClient.Close()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := privateClient.ConfigurePool(reqCtx, &api.ConfigurePoolRequest{
		ImageId:  imageID,
		ZoneId:   "zone",
		Capacity: 1000,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.Error(t, err)
	require.Contains(t, err.Error(), "not found")

	imageSize := uint64(4 * 1024 * 1024)

	_, _ = testcommon.CreateImage(
		t,
		ctx,
		imageID,
		imageSize,
		"dataplaneFolder",
		false, // pooled
	)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = privateClient.ConfigurePool(reqCtx, &api.ConfigurePoolRequest{
		ImageId:  imageID,
		ZoneId:   "zone",
		Capacity: 1000,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = privateClient.ConfigurePool(reqCtx, &api.ConfigurePoolRequest{
		ImageId:  imageID,
		ZoneId:   "zone",
		Capacity: 0,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}

func TestPrivateServiceDeletePool(t *testing.T) {
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
		false, // pooled
	)

	privateClient, err := testcommon.CreatePrivateClient(ctx)
	require.NoError(t, err)
	defer privateClient.Close()

	reqCtx := testcommon.GetRequestContext(t, ctx)
	operation, err := privateClient.ConfigurePool(reqCtx, &api.ConfigurePoolRequest{
		ImageId:  imageID,
		ZoneId:   "zone",
		Capacity: 1,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	diskID1 := fmt.Sprintf("%v%v", t.Name(), 1)
	diskSize := 2 * imageSize

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
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

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = privateClient.DeletePool(reqCtx, &api.DeletePoolRequest{
		ImageId: imageID,
		ZoneId:  "zone",
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	// Check that we can't create pool once more.
	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = privateClient.ConfigurePool(reqCtx, &api.ConfigurePoolRequest{
		ImageId:  imageID,
		ZoneId:   "zone",
		Capacity: 1,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.Error(t, err)

	// Still we can create disk.
	diskID2 := fmt.Sprintf("%v%v", t.Name(), 2)

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

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID1,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)

	reqCtx = testcommon.GetRequestContext(t, ctx)
	operation, err = client.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID2,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)
	err = internal_client.WaitOperation(ctx, client, operation.Id)
	require.NoError(t, err)
}
