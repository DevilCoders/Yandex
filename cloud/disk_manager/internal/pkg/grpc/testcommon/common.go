package testcommon

import (
	"bytes"
	"context"
	"fmt"
	"hash/crc32"
	"math/rand"
	"os"
	"strconv"
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes/empty"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc"
	grpc_codes "google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	grpc_status "google.golang.org/grpc/status"

	operation_proto "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	nbs_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	sdk_client "a.yandex-team.ru/cloud/disk_manager/pkg/client"
	client_config "a.yandex-team.ru/cloud/disk_manager/pkg/client/config"
)

////////////////////////////////////////////////////////////////////////////////

func makeDefaultClientConfig() *client_config.Config {
	endpoint := fmt.Sprintf(
		"localhost:%v",
		os.Getenv("DISK_MANAGER_RECIPE_DISK_MANAGER_PORT"),
	)
	maxRetryAttempts := uint32(1000)
	timeout := "1s"

	return &client_config.Config{
		Endpoint:            &endpoint,
		MaxRetryAttempts:    &maxRetryAttempts,
		PerRetryTimeout:     &timeout,
		BackoffTimeout:      &timeout,
		OperationPollPeriod: &timeout,
	}
}

////////////////////////////////////////////////////////////////////////////////

func GetImageFileURL() string {
	port := os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_SERVER_PORT")
	return fmt.Sprintf("http://localhost:%v", port)
}

func GetImageFileSize(t *testing.T) uint64 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_SIZE"), 10, 64)
	require.NoError(t, err)
	return uint64(value)
}

func GetImageFileCrc32(t *testing.T) uint32 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_CRC32"), 10, 32)
	require.NoError(t, err)
	return uint32(value)
}

func GetInvalidImageFileURL() string {
	port := os.Getenv("DISK_MANAGER_RECIPE_INVALID_IMAGE_FILE_SERVER_PORT")
	return fmt.Sprintf("http://localhost:%v", port)
}

func GetQCOW2ImageFileURL() string {
	port := os.Getenv("DISK_MANAGER_RECIPE_QCOW2_IMAGE_FILE_SERVER_PORT")
	return fmt.Sprintf("http://localhost:%v", port)
}

func GetQCOW2ImageSize(t *testing.T) uint64 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_QCOW2_IMAGE_SIZE"), 10, 64)
	require.NoError(t, err)
	return uint64(value)
}

func GetQCOW2ImageFileCrc32(t *testing.T) uint32 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_QCOW2_IMAGE_CRC32"), 10, 32)
	require.NoError(t, err)
	return uint32(value)
}

////////////////////////////////////////////////////////////////////////////////

func CancelOperation(
	t *testing.T,
	ctx context.Context,
	client sdk_client.Client,
	operationID string,
) {

	operation, err := client.CancelOperation(ctx, &disk_manager.CancelOperationRequest{
		OperationId: operationID,
	})
	require.NoError(t, err)
	require.Equal(t, operationID, operation.Id)
	require.True(t, operation.Done)

	switch result := operation.Result.(type) {
	case *operation_proto.Operation_Error:
		status := grpc_status.FromProto(result.Error)
		require.Equal(t, grpc_codes.Canceled, status.Code())
	case *operation_proto.Operation_Response:
	default:
		require.True(t, false)
	}
}

func CreateClient(ctx context.Context) (sdk_client.Client, error) {
	creds, err := credentials.NewClientTLSFromFile(
		os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE"),
		"",
	)
	if err != nil {
		return nil, err
	}

	return sdk_client.NewClient(
		ctx,
		makeDefaultClientConfig(),
		grpc.WithTransportCredentials(creds),
	)
}

func CreatePrivateClient(ctx context.Context) (internal_client.PrivateClient, error) {
	creds, err := credentials.NewClientTLSFromFile(
		os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE"),
		"",
	)
	if err != nil {
		return nil, err
	}

	return internal_client.NewPrivateClient(
		ctx,
		makeDefaultClientConfig(),
		grpc.WithTransportCredentials(creds),
	)
}

