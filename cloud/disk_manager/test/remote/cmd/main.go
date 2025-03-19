package main

import (
	"context"
	"fmt"
	"hash/crc32"
	"io/ioutil"
	"log"
	"math/rand"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"
	"github.com/spf13/cobra"

	operation_proto "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/api"
	internal_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	nbs_client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/pkg/client"
)

////////////////////////////////////////////////////////////////////////////////

const (
	testSuiteName           = "DiskManagerRemoteTests"
	cloudID                 = "DiskManagerRemoteTests"
	defaultFolderID         = "DiskManagerRemoteTests"
	dataplaneFolderID       = "DiskManagerDataplaneRemoteTests"
	dataplaneForURLFolderID = "DiskManagerDataplaneFromUrlRemoteTests"
	zoneID                  = "ru-central1-a"
	imageURL                = "https://s3.mdst.yandex.net/nbs-loadtest-images/ubuntu1604-ci-stable"
	// For better testing, disk size should be greater than SSD allocation unit
	// (32 GB).
	defaultDiskSize       = 33 << 30
	defaultDiskDataSize   = 4 << 30
	nonreplicatedDiskSize = 99857989632
	mirror2DiskSize       = 99857989632
	mirror3DiskSize       = 99857989632
	defaultBlockSize      = 4096
	largeBlockSize        = 128 * 1024
	imageSize             = 15246295040
	imageCrc32            = 0x46e1e060
)

var curLaunchID string
var lastReqNumber int

////////////////////////////////////////////////////////////////////////////////

func generateID() string {
	return fmt.Sprintf("%v_%v", testSuiteName, uuid.Must(uuid.NewV4()).String())
}

func getRequestContext(ctx context.Context) context.Context {
	if len(curLaunchID) == 0 {
		curLaunchID = generateID()
	}

	lastReqNumber++

	cookie := fmt.Sprintf("%v_%v", curLaunchID, lastReqNumber)
	ctx = headers.SetOutgoingIdempotencyKey(ctx, cookie)
	ctx = headers.SetOutgoingRequestID(ctx, cookie)
	return ctx
}

////////////////////////////////////////////////////////////////////////////////

func parseConfig(
	configFileName string,
	config *client_config.ClientConfig,
	nbsConfigFileName string,
	nbsConfig *nbs_client_config.ClientConfig,
) error {

	log.Printf("Reading DM client config file=%v", configFileName)

	configBytes, err := ioutil.ReadFile(configFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read Disk Manager config file %v: %w",
			configFileName,
			err,
		)
	}

	log.Printf("Parsing DM client config file as protobuf")

	err = proto.UnmarshalText(string(configBytes), config)
	if err != nil {
		return fmt.Errorf(
			"failed to parse Disk Manager config file %v as protobuf: %w",
			configFileName,
			err,
		)
	}

	if len(nbsConfigFileName) == 0 {
		return fmt.Errorf("nbs config file name should not be empty")
	}

	log.Printf("Reading NBS client config file=%v", nbsConfigFileName)

	nbsConfigBytes, err := ioutil.ReadFile(nbsConfigFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read NBS config file %v: %w",
			nbsConfigFileName,
			err,
		)
	}

	log.Printf("Parsing NBS client config file as protobuf")

	err = proto.UnmarshalText(string(nbsConfigBytes), nbsConfig)
	if err != nil {
		return fmt.Errorf(
			"failed to parse NBS config file %v as protobuf: %w",
			nbsConfigFileName,
			err,
		)
	}

	return nil
}

func waitOperation(
	ctx context.Context,
	client client.Client,
	operation *operation_proto.Operation,
) error {

	err := internal_client.WaitOperation(ctx, client, operation.Id)
	if err != nil {
		return fmt.Errorf("operation=%v failed: %w", operation, err)
	}

	log.Printf("Successfully done operation=%v", operation)
	return nil
}

func createContext(config *client_config.ClientConfig) context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.InfoLevel),
	)
}

