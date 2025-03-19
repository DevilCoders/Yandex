package tests

import (
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"context"
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	nfs_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem"
	filesystem_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/config"
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
		os.Getenv("KIKIMR_SERVER_PORT"),
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

func createServices(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
) (tasks.Scheduler, filesystem.Service) {

	pollForTaskUpdatesPeriod := "100ms"
	pollForTasksPeriodMin := "50ms"
	pollForTasksPeriodMax := "100ms"
	pollForStallingTasksPeriodMin := "100ms"
	pollForStallingTasksPeriodMax := "400ms"
	taskPingPeriod := "1s"
	taskStallingTimeout := "1s"
	taskWaitingTimeout := "1s"
	scheduleRegularTasksPeriodMin := "100ms"
	scheduleRegularTasksPeriodMax := "400ms"
	runnersCount := uint64(10)
	stalkingRunnersCount := uint64(5)
	endedTaskExpirationTimeout := "500s"
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
		TaskPingPeriod:                      &taskPingPeriod,
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

	resourceStorage, err := createResourceStorage(t, ctx, db)
	require.NoError(t, err)

	insecure := true
	nfsFactory := nfs.CreateFactory(
		&nfs_config.ClientConfig{
			Zones: map[string]*nfs_config.Zone{
				"zone": {
					Endpoints: []string{
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("NFS_SERVER_PORT"),
						),
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("NFS_SERVER_PORT"),
						),
					},
				},
			},
			Insecure: &insecure,
		},
		metrics.CreateEmptyRegistry(),
	)

	deletedFilesystemExpirationTimeout := "1s"
	clearDeletedFilesystemsTaskScheduleInterval := "100ms"
	clearDeletedFilesystemsLimit := uint32(10)
	filesystemConfig := &filesystem_config.FilesystemConfig{
		DeletedFilesystemExpirationTimeout:          &deletedFilesystemExpirationTimeout,
		ClearDeletedFilesystemsTaskScheduleInterval: &clearDeletedFilesystemsTaskScheduleInterval,
		ClearDeletedFilesystemsLimit:                &clearDeletedFilesystemsLimit,
	}

	err = filesystem.Register(
		ctx,
		filesystemConfig,
		taskScheduler,
		taskRegistry,
		resourceStorage,
		nfsFactory,
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
		filesystem.CreateService(taskScheduler, filesystemConfig, nfsFactory)
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

////////////////////////////////////////////////////////////////////////////////

func TestCreateDeleteFilesystem(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service := createServices(t, ctx, db)

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateFilesystem(reqCtx, &disk_manager.CreateFilesystemRequest{
		Size:      100500 * 4096,
		BlockSize: 4096,
		FilesystemId: &disk_manager.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
		CloudId:     "cloud",
		FolderId:    "folder",
		StorageKind: disk_manager.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_HDD,
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	taskID, err = service.DeleteFilesystem(reqCtx, &disk_manager.DeleteFilesystemRequest{
		FilesystemId: &disk_manager.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)
}

func TestCreateResizeFilesystem(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	scheduler, service := createServices(t, ctx, db)

	reqCtx := getRequestContext(t, ctx)
	taskID, err := service.CreateFilesystem(reqCtx, &disk_manager.CreateFilesystemRequest{
		Size:      100500 * 4096,
		BlockSize: 4096,
		FilesystemId: &disk_manager.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
		CloudId:     "cloud",
		FolderId:    "folder",
		StorageKind: disk_manager.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_HDD,
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	taskID, err = service.ResizeFilesystem(reqCtx, &disk_manager.ResizeFilesystemRequest{
		FilesystemId: &disk_manager.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
		Size: 200500 * 4096,
	})

	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	taskID, err = service.DeleteFilesystem(reqCtx, &disk_manager.DeleteFilesystemRequest{
		FilesystemId: &disk_manager.FilesystemId{
			ZoneId:       "zone",
			FilesystemId: "filesystem",
		},
	})
	require.NoError(t, err)
	require.NotEmpty(t, taskID)
	err = waitTaskFinished(ctx, scheduler, taskID)
	require.NoError(t, err)
}

func TestDescribeModel(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	_, service := createServices(t, ctx, db)

	reqCtx := getRequestContext(t, ctx)
	model, err := service.DescribeFilesystemModel(reqCtx, &disk_manager.DescribeFilesystemModelRequest{
		ZoneId:      "zone",
		BlockSize:   4096,
		Size:        100500 * 4096,
		StorageKind: disk_manager.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_SSD,
	})

	require.NoError(t, err)
	require.NotNil(t, model)

	require.Equal(t, model.Size, int64(100500*4096))
	require.Equal(t, model.BlockSize, int64(4096))
	require.NotEqual(t, model.ChannelsCount, int64(0))
	require.Equal(t, model.StorageKind, disk_manager.FilesystemStorageKind_FILESYSTEM_STORAGE_KIND_SSD)

	require.NotEqual(t, model.PerformanceProfile.MaxReadBandwidth, int64(0))
	require.NotEqual(t, model.PerformanceProfile.MaxReadIops, int64(0))
	require.NotEqual(t, model.PerformanceProfile.MaxWriteBandwidth, int64(0))
	require.NotEqual(t, model.PerformanceProfile.MaxWriteIops, int64(0))
}
