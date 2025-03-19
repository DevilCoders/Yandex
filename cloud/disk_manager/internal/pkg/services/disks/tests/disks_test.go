package tests

import (
	"context"
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes/empty"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	nbs_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	snapshot_mocks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot/mocks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks"
	disks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	pools_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/config"
	pools_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	transfer_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	tasks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/config"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

func createYDB(ctx context.Context) (*persistence.YDBClient, error) {
	endpoint := fmt.Sprintf(
		"localhost:%v",
		os.Getenv("DISK_MANAGER_RECIPE_KIKIMR_PORT"),
	)
	database := "/Root"
	rootPath := "disk_manager"

	return persistence.CreateYDBClient(
		ctx,
		&persistence_config.PersistenceConfig{
			Endpoint: &endpoint,
			Database: &database,
			RootPath: &rootPath,
		},
	)
}

func createTaskStorage(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	config *tasks_config.TasksConfig,
	metricsRegistry metrics.Registry,
) (tasks_storage.Storage, error) {

	folder := fmt.Sprintf("%v/tasks", t.Name())
	config.StorageFolder = &folder

	err := tasks_storage.CreateYDBTables(ctx, config, db)
	if err != nil {
		return nil, err
	}

	return tasks_storage.CreateStorage(config, metricsRegistry, db)
}

func createPoolStorage(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	config *pools_config.PoolsConfig,
) (pools_storage.Storage, error) {

	folder := fmt.Sprintf("%v/pools", t.Name())
	config.StorageFolder = &folder

	err := pools_storage.CreateYDBTables(ctx, config, db)
	if err != nil {
		return nil, err
	}

	return pools_storage.CreateStorage(config, db)
}

func createResourceStorage(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
) (resources.Storage, error) {

	disksFolder := fmt.Sprintf("%v/disks", t.Name())
	imagesFolder := fmt.Sprintf("%v/images", t.Name())
	snapshotsFolder := fmt.Sprintf("%v/snapshots", t.Name())
	filesystemsFolder := fmt.Sprintf("%v/filesystems", t.Name())
	placementGroupsFolder := fmt.Sprintf("%v/placement_groups", t.Name())

	err := resources.CreateYDBTables(
		ctx,
		disksFolder,
		imagesFolder,
		snapshotsFolder,
		filesystemsFolder,
		placementGroupsFolder,
		db,
	)
	if err != nil {
		return nil, err
	}

	return resources.CreateStorage(
		disksFolder,
		imagesFolder,
		snapshotsFolder,
		filesystemsFolder,
		placementGroupsFolder,
		db,
	)
}

func createServicesWithSetup(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	setupSnapshotClient func(*snapshot_mocks.ClientMock),
) (tasks.Scheduler, disks.Service, pools_storage.Storage) {
	pollForTaskUpdatesPeriod := "1s"
	pollForTasksPeriodMin := "1s"
	pollForTasksPeriodMax := "2s"
	pollForStallingTasksPeriodMin := "1s"
	pollForStallingTasksPeriodMax := "2s"
	tasksPingPeriod := "1s"
	taskStallingTimeout := "5s"
	taskWaitingTimeout := "4s"
	scheduleRegularTasksPeriodMin := "1s"
	scheduleRegularTasksPeriodMax := "2s"
	runnersCount := uint64(10)
	stalkingRunnersCount := uint64(5)
	endedTaskExpirationTimeout := "1000s"
	clearEndedTasksTaskScheduleInterval := "6s"
	clearEndedTasksLimit := uint64(10)
	maxRetriableErrorCount := uint64(1000)
	hangingTaskTimeout := "100s"

	tasksConfig := &tasks_config.TasksConfig{
		PollForTaskUpdatesPeriod:            &pollForTaskUpdatesPeriod,
		PollForTasksPeriodMin:               &pollForTasksPeriodMin,
		PollForTasksPeriodMax:               &pollForTasksPeriodMax,
		PollForStallingTasksPeriodMin:       &pollForStallingTasksPeriodMin,
		PollForStallingTasksPeriodMax:       &pollForStallingTasksPeriodMax,
		TaskPingPeriod:                      &tasksPingPeriod,
		TaskStallingTimeout:                 &taskStallingTimeout,
		TaskWaitingTimeout:                  &taskWaitingTimeout,
		ScheduleRegularTasksPeriodMin:       &scheduleRegularTasksPeriodMin,
		ScheduleRegularTasksPeriodMax:       &scheduleRegularTasksPeriodMax,
		RunnersCount:                        &runnersCount,
		StalkingRunnersCount:                &stalkingRunnersCount,
		EndedTaskExpirationTimeout:          &endedTaskExpirationTimeout,
		ClearEndedTasksTaskScheduleInterval: &clearEndedTasksTaskScheduleInterval,
		ClearEndedTasksLimit:                &clearEndedTasksLimit,
		MaxRetriableErrorCount:              &maxRetriableErrorCount,
		HangingTaskTimeout:                  &hangingTaskTimeout,
	}

	taskStorage, err := createTaskStorage(
		t,
		ctx,
		db,
		tasksConfig,
		metrics.CreateEmptyRegistry(),
	)
	require.NoError(t, err)

	taskRegistry := tasks.CreateRegistry()
	taskScheduler, err := tasks.CreateScheduler(
		ctx,
		taskRegistry,
		taskStorage,
		tasksConfig,
	)
	require.NoError(t, err)

	rootCertsFile := os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE")

	nbsFactory, err := nbs.CreateFactory(
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
			RootCertsFile: &rootCertsFile,
		},
		metrics.CreateEmptyRegistry(),
	)
	require.NoError(t, err)

	maxActiveSlots := uint32(3)
	maxBaseDisksInflight := uint32(3)
	maxBaseDiskUnits := uint32(10)
	longTimeout := "1s"
	shortTimeout := "100ms"
	clearDeletedBaseDisksLimit := uint32(10)
	clearReleasedSlotsLimit := uint32(10)

	poolsConfig := &pools_config.PoolsConfig{
		MaxActiveSlots:                            &maxActiveSlots,
		MaxBaseDisksInflight:                      &maxBaseDisksInflight,
		MaxBaseDiskUnits:                          &maxBaseDiskUnits,
		ScheduleBaseDisksTaskScheduleInterval:     &shortTimeout,
		DeleteBaseDisksTaskScheduleInterval:       &shortTimeout,
		DeletedBaseDiskExpirationTimeout:          &longTimeout,
		ClearDeletedBaseDisksTaskScheduleInterval: &shortTimeout,
		ClearDeletedBaseDisksLimit:                &clearDeletedBaseDisksLimit,
		ReleasedSlotExpirationTimeout:             &longTimeout,
		ClearReleasedSlotsTaskScheduleInterval:    &shortTimeout,
		ClearReleasedSlotsLimit:                   &clearReleasedSlotsLimit,
	}
	poolStorage, err := createPoolStorage(t, ctx, db, poolsConfig)
	require.NoError(t, err)

	poolService := pools.CreateService(taskScheduler, poolStorage)

	snapshotFactory := snapshot_mocks.CreateFactoryMock()
	snapshotClient := snapshot_mocks.CreateClientMock()
	setupSnapshotClient(snapshotClient)
	snapshotFactory.On(
		"CreateClientFromZone",
		mock.Anything,
		"zone",
	).Return(snapshotClient, nil)

	resourceStorage, err := createResourceStorage(t, ctx, db)
	require.NoError(t, err)

	snapshotTaskPingPeriod := "100ms"
	snapshotTaskHeartbeatTimeout := "1s"
	snapshotTaskBackoffTimeout := "2s"
	transferConfig := &transfer_config.TransferConfig{
		SnapshotTaskPingPeriod:       &snapshotTaskPingPeriod,
		SnapshotTaskHeartbeatTimeout: &snapshotTaskHeartbeatTimeout,
		SnapshotTaskBackoffTimeout:   &snapshotTaskBackoffTimeout,
	}

	transferService := transfer.CreateService(taskScheduler)
	err = transfer.Register(ctx, transferConfig, taskRegistry, snapshotFactory, nbsFactory)
	require.NoError(t, err)

	err = pools.Register(
		ctx,
		poolsConfig,
		taskRegistry,
		taskScheduler,
		poolStorage,
		nbsFactory,
		snapshotFactory,
		transferService,
		resourceStorage,
	)
	require.NoError(t, err)

	deletedDiskExpirationTimeout := "1s"
	clearDeletedDisksTaskScheduleInterval := "100ms"
	clearDeletedDisksLimit := uint32(10)

	disksConfig := &disks_config.DisksConfig{
		DeletedDiskExpirationTimeout:          &deletedDiskExpirationTimeout,
		ClearDeletedDisksTaskScheduleInterval: &clearDeletedDisksTaskScheduleInterval,
		ClearDeletedDisksLimit:                &clearDeletedDisksLimit,
	}
	performanceConfig := &performance_config.PerformanceConfig{}

	err = disks.Register(
		ctx,
		disksConfig,
		performanceConfig,
		resourceStorage,
		taskRegistry,
		taskScheduler,
		poolService,
		nbsFactory,
		transferService,
	)
	require.NoError(t, err)

	err = tasks.StartRunners(
		ctx,
		taskStorage,
		taskRegistry,
		metrics.CreateEmptyRegistry(),
		tasksConfig,
		"localhost",
	)
	require.NoError(t, err)

	return taskScheduler,
		disks.CreateService(taskScheduler, disksConfig, nbsFactory, poolService),
		poolStorage
}