func createNbsClient(
	ctx context.Context,
	config *nbs_client_config.ClientConfig,
) (nbs.Client, error) {

	factory, err := nbs.CreateFactory(ctx, config, metrics.CreateEmptyRegistry())
	if err != nil {
		return nil, err
	}

	return factory.GetClient(ctx, zoneID)
}

////////////////////////////////////////////////////////////////////////////////

func createEmptyDisk(
	ctx context.Context,
	client client.Client,
	diskID string,
	diskSize uint64,
	blockSize uint64,
	folderID string,
) error {

	req := &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size:      int64(diskSize),
		BlockSize: int64(blockSize),
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: zoneID,
			DiskId: diskID,
		},
		CloudId:  cloudID,
		FolderId: folderID,
	}

	log.Printf("Sending request=%v", req)

	operation, err := client.CreateDisk(getRequestContext(ctx), req)
	if err != nil {
		log.Printf("Create empty disk=%v failed: %v", diskID, err)
		return fmt.Errorf("create empty disk=%v failed: %w", diskID, err)
	}
	log.Printf(
		"Successfully scheduled create disk=%v operation=%v",
		diskID,
		operation,
	)

	return waitOperation(ctx, client, operation)
}

func fillNbsDisk(
	ctx context.Context,
	nbsClient nbs.Client,
	diskID string,
	diskSize uint64,
	diskDataSize uint64,
) (uint32, error) {

	session, err := nbsClient.MountRW(ctx, diskID)
	if err != nil {
		return 0, err
	}
	defer session.Close(ctx)

	chunkSize := uint32(4 * 1024 * 1024)
	blockSize := session.BlockSize()
	blocksInChunk := chunkSize / blockSize
	acc := crc32.NewIEEE()
	rand.Seed(time.Now().UnixNano())

	for i := uint64(0); i < diskSize; i += uint64(chunkSize) {
		blockIndex := i / uint64(blockSize)
		bytes := make([]byte, chunkSize)
		dice := rand.Intn(int(diskSize/diskDataSize) + 2)

		var err error
		switch dice {
		case 0:
			rand.Read(bytes)
			err = session.Write(ctx, blockIndex, bytes)
		case 1:
			err = session.Zero(ctx, blockIndex, blocksInChunk)
		}
		if err != nil {
			return 0, err
		}

		_, err = acc.Write(bytes)
		if err != nil {
			return 0, err
		}
	}

	return acc.Sum32(), nil
}

func sendCreateDiskFromImageRequest(
	ctx context.Context,
	client client.Client,
	imageID string,
	diskID string,
	diskSize uint64,
	blockSize uint64,
	folderID string,
) (*operation_proto.Operation, error) {

	req := &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: imageID,
		},
		Size:      int64(diskSize),
		BlockSize: int64(blockSize),
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: zoneID,
			DiskId: diskID,
		},
		CloudId:  cloudID,
		FolderId: folderID,
	}

	log.Printf("Sending request=%v", req)

	operation, err := client.CreateDisk(getRequestContext(ctx), req)
	if err != nil {
		log.Printf(
			"Create disk=%v request from image=%v failed: %v",
			diskID,
			imageID,
			err,
		)
		return nil, fmt.Errorf("create disk=%v request failed: %w", diskID, err)
	}
	log.Printf(
		"Successfully scheduled create disk=%v operation=%v",
		diskID,
		operation,
	)

	return operation, nil
}

func createDiskFromImage(
	ctx context.Context,
	client client.Client,
	imageID string,
	diskID string,
	diskSize uint64,
	blockSize uint64,
	folderID string,
) error {

	operation, err := sendCreateDiskFromImageRequest(
		ctx,
		client,
		imageID,
		diskID,
		diskSize,
		blockSize,
		folderID,
	)
	if err != nil {
		return err
	}

	return waitOperation(ctx, client, operation)
}

