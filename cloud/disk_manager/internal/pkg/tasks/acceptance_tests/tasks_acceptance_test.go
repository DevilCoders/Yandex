package tests

import (
	"context"
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	node_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/node/config"
	recipe_tasks "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/tasks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

func createContext() (context.Context, func()) {
	ctx, cancel := context.WithCancel(context.Background())
	logger := logging.CreateStderrLogger(logging.DebugLevel)
	return logging.SetLogger(ctx, logger), cancel
}

var lastReqNumber int

func getRequestContext(t *testing.T, ctx context.Context) context.Context {
	lastReqNumber++

	cookie := fmt.Sprintf("%v_%v", t.Name(), lastReqNumber)
	ctx = headers.SetIncomingIdempotencyKey(ctx, cookie)
	ctx = headers.SetIncomingRequestID(ctx, cookie)
	return ctx
}

////////////////////////////////////////////////////////////////////////////////

type client struct {
	db        *persistence.YDBClient
	scheduler tasks.Scheduler
}

func newClient(ctx context.Context) (*client, error) {
	var config node_config.Config
	configString := os.Getenv("DISK_MANAGER_TASKS_ACCEPTANCE_TESTS_RECIPE_NODE0_CONFIG")

	if len(configString) == 0 {
		return nil, fmt.Errorf("node config should not be empty")
	}

	err := proto.UnmarshalText(configString, &config)
	if err != nil {
		return nil, err
	}

	db, err := persistence.CreateYDBClient(ctx, config.PersistenceConfig)
	if err != nil {
		return nil, err
	}

	storage, err := tasks_storage.CreateStorage(
		config.TasksConfig,
		metrics.CreateEmptyRegistry(),
		db,
	)
	if err != nil {
		return nil, err
	}

	registry := tasks.CreateRegistry()

	scheduler, err := tasks.CreateScheduler(
		ctx,
		registry,
		storage,
		config.TasksConfig,
	)
	if err != nil {
		return nil, err
	}

	err = registry.Register(ctx, "ChainTask", func() tasks.Task {
		return &recipe_tasks.ChainTask{}
	})
	if err != nil {
		return nil, err
	}

	return &client{
		db:        db,
		scheduler: scheduler,
	}, nil
}

func (c *client) Close(ctx context.Context) error {
	return c.db.Close(ctx)
}

func (c *client) execute(f func() error) error {
	for {
		err := f()
		if err != nil {
			if errors.CanRetry(err) {
				<-time.After(time.Second)
				continue
			}

			return err
		}

		return nil
	}
}

func (c *client) scheduleChain(
	t *testing.T,
	ctx context.Context,
	depth int,
) (string, error) {

	reqCtx := getRequestContext(t, ctx)

	var taskID string

	err := c.execute(func() error {
		var err error
		taskID, err = c.scheduler.ScheduleTask(
			reqCtx,
			"ChainTask",
			"",
			&protos.ChainTaskRequest{
				Depth: uint32(depth),
			},
			"",
			"",
		)
		return err
	})
	return taskID, err
}

func (c *client) cancelTask(
	ctx context.Context,
	taskID string,
) error {

	return c.execute(func() error {
		_, err := c.scheduler.CancelTask(ctx, taskID)
		return err
	})
}

////////////////////////////////////////////////////////////////////////////////

var defaultTimeout = 40 * time.Minute

func (c *client) waitTask(
	ctx context.Context,
	taskID string,
) error {

	_, err := c.scheduler.WaitTaskResponse(ctx, taskID, defaultTimeout)
	return err
}

func (c *client) waitTaskEnded(
	ctx context.Context,
	taskID string,
) error {

	return c.execute(func() error {
		return c.scheduler.WaitTaskEnded(ctx, taskID)
	})
}

////////////////////////////////////////////////////////////////////////////////

func TestTasksAcceptanceRunChains(t *testing.T) {
	ctx, cancel := createContext()
	defer cancel()

	client, err := newClient(ctx)
	require.NoError(t, err)
	defer client.Close(ctx)

	tasks := make([]string, 0)

	for i := 0; i < 30; i++ {
		taskID, err := client.scheduleChain(t, ctx, 2)
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	errs := make(chan error)

	for _, taskID := range tasks {
		go func(taskID string) {
			errs <- client.waitTask(ctx, taskID)
		}(taskID)
	}

	for range tasks {
		err := <-errs
		require.NoError(t, err)
	}
}

func TestTasksAcceptanceCancelChains(t *testing.T) {
	ctx, cancel := createContext()
	defer cancel()

	client, err := newClient(ctx)
	require.NoError(t, err)
	defer client.Close(ctx)

	tasks := make([]string, 0)

	for i := 0; i < 30; i++ {
		taskID, err := client.scheduleChain(t, ctx, 2)
		require.NoError(t, err)
		require.NotEmpty(t, taskID)

		tasks = append(tasks, taskID)
	}

	for _, taskID := range tasks {
		err := client.cancelTask(ctx, taskID)
		require.NoError(t, err)
	}

	errs := make(chan error)

	for _, taskID := range tasks {
		go func(taskID string) {
			errs <- client.waitTaskEnded(ctx, taskID)
		}(taskID)
	}

	for range tasks {
		err := <-errs
		require.NoError(t, err)
	}
}