func createServices(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
) (tasks.Scheduler, disks.Service, pools_storage.Storage) {

	return createServicesWithSetup(
		t,
		ctx,
		db,
		func(c *snapshot_mocks.ClientMock) {
			c.On("TransferFromImageToDisk", mock.Anything, mock.Anything, mock.Anything).Return(nil)
			c.On("TransferFromSnapshotToDisk", mock.Anything, mock.Anything, mock.Anything).Return(nil)
			c.On("GetTaskStatus", mock.Anything, mock.Anything).Return(
				snapshot.TaskStatus{
					Finished: true,
					Progress: 0,
					Offset:   0,
					Error:    nil,
				},
				nil,
			)
			c.On("WaitForTask", mock.Anything, mock.Anything, mock.Anything, mock.Anything).Return(nil)
			c.On("Close").Return(nil)
		},
	)
}

////////////////////////////////////////////////////////////////////////////////

var lastReqNumber int

func getRequestContext(t *testing.T, ctx context.Context) context.Context {
	lastReqNumber++

	cookie := fmt.Sprintf("%v_%v", t.Name(), lastReqNumber)
	ctx = headers.SetIncomingIdempotencyKey(ctx, cookie)
	ctx = headers.SetIncomingRequestID(ctx, cookie)
	return ctx
}