func createDiskFromSnapshot(
	ctx context.Context,
	client client.Client,
	snapshotID string,
	diskID string,
	diskKind disk_manager.DiskKind,
	diskSize uint64,
	blockSize uint64,
	folderID string,
) error {

	req := &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: snapshotID,
		},
		Size:      int64(diskSize),
		BlockSize: int64(blockSize),
		Kind:      diskKind,
		DiskId: &disk_manager.DiskId{
			ZoneId: zoneID,
			DiskId: diskID,
		},
		CloudId:  cloudID,
		FolderId: folderID,
	}

	log.Printf("Sending request=%v", req)

	operation, err := client.CreateDisk(getRequestContext(ctx), req)
	if err != nil {
		log.Printf(
			"Create disk=%v from snapshot=%v failed: %v",
			diskID,
			snapshotID,
			err,
		)
		return fmt.Errorf("create disk=%v failed: %w", diskID, err)
	}
	log.Printf(
		"Successfully scheduled create disk=%v operation=%v",
		diskID,
		operation,
	)

	return waitOperation(ctx, client, operation)
}

func sendDeleteDiskRequest(
	ctx context.Context,
	client client.Client,
	diskID string,
) (*operation_proto.Operation, error) {

	req := &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: zoneID,
			DiskId: diskID,
		},
	}

	log.Printf("Sending request=%v", req)

	operation, err := client.DeleteDisk(getRequestContext(ctx), req)
	if err != nil {
		log.Printf(
			"Delete disk=%v failed: %v",
			diskID,
			err,
		)
		return nil, fmt.Errorf("delete disk=%v failed: %w", diskID, err)
	}

	log.Printf(
		"Successfully scheduled delete disk=%v operation=%v",
		diskID,
		operation,
	)
	return operation, nil
}

func sendConfigurePoolRequest(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	imageID string,
	useImageSize bool,
) (*operation_proto.Operation, error) {

	req := &api.ConfigurePoolRequest{
		ImageId:      imageID,
		ZoneId:       zoneID,
		Capacity:     10,
		UseImageSize: useImageSize,
	}

	log.Printf("Sending request=%v", req)

	operation, err := privateClient.ConfigurePool(getRequestContext(ctx), req)
	if err != nil {
		log.Printf(
			"Configure pool request for image=%v failed: %v",
			imageID,
			err,
		)
		return nil, fmt.Errorf("configure pool request failed: %w", err)
	}
	log.Printf(
		"Successfully scheduled configure pool for image=%v operation=%v",
		imageID,
		operation,
	)

	return operation, nil
}

func configurePool(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	imageID string,
	useImageSize bool,
) error {

	/* Uncomment after NBS-2715.
	operation, err := sendConfigurePoolRequest(
		ctx,
		client,
		privateClient,
		imageID,
		useImageSize,
	)
	if err != nil {
		return err
	}

	return waitOperation(ctx, client, operation)
	*/
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func createImage(
	ctx context.Context,
	client client.Client,
	req *disk_manager.CreateImageRequest,
) error {

	log.Printf("Sending request=%v", req)

	operation, err := client.CreateImage(getRequestContext(ctx), req)
	if err != nil {
		log.Printf("Create image=%v failed: %v", req.DstImageId, err)
		return fmt.Errorf("create image=%v failed: %w", req.DstImageId, err)
	}
	log.Printf(
		"Successfully scheduled create image=%v operation=%v",
		req.DstImageId,
		operation,
	)

	return waitOperation(ctx, client, operation)
}

func createImageFromURL(
	ctx context.Context,
	client client.Client,
	imageID string,
	folderID string,
) error {

	req := &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcUrl{
			SrcUrl: &disk_manager.ImageUrl{
				Url: imageURL,
			},
		},
		DstImageId: imageID,
		FolderId:   folderID,
		Pooled:     true,
	}

	return createImage(ctx, client, req)
}

func createImageFromImage(
	ctx context.Context,
	client client.Client,
	srcImageID string,
	folderID string,
	dstImageID string,
) error {

	req := &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcImageId{
			SrcImageId: srcImageID,
		},
		DstImageId: dstImageID,
		FolderId:   folderID,
		Pooled:     true,
	}

	return createImage(ctx, client, req)
}