func CreateNbsClient(t *testing.T, ctx context.Context) nbs.Client {
	rootCertsFile := os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE")

	durableClientTimeout := "5m"
	discoveryClientHardTimeout := "8m"
	discoveryClientSoftTimeout := "15s"

	factory, err := nbs.CreateFactory(
		ctx,
		&nbs_config.ClientConfig{
			Zones: map[string]*nbs_config.Zone{
				"zone": {
					Endpoints: []string{
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
			RootCertsFile:              &rootCertsFile,
			DurableClientTimeout:       &durableClientTimeout,
			DiscoveryClientHardTimeout: &discoveryClientHardTimeout,
			DiscoveryClientSoftTimeout: &discoveryClientSoftTimeout,
		},
		metrics.CreateEmptyRegistry(),
	)
	require.NoError(t, err)

	client, err := factory.GetClient(ctx, "zone")
	require.NoError(t, err)

	return client
}

func CreateContextWithToken(token string) context.Context {
	return headers.SetOutgoingAccessToken(context.Background(), token)
}

func CreateContext() context.Context {
	ctxWithToken := CreateContextWithToken("TestToken")
	return logging.SetLogger(
		ctxWithToken,
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

////////////////////////////////////////////////////////////////////////////////

var lastReqNumber int

func GetRequestContext(t *testing.T, ctx context.Context) context.Context {
	lastReqNumber++

	cookie := fmt.Sprintf("%v_%v", t.Name(), lastReqNumber)
	ctx = headers.SetOutgoingIdempotencyKey(ctx, cookie)
	ctx = headers.SetOutgoingRequestID(ctx, cookie)
	return ctx
}

////////////////////////////////////////////////////////////////////////////////

func WaitForRandomDuration(min time.Duration, max time.Duration) {
	var duration time.Duration

	rand.Seed(time.Now().UnixNano())
	x := min.Microseconds()
	y := max.Microseconds()

	if y <= x {
		duration = min
	} else {
		duration = time.Duration(x+rand.Int63n(y-x)) * time.Microsecond
	}

	<-time.After(duration)
}

func RequireCheckpointsAreEmpty(
	t *testing.T,
	ctx context.Context,
	diskID string,
) {

	nbsClient := CreateNbsClient(t, ctx)
	checkpoints, err := nbsClient.GetCheckpoints(ctx, diskID)
	require.NoError(t, err)
	require.Empty(t, checkpoints)
}

func WaitForCheckpointsAreEmpty(
	t *testing.T,
	ctx context.Context,
	diskID string,
) {

	nbsClient := CreateNbsClient(t, ctx)

	for {
		checkpoints, err := nbsClient.GetCheckpoints(ctx, diskID)
		require.NoError(t, err)

		if len(checkpoints) == 0 {
			return
		}

		logging.Warn(
			ctx,
			"waitForCheckpointsAreEmpty proceeding to next iteration",
		)

		<-time.After(100 * time.Millisecond)
	}
}

////////////////////////////////////////////////////////////////////////////////

func FillNbsDisk(
	nbsClient nbs.Client,
	diskID string,
	diskSize uint64,
) (uint32, uint64, error) {

	ctx := CreateContext()

	session, err := nbsClient.MountRW(ctx, diskID)
	if err != nil {
		return 0, 0, err
	}
	defer session.Close(ctx)

	blocksInChunk := uint32(1024)
	blockSize := session.BlockSize()
	chunkSize := uint64(blocksInChunk * blockSize)
	acc := crc32.NewIEEE()
	storageSize := uint64(0)
	zeroes := make([]byte, chunkSize)

	rand.Seed(time.Now().UnixNano())

	for offset := uint64(0); offset < diskSize; offset += chunkSize {
		blockIndex := offset / uint64(blockSize)
		data := make([]byte, chunkSize)
		dice := rand.Intn(3)

		var err error
		switch dice {
		case 0:
			rand.Read(data)
			if bytes.Equal(data, zeroes) {
				logging.Debug(ctx, "FillNbsDisk: rand generated all zeroes")
			}

			err = session.Write(ctx, blockIndex, data)
			storageSize += chunkSize
		case 1:
			err = session.Zero(ctx, blockIndex, blocksInChunk)
		}
		if err != nil {
			return 0, 0, err
		}

		_, err = acc.Write(data)
		if err != nil {
			return 0, 0, err
		}
	}

	return acc.Sum32(), storageSize, nil
}

////////////////////////////////////////////////////////////////////////////////

func CreateImage(
	t *testing.T,
	ctx context.Context,
	imageID string,
	imageSize uint64,
	folderID string,
	pooled bool,
) (crc32 uint32, storageSize uint64) {

	client, err := CreateClient(ctx)
	require.NoError(t, err)
	defer client.Close()

	diskID := "temporary_disk_for_image_" + imageID

	reqCtx := GetRequestContext(t, ctx)
	operation, err := client.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: int64(imageSize),
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

	nbsClient := CreateNbsClient(t, ctx)
	crc32, storageSize, err = FillNbsDisk(nbsClient, diskID, imageSize)
	require.NoError(t, err)

	reqCtx = GetRequestContext(t, ctx)
	operation, err = client.CreateImage(reqCtx, &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		},
		DstImageId: imageID,
		FolderId:   folderID,
		Pooled:     pooled,
	})
	require.NoError(t, err)
	require.NotEmpty(t, operation)

	response := disk_manager.CreateImageResponse{}
	err = internal_client.WaitResponse(ctx, client, operation.Id, &response)
	require.NoError(t, err)
	require.Equal(t, int64(imageSize), response.Size)
	require.Equal(t, int64(storageSize), response.StorageSize)

	return crc32, storageSize
}