////////////////////////////////////////////////////////////////////////////////

var defaultTimeout = 20 * time.Minute

func waitTaskFinished(
	ctx context.Context,
	scheduler tasks.Scheduler,
	taskID string,
) error {

	_, err := scheduler.WaitTaskResponse(ctx, taskID, defaultTimeout)
	return err
}

func waitTaskCancelled(
	ctx context.Context,
	scheduler tasks.Scheduler,
	taskID string,
) error {

	return scheduler.WaitTaskCancelled(ctx, taskID, defaultTimeout)
}

////////////////////////////////////////////////////////////////////////////////

func TestCreateEmptyDisk(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, _ := createServices(t, ctx, db)

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size:      4096,
		BlockSize: 4096,
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: "disk",
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	taskID, err = service.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: "disk",
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)
}

func TestCreateEmptyDiskWithoutBlockSize(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, _ := createServices(t, ctx, db)

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcEmpty{
			SrcEmpty: &empty.Empty{},
		},
		Size: 4096,
		Kind: disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: "disk",
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	taskID, err = service.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: "disk",
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)
}

func TestCreateOverlayDisk(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, poolStorage := createServices(t, ctx, db)

	err = poolStorage.ConfigurePool(ctx, "image", "zone", 11, 0)
	require.NoError(t, err)

	diskID := t.Name()

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: "image",
		},
		Size:      4096,
		BlockSize: 4096,
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	taskID, err = service.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)
}

func TestCreateOverlayDiskCancelsWhenTransferFails(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, poolStorage := createServicesWithSetup(t, ctx, db, func(c *snapshot_mocks.ClientMock) {
		c.On(
			"TransferFromImageToDisk",
			mock.Anything,
			mock.Anything,
			mock.Anything,
		).Return(fmt.Errorf("failure"))
		c.On("Close").Return(nil)
	})

	err = poolStorage.ConfigurePool(ctx, "image", "zone", 11, 0)
	require.NoError(t, err)

	diskID := t.Name()

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: "image",
		},
		Size:      4096,
		BlockSize: 4096,
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.Error(t, err)
	err = waitTaskCancelled(ctx, scheduler, taskID)
	require.NoError(t, err)
}

func TestCancelCreateOverlayDisk(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, poolStorage := createServices(t, ctx, db)

	err = poolStorage.ConfigurePool(ctx, "image", "zone", 11, 0)
	require.NoError(t, err)

	diskID := t.Name()

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcImageId{
			SrcImageId: "image",
		},
		Size:      4096,
		BlockSize: 4096,
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)

	cancelling, err := scheduler.CancelTask(ctx, taskID)
	require.NoError(t, err)

	if cancelling {
		err = waitTaskCancelled(ctx, scheduler, taskID)
		require.NoError(t, err)
	}
}

func TestCreateOverlayDisks1(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, poolStorage := createServices(t, ctx, db)

	err = poolStorage.ConfigurePool(ctx, "image", "zone", 11, 0)
	require.NoError(t, err)

	diskCount := 31
	tasks := make([]string, 0, diskCount)

	for i := 0; i < diskCount; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx := getRequestContext(t, ctx)
		taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
			Src: &disk_manager.CreateDiskRequest_SrcImageId{
				SrcImageId: "image",
			},
			Size:      4096,
			BlockSize: 4096,
			Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	for _, taskID := range tasks {
		err := waitTaskFinished(ctx, scheduler, taskID)
		require.NoError(t, err)
	}

	tasks = make([]string, 0, diskCount)

	for i := 0; i < diskCount; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx := getRequestContext(t, ctx)
		taskID, err := service.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	for _, taskID := range tasks {
		err := waitTaskFinished(ctx, scheduler, taskID)
		require.NoError(t, err)
	}
}