func createImageFromDisk(
	ctx context.Context,
	client client.Client,
	diskID string,
	folderID string,
	imageID string,
) error {

	req := &disk_manager.CreateImageRequest{
		Src: &disk_manager.CreateImageRequest_SrcDiskId{
			SrcDiskId: &disk_manager.DiskId{
				ZoneId: zoneID,
				DiskId: diskID,
			},
		},
		DstImageId: imageID,
		FolderId:   folderID,
		Pooled:     true,
	}

	return createImage(ctx, client, req)
}

func sendDeleteImageRequest(
	ctx context.Context,
	client client.Client,
	imageID string,
) (*operation_proto.Operation, error) {

	req := &disk_manager.DeleteImageRequest{
		ImageId: imageID,
	}

	log.Printf("Sending request=%v", req)

	operation, err := client.DeleteImage(getRequestContext(ctx), req)
	if err != nil {
		log.Printf("Delete image=%v failed: %v", imageID, err)
		return nil, fmt.Errorf("delete image=%v failed: %w", imageID, err)
	}

	log.Printf(
		"Successfully scheduled delete image=%v operation=%v",
		imageID,
		operation,
	)
	return operation, nil
}

func createSnapshot(
	ctx context.Context,
	client client.Client,
	diskID string,
	snapshotID string,
	folderID string,
) error {

	req := &disk_manager.CreateSnapshotRequest{
		Src: &disk_manager.DiskId{
			ZoneId: zoneID,
			DiskId: diskID,
		},
		SnapshotId: snapshotID,
		FolderId:   folderID,
	}

	log.Printf("Sending request=%v", req)

	operation, err := client.CreateSnapshot(getRequestContext(ctx), req)
	if err != nil {
		log.Printf(
			"Create snapshot=%v of disk=%v failed: %v",
			snapshotID,
			diskID,
			err,
		)
		return fmt.Errorf(
			"create snapshot=%v of disk=%v failed: %w",
			snapshotID,
			diskID,
			err,
		)
	}
	log.Printf(
		"Successfully scheduled create snapshot=%v operation=%v",
		snapshotID,
		operation,
	)

	return waitOperation(ctx, client, operation)
}

func sendDeleteSnapshotRequest(
	ctx context.Context,
	client client.Client,
	snapshotID string,
) (*operation_proto.Operation, error) {

	req := &disk_manager.DeleteSnapshotRequest{
		SnapshotId: snapshotID,
	}

	log.Printf("Sending request=%v", req)

	operation, err := client.DeleteSnapshot(getRequestContext(ctx), req)
	if err != nil {
		log.Printf("Delete snapshot=%v failed: %v", snapshotID, err)
		return nil, fmt.Errorf("delete snapshot=%v failed: %w", snapshotID, err)
	}

	log.Printf(
		"Successfully scheduled delete snapshot=%v operation=%v",
		snapshotID,
		operation,
	)
	return operation, nil
}

////////////////////////////////////////////////////////////////////////////////

type sideEffects struct {
	Disks     []string
	Images    []string
	Snapshots []string
}

////////////////////////////////////////////////////////////////////////////////