func TestCreateOverlayDisks2(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, poolStorage := createServices(t, ctx, db)

	err = poolStorage.ConfigurePool(ctx, "image", "zone", 11, 0)
	require.NoError(t, err)

	diskCount := 62
	tasks := make([]string, 0, diskCount)

	// Create first part of disks.
	for i := 0; i < diskCount/2; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx := getRequestContext(t, ctx)
		taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
			Src: &disk_manager.CreateDiskRequest_SrcImageId{
				SrcImageId: "image",
			},
			Size:      4096,
			BlockSize: 4096,
			Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	for _, taskID := range tasks {
		err := waitTaskFinished(ctx, scheduler, taskID)
		require.NoError(t, err)
	}

	tasks = make([]string, 0)

	// Delete first part of disks, but do not wait for completion.
	for i := 0; i < diskCount/2; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx := getRequestContext(t, ctx)
		taskID, err := service.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	// Create second part of disks, deletion of first part still in progress.
	for i := diskCount / 2; i < diskCount; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx := getRequestContext(t, ctx)
		taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
			Src: &disk_manager.CreateDiskRequest_SrcImageId{
				SrcImageId: "image",
			},
			Size:      4096,
			BlockSize: 4096,
			Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	for _, taskID := range tasks {
		err := waitTaskFinished(ctx, scheduler, taskID)
		require.NoError(t, err)
	}
}

func TestCreateOverlayDisksWhenBaseDiskCreationFails(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, poolStorage := createServicesWithSetup(t, ctx, db, func(c *snapshot_mocks.ClientMock) {
		c.On(
			"TransferFromImageToDisk",
			mock.Anything,
			mock.Anything,
			mock.Anything,
		).Return(fmt.Errorf("error"))
		c.On("Close").Return(nil)
	})

	err = poolStorage.ConfigurePool(ctx, "image", "zone", 1, 0)
	require.NoError(t, err)

	for i := 0; i < 2; i++ {
		diskID := fmt.Sprintf("%v_%v", t.Name(), i)

		reqCtx := getRequestContext(t, ctx)
		taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
			Src: &disk_manager.CreateDiskRequest_SrcImageId{
				SrcImageId: "image",
			},
			Size:      4096,
			BlockSize: 4096,
			Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		err = waitTaskFinished(ctx, scheduler, taskID)
		require.Error(t, err)
	}
}

func TestCancelOverlayDisksCreation(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, poolStorage := createServices(t, ctx, db)

	err = poolStorage.ConfigurePool(ctx, "image", "zone", 1, 0)
	require.NoError(t, err)

	diskCount := 10
	tasks := make([]string, 0, diskCount)

	for i := 0; i < diskCount; i++ {
		diskID := fmt.Sprintf("%v%v", t.Name(), i)

		reqCtx := getRequestContext(t, ctx)
		taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
			Src: &disk_manager.CreateDiskRequest_SrcImageId{
				SrcImageId: "image",
			},
			Size:      4096,
			BlockSize: 4096,
			Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
			DiskId: &disk_manager.DiskId{
				ZoneId: "zone",
				DiskId: diskID,
			},
		})
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	for _, taskID := range tasks {
		cancelling, err := scheduler.CancelTask(ctx, taskID)
		require.NoError(t, err)

		if cancelling {
			err = waitTaskCancelled(ctx, scheduler, taskID)
			require.NoError(t, err)
		}
	}
}

func TestCreateDiskFromSnapshot(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, _ := createServices(t, ctx, db)

	diskID := t.Name()

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: "snapshotID",
		},
		Size:      4096,
		BlockSize: 4096,
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	taskID, err = service.DeleteDisk(reqCtx, &disk_manager.DeleteDiskRequest{
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)
}

func TestCancelCreateDiskFromSnapshot(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service, _ := createServices(t, ctx, db)

	diskID := t.Name()

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateDisk(reqCtx, &disk_manager.CreateDiskRequest{
		Src: &disk_manager.CreateDiskRequest_SrcSnapshotId{
			SrcSnapshotId: "snapshotID",
		},
		Size:      4096,
		BlockSize: 4096,
		Kind:      disk_manager.DiskKind_DISK_KIND_SSD,
		DiskId: &disk_manager.DiskId{
			ZoneId: "zone",
			DiskId: diskID,
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)

	cancelling, err := scheduler.CancelTask(ctx, taskID)
	require.NoError(t, err)

	if cancelling {
		err = waitTaskCancelled(ctx, scheduler, taskID)
		require.NoError(t, err)
	}
}