func cleanupResources(
	ctx context.Context,
	client client.Client,
	se sideEffects,
) error {

	operations := make([]*operation_proto.Operation, 0)

	for _, disk := range se.Disks {
		operation, err := sendDeleteDiskRequest(ctx, client, disk)
		if err != nil {
			return err
		}

		operations = append(operations, operation)
	}

	for _, image := range se.Images {
		operation, err := sendDeleteImageRequest(ctx, client, image)
		if err != nil {
			return err
		}

		operations = append(operations, operation)
	}

	for _, snapshot := range se.Snapshots {
		operation, err := sendDeleteSnapshotRequest(ctx, client, snapshot)
		if err != nil {
			return err
		}

		operations = append(operations, operation)
	}

	for _, operation := range operations {
		err := waitOperation(ctx, client, operation)
		if err != nil {
			return err
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func listDisks(
	ctx context.Context,
	privateClient internal_client.PrivateClient,
	folderID string,
) ([]string, error) {

	resp, err := privateClient.ListDisks(ctx, &api.ListDisksRequest{
		FolderId: folderID,
	})
	if err != nil {
		return nil, err
	}

	return resp.DiskIds, nil
}

func listImages(
	ctx context.Context,
	privateClient internal_client.PrivateClient,
	folderID string,
) ([]string, error) {

	resp, err := privateClient.ListImages(ctx, &api.ListImagesRequest{
		FolderId: folderID,
	})
	if err != nil {
		return nil, err
	}

	return resp.ImageIds, nil
}

func listSnapshots(
	ctx context.Context,
	privateClient internal_client.PrivateClient,
	folderID string,
) ([]string, error) {

	resp, err := privateClient.ListSnapshots(ctx, &api.ListSnapshotsRequest{
		FolderId: folderID,
	})
	if err != nil {
		return nil, err
	}

	return resp.SnapshotIds, nil
}

type ListFunc = func(
	ctx context.Context,
	privateClient internal_client.PrivateClient,
	folderID string,
) ([]string, error)

func listAllFolders(
	ctx context.Context,
	privateClient internal_client.PrivateClient,
	list ListFunc,
) ([]string, error) {

	ids1, err := list(ctx, privateClient, defaultFolderID)
	if err != nil {
		return nil, err
	}

	ids2, err := list(ctx, privateClient, dataplaneFolderID)
	if err != nil {
		return nil, err
	}

	return append(ids1, ids2...), nil
}

func cleanupResourcesFromPreviousRuns(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
) error {

	disks, err := listAllFolders(ctx, privateClient, listDisks)
	if err != nil {
		return err
	}

	images, err := listAllFolders(ctx, privateClient, listImages)
	if err != nil {
		return err
	}

	snapshots, err := listAllFolders(ctx, privateClient, listSnapshots)
	if err != nil {
		return err
	}

	se := sideEffects{
		Disks:     disks,
		Images:    images,
		Snapshots: snapshots,
	}
	return cleanupResources(ctx, client, se)
}

////////////////////////////////////////////////////////////////////////////////

func testCreateEmptyDisk(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	diskID := generateID()
	se := sideEffects{
		Disks: []string{diskID},
	}
	return se, createEmptyDisk(
		ctx,
		client,
		diskID,
		1<<40,
		defaultBlockSize,
		defaultFolderID,
	)
}

func testCreateImageFromURL(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	imageID := generateID()
	se := sideEffects{
		Images: []string{imageID},
	}
	return se, createImageFromURL(ctx, client, imageID, defaultFolderID)
}

func testCreateDataplaneImageFromURL(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	imageID := generateID()
	se := sideEffects{
		Images: []string{imageID},
	}
	return se, createImageFromURL(ctx, client, imageID, dataplaneForURLFolderID)
}

func testCreateDiskFromImageImpl(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
	shouldConfigurePool bool,
	useImageSize bool,
	folderID string,
) (sideEffects, error) {

	diskID := generateID()
	imageID := generateID()
	se := sideEffects{
		Disks:  []string{diskID},
		Images: []string{imageID},
	}

	err := createImageFromURL(ctx, client, imageID, folderID)
	if err != nil {
		return se, err
	}

	if shouldConfigurePool {
		err = configurePool(ctx, client, privateClient, imageID, useImageSize)
		if err != nil {
			return se, err
		}
	}

	err = createDiskFromImage(
		ctx,
		client,
		imageID,
		diskID,
		defaultDiskSize,
		defaultBlockSize,
		folderID,
	)
	if err != nil {
		return se, err
	}

	nbsClient, err := createNbsClient(ctx, nbsConfig)
	if err != nil {
		return se, err
	}

	err = nbsClient.ValidateCrc32(diskID, imageSize, imageCrc32)
	return se, err
}

func testCreateDiskFromImage(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateDiskFromImageImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		false, // shouldConfigureDiskPool
		false, // useImageSize
		defaultFolderID,
	)
}

func testCreateDiskFromDataplaneImage(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateDiskFromImageImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		false, // shouldConfigureDiskPool
		false, // useImageSize
		dataplaneForURLFolderID,
	)
}

// Test for NBS-2005.
func testCreateDiskFromImageUsingImageSize(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateDiskFromImageImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		true, // shouldConfigureDiskPool
		true, // useImageSize
		defaultFolderID,
	)
}

func testCreateDisksFromImage(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	imageID := generateID()
	se := sideEffects{
		Images: []string{imageID},
	}
	for i := 0; i < 200; i++ {
		se.Disks = append(se.Disks, generateID())
	}

	err := createImageFromURL(ctx, client, imageID, defaultFolderID)
	if err != nil {
		return se, err
	}

	operations := make([]*operation_proto.Operation, 0)
	for _, diskID := range se.Disks {
		operation, err := sendCreateDiskFromImageRequest(
			ctx,
			client,
			imageID,
			diskID,
			defaultDiskSize,
			defaultBlockSize,
			defaultFolderID,
		)
		if err != nil {
			return se, err
		}
		operations = append(operations, operation)
	}

	for _, operation := range operations {
		err := waitOperation(ctx, client, operation)
		if err != nil {
			return se, err
		}
	}

	return se, nil
}

func testRetireBaseDisks(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	imageID := generateID()
	se := sideEffects{
		Images: []string{imageID},
	}

	diskCount := 800
	diskCountToValidate := 20

	for i := 0; i < diskCount; i++ {
		se.Disks = append(se.Disks, generateID())
	}

	err := createImageFromURL(ctx, client, imageID, defaultFolderID)
	if err != nil {
		return se, err
	}

	useImageSize := false
	err = configurePool(ctx, client, privateClient, imageID, useImageSize)
	if err != nil {
		return se, err
	}

	operations := make([]*operation_proto.Operation, 0)
	for _, diskID := range se.Disks {
		operation, err := sendCreateDiskFromImageRequest(
			ctx,
			client,
			imageID,
			diskID,
			defaultDiskSize,
			defaultBlockSize,
			defaultFolderID,
		)
		if err != nil {
			return se, err
		}

		operations = append(operations, operation)
	}

	// Should wait for first disk creation in order to ensure that pool is
	// created.
	err = waitOperation(ctx, client, operations[0])
	if err != nil {
		return se, err
	}

	errs := make(chan error)

	for i := 0; i < diskCountToValidate; i++ {
		operation := operations[i]
		diskID := se.Disks[i]

		nbsClient, err := createNbsClient(ctx, nbsConfig)
		if err != nil {
			return se, err
		}

		go func() {
			err := waitOperation(ctx, client, operation)
			if err != nil {
				errs <- err
				return
			}

			err = nbsClient.ValidateCrc32(diskID, imageSize, imageCrc32)
			errs <- err
		}()
	}

	reqCtx := getRequestContext(ctx)
	operation, err := privateClient.RetireBaseDisks(reqCtx, &api.RetireBaseDisksRequest{
		ImageId: imageID,
		ZoneId:  zoneID,
	})
	if err != nil {
		return se, err
	}

	operations = append(operations, operation)

	for i := 0; i < diskCountToValidate; i++ {
		err := <-errs
		if err != nil {
			return se, err
		}
	}

	for _, operation := range operations {
		err := waitOperation(ctx, client, operation)
		if err != nil {
			return se, err
		}
	}

	return se, nil
}

func testCreateDiskFromSnapshotImpl(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
	diskKind disk_manager.DiskKind,
	diskSize uint64,
	blockSize uint64,
	folderID string,
) (sideEffects, error) {

	diskID1 := generateID()
	diskID2 := generateID()
	snapshotID := generateID()
	se := sideEffects{
		Disks:     []string{diskID1, diskID2},
		Snapshots: []string{snapshotID},
	}

	nbsClient, err := createNbsClient(ctx, nbsConfig)
	if err != nil {
		return se, err
	}

	err = createEmptyDisk(ctx, client, diskID1, diskSize, blockSize, folderID)
	if err != nil {
		return se, err
	}

	expectedCrc32, err := fillNbsDisk(ctx, nbsClient, diskID1, diskSize, defaultDiskDataSize)
	if err != nil {
		return se, err
	}

	err = createSnapshot(ctx, client, diskID1, snapshotID, folderID)
	if err != nil {
		return se, err
	}

	err = createDiskFromSnapshot(
		ctx,
		client,
		snapshotID,
		diskID2,
		diskKind,
		diskSize,
		blockSize,
		folderID,
	)
	if err != nil {
		return se, err
	}

	err = nbsClient.ValidateCrc32(diskID2, diskSize, expectedCrc32)
	return se, err
}

func testCreateDiskFromSnapshot(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateDiskFromSnapshotImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		disk_manager.DiskKind_DISK_KIND_SSD,
		defaultDiskSize,
		defaultBlockSize,
		defaultFolderID,
	)
}

func testDataplaneCreateDiskFromSnapshot(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateDiskFromSnapshotImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		disk_manager.DiskKind_DISK_KIND_SSD,
		defaultDiskSize,
		defaultBlockSize,
		dataplaneFolderID,
	)
}

func testDataplaneCreateLargeDiskFromSnapshot(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateDiskFromSnapshotImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		disk_manager.DiskKind_DISK_KIND_SSD,
		defaultDiskSize,
		largeBlockSize,
		dataplaneFolderID,
	)
}

func testDataplaneCreateNonreplicatedDiskFromSnapshot(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateDiskFromSnapshotImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		disk_manager.DiskKind_DISK_KIND_SSD_NONREPLICATED,
		nonreplicatedDiskSize,
		defaultBlockSize,
		dataplaneFolderID,
	)
}

func testCreateImageFromImageImpl(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
	folderID string,
) (sideEffects, error) {

	diskSize := uint64(4 * 1024 * 1024 * 1024)

	diskID1 := generateID()
	diskID2 := generateID()
	imageID1 := generateID()
	imageID2 := generateID()
	se := sideEffects{
		Disks:  []string{diskID1, diskID2},
		Images: []string{imageID1, imageID2},
	}

	nbsClient, err := createNbsClient(ctx, nbsConfig)
	if err != nil {
		return se, err
	}

	err = createEmptyDisk(ctx, client, diskID1, diskSize, defaultBlockSize, folderID)
	if err != nil {
		return se, err
	}

	expectedCrc32, err := fillNbsDisk(ctx, nbsClient, diskID1, diskSize, diskSize/2)
	if err != nil {
		return se, err
	}

	err = createImageFromDisk(ctx, client, diskID1, folderID, imageID1)
	if err != nil {
		return se, err
	}

	err = createImageFromImage(ctx, client, imageID1, folderID, imageID2)
	if err != nil {
		return se, err
	}

	err = createDiskFromImage(ctx, client, imageID2, diskID2, diskSize, defaultBlockSize, folderID)
	if err != nil {
		return se, err
	}

	err = nbsClient.ValidateCrc32(diskID2, uint64(diskSize), expectedCrc32)
	return se, err
}

func testDataplaneCreateImageFromImage(
	ctx context.Context,
	client client.Client,
	privateClient internal_client.PrivateClient,
	nbsConfig *nbs_client_config.ClientConfig,
) (sideEffects, error) {

	return testCreateImageFromImageImpl(
		ctx,
		client,
		privateClient,
		nbsConfig,
		dataplaneFolderID,
	)
}

////////////////////////////////////////////////////////////////////////////////

type testCase struct {
	Name string
	Run  func(
		context.Context,
		client.Client,
		internal_client.PrivateClient,
		*nbs_client_config.ClientConfig,
	) (sideEffects, error)
}

func runTests(
	config *client_config.ClientConfig,
	nbsConfig *nbs_client_config.ClientConfig,
) error {

	curLaunchID = generateID()

	ctx := createContext(config)

	log.Printf("Creating DM client with config=%v", config)

	client, err := internal_client.NewClient(ctx, config)
	if err != nil {
		return fmt.Errorf("failed to create client: %w", err)
	}
	defer client.Close()

	privateClient, err := internal_client.NewPrivateClientForCLI(ctx, config)
	if err != nil {
		return fmt.Errorf("failed to create private client: %w", err)
	}
	defer privateClient.Close()

	err = cleanupResourcesFromPreviousRuns(ctx, client, privateClient)
	if err != nil {
		return fmt.Errorf(
			"failed to cleanup resources from previous runs: %w",
			err,
		)
	}

	tests := []testCase{
		{
			Name: "testCreateEmptyDisk",
			Run:  testCreateEmptyDisk,
		},
		{
			Name: "testCreateImageFromURL",
			Run:  testCreateImageFromURL,
		},
		{
			Name: "testCreateDataplaneImageFromURL",
			Run:  testCreateDataplaneImageFromURL,
		},
		{
			Name: "testCreateDiskFromImage",
			Run:  testCreateDiskFromImage,
		},
		{
			Name: "testCreateDiskFromDataplaneImage",
			Run:  testCreateDiskFromDataplaneImage,
		},
		{
			Name: "testCreateDiskFromImageUsingImageSize",
			Run:  testCreateDiskFromImageUsingImageSize,
		},
		{
			Name: "testCreateDisksFromImage",
			Run:  testCreateDisksFromImage,
		},
		{
			Name: "testRetireBaseDisks",
			Run:  testRetireBaseDisks,
		},
		{
			Name: "testCreateDiskFromSnapshot",
			Run:  testCreateDiskFromSnapshot,
		},
		{
			Name: "testDataplaneCreateDiskFromSnapshot",
			Run:  testDataplaneCreateDiskFromSnapshot,
		},
		{
			Name: "testDataplaneCreateLargeDiskFromSnapshot",
			Run:  testDataplaneCreateLargeDiskFromSnapshot,
		},
		{
			Name: "testDataplaneCreateNonreplicatedDiskFromSnapshot",
			Run:  testDataplaneCreateNonreplicatedDiskFromSnapshot,
		},
		// TODO: testDataplaneCreateMirror2DiskFromSnapshot
		// TODO: testDataplaneCreateMirror3DiskFromSnapshot
		{
			Name: "testDataplaneCreateImageFromImage",
			Run:  testDataplaneCreateImageFromImage,
		},
	}

	log.Printf("Starting tests")

	for _, test := range tests {
		log.Printf("Starting test %v", test.Name)

		se, runErr := test.Run(ctx, client, privateClient, nbsConfig)
		if runErr == nil {
			log.Printf("Test %v success", test.Name)
		} else {
			log.Printf("Test %v failed: %v", test.Name, runErr)
		}

		err := cleanupResources(ctx, client, se)
		if err != nil {
			return fmt.Errorf(
				"test %v failed to cleanup resources from current run: %w",
				test.Name,
				err,
			)
		}

		if runErr != nil {
			return fmt.Errorf("test %v run failed: %w", test.Name, runErr)
		}
	}

	log.Printf("All tests successfully finished")
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var configFileName string
	config := &client_config.ClientConfig{}
	var nbsConfigFileName string
	nbsConfig := &nbs_client_config.ClientConfig{}

	rootCmd := &cobra.Command{
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			return parseConfig(
				configFileName,
				config,
				nbsConfigFileName,
				nbsConfig,
			)
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			return runTests(config, nbsConfig)
		},
	}

	rootCmd.PersistentFlags().StringVar(
		&configFileName,
		"disk-manager-client-config",
		"/etc/yc/disk-manager/client-config.txt",
		"Path to the Disk Manager client config file",
	)
	rootCmd.PersistentFlags().StringVar(
		&nbsConfigFileName,
		"nbs-client-config",
		"",
		"Path to the NBS client config file",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Failed to run: %v", err)
	}
}
