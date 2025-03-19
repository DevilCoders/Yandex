package tests

import (
	"context"
	"fmt"
	"os"
	"sync"
	"testing"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"
	"github.com/golang/protobuf/ptypes/wrappers"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	grpc_status "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	tasks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
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

func createStorage(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	config *tasks_config.TasksConfig,
	metricsRegistry metrics.Registry,
) tasks_storage.Storage {

	folder := fmt.Sprintf("tasks_ydb_test/%v", t.Name())
	config.StorageFolder = &folder

	err := tasks_storage.CreateYDBTables(ctx, config, db)
	require.NoError(t, err)

	storage, err := tasks_storage.CreateStorage(config, metricsRegistry, db)
	require.NoError(t, err)

	return storage
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

func makeDefaultConfig() *tasks_config.TasksConfig {
	pollForTaskUpdatesPeriod := "100ms"
	pollForTasksPeriodMin := "100ms"
	pollForTasksPeriodMax := "200ms"
	pollForStallingTasksPeriodMin := "100ms"
	pollForStallingTasksPeriodMax := "400ms"
	taskPingPeriod := "100ms"
	taskStallingTimeout := "1s"
	taskWaitingTimeout := "500ms"
	scheduleRegularTasksPeriodMin := "100ms"
	scheduleRegularTasksPeriodMax := "400ms"
	endedTaskExpirationTimeout := "300s"
	clearEndedTasksTaskScheduleInterval := "6s"
	clearEndedTasksLimit := uint64(10)
	maxRetriableErrorCount := uint64(2)
	hangingTaskTimeout := "100s"

	return &tasks_config.TasksConfig{
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
		EndedTaskExpirationTimeout:          &endedTaskExpirationTimeout,
		ClearEndedTasksTaskScheduleInterval: &clearEndedTasksTaskScheduleInterval,
		ClearEndedTasksLimit:                &clearEndedTasksLimit,
		MaxRetriableErrorCount:              &maxRetriableErrorCount,
		HangingTaskTimeout:                  &hangingTaskTimeout,
	}
}

////////////////////////////////////////////////////////////////////////////////

type services struct {
	config    *tasks_config.TasksConfig
	registry  *tasks.Registry
	scheduler tasks.Scheduler
	storage   tasks_storage.Storage
}

func createServicesWithConfig(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	config *tasks_config.TasksConfig,
) services {

	registry := tasks.CreateRegistry()

	storage := createStorage(
		t,
		ctx,
		db,
		config,
		metrics.CreateEmptyRegistry(),
	)

	scheduler, err := tasks.CreateScheduler(ctx, registry, storage, config)
	require.NoError(t, err)

	return services{
		config:    config,
		registry:  registry,
		scheduler: scheduler,
		storage:   storage,
	}
}

func createServices(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	runnersCount uint64,
	taskTypeWhitelist []string,
	taskTypeBlacklist []string,
) services {

	config := proto.Clone(makeDefaultConfig()).(*tasks_config.TasksConfig)
	config.RunnersCount = &runnersCount
	config.StalkingRunnersCount = &runnersCount
	config.TaskTypeWhitelist = taskTypeWhitelist
	config.TaskTypeBlacklist = taskTypeBlacklist
	return createServicesWithConfig(t, ctx, db, config)
}

func (s *services) startRunners(ctx context.Context) error {
	return tasks.StartRunners(ctx, s.storage, s.registry, metrics.CreateEmptyRegistry(), s.config, "localhost")
}

////////////////////////////////////////////////////////////////////////////////

type doublerTask struct {
	state *wrappers.UInt64Value
}

func (t *doublerTask) Init(ctx context.Context, request proto.Message) error {
	t.state = request.(*wrappers.UInt64Value)
	return nil
}

func (t *doublerTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *doublerTask) Load(ctx context.Context, state []byte) error {
	t.state = &wrappers.UInt64Value{}
	return proto.Unmarshal(state, t.state)
}

func (t *doublerTask) Run(ctx context.Context, execCtx tasks.ExecutionContext) error {
	t.state.Value *= 2
	return nil
}

func (t *doublerTask) Cancel(ctx context.Context, execCtx tasks.ExecutionContext) error {
	return nil
}

func (t *doublerTask) GetMetadata(ctx context.Context) (proto.Message, error) {
	return &empty.Empty{}, nil
}

func (t *doublerTask) GetResponse() proto.Message {
	return t.state
}

func registerDoublerTask(ctx context.Context, registry *tasks.Registry) error {
	return registry.Register(ctx, "doubler", func() tasks.Task {
		return &doublerTask{}
	})
}

func scheduleDoublerTask(
	ctx context.Context,
	scheduler tasks.Scheduler,
	request uint64,
) (string, error) {

	return scheduler.ScheduleTask(ctx, "doubler", "Doubler task", &wrappers.UInt64Value{
		Value: request,
	}, "", "")
}

////////////////////////////////////////////////////////////////////////////////

// Fails exactly n times in a row.
type unstableTask struct {
	failuresUntilSuccess uint64
	state                *wrappers.UInt64Value
}

func (t *unstableTask) Init(ctx context.Context, request proto.Message) error {
	t.state = request.(*wrappers.UInt64Value)
	return nil
}

func (t *unstableTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *unstableTask) Load(ctx context.Context, state []byte) error {
	t.state = &wrappers.UInt64Value{}
	return proto.Unmarshal(state, t.state)
}

func (t *unstableTask) Run(ctx context.Context, execCtx tasks.ExecutionContext) error {
	if t.state.Value == t.failuresUntilSuccess {
		return nil
	}

	t.state.Value++

	err := execCtx.SaveState(ctx)
	if err != nil {
		return err
	}

	return &errors.RetriableError{Err: assert.AnError}
}

func (t *unstableTask) Cancel(ctx context.Context, execCtx tasks.ExecutionContext) error {
	return nil
}

func (t *unstableTask) GetMetadata(ctx context.Context) (proto.Message, error) {
	return &empty.Empty{}, nil
}

func (t *unstableTask) GetResponse() proto.Message {
	return t.state
}

func registerUnstableTask(
	ctx context.Context,
	registry *tasks.Registry,
	failuresUntilSuccess uint64,
) error {

	return registry.Register(ctx, "unstable", func() tasks.Task {
		return &unstableTask{
			failuresUntilSuccess: failuresUntilSuccess,
		}
	})
}

func scheduleUnstableTask(
	ctx context.Context,
	scheduler tasks.Scheduler,
) (string, error) {

	return scheduler.ScheduleTask(
		ctx,
		"unstable",
		"Unstable task",
		&wrappers.UInt64Value{},
		"",
		"",
	)
}

////////////////////////////////////////////////////////////////////////////////

type failureTask struct {
	state   *empty.Empty
	failure error
}

func (t *failureTask) Init(ctx context.Context, request proto.Message) error {
	t.state = request.(*empty.Empty)
	return nil
}

func (t *failureTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *failureTask) Load(ctx context.Context, state []byte) error {
	t.state = &empty.Empty{}
	return proto.Unmarshal(state, t.state)
}

func (t *failureTask) Run(ctx context.Context, execCtx tasks.ExecutionContext) error {
	return t.failure
}

func (*failureTask) Cancel(ctx context.Context, execCtx tasks.ExecutionContext) error {
	return nil
}

func (t *failureTask) GetMetadata(ctx context.Context) (proto.Message, error) {
	return &empty.Empty{}, nil
}

func (t *failureTask) GetResponse() proto.Message {
	return t.state
}

func registerFailureTask(
	ctx context.Context,
	registry *tasks.Registry,
	failure error,
) error {

	return registry.Register(ctx, "failure", func() tasks.Task {
		return &failureTask{failure: failure}
	})
}

func scheduleFailureTask(
	ctx context.Context,
	scheduler tasks.Scheduler,
) (string, error) {

	return scheduler.ScheduleTask(
		ctx,
		"failure",
		"Failure task",
		&empty.Empty{},
		"",
		"",
	)
}

////////////////////////////////////////////////////////////////////////////////

type sixTimesTask struct {
	scheduler tasks.Scheduler
	state     *wrappers.UInt64Value
}

func (t *sixTimesTask) Init(ctx context.Context, request proto.Message) error {
	t.state = request.(*wrappers.UInt64Value)
	return nil
}

func (t *sixTimesTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *sixTimesTask) Load(ctx context.Context, state []byte) error {
	t.state = &wrappers.UInt64Value{}
	return proto.Unmarshal(state, t.state)
}

func (t *sixTimesTask) Run(ctx context.Context, execCtx tasks.ExecutionContext) error {
	id, err := scheduleDoublerTask(
		headers.SetIncomingIdempotencyKey(ctx, execCtx.GetTaskID()),
		t.scheduler,
		t.state.Value,
	)
	if err != nil {
		return err
	}

	response, err := t.scheduler.WaitTaskAsync(ctx, execCtx, id)
	if err != nil {
		return err
	}
	res := response.(*wrappers.UInt64Value).GetValue()

	t.state.Value = res * 3
	return nil
}

func (t *sixTimesTask) Cancel(ctx context.Context, execCtx tasks.ExecutionContext) error {
	return nil
}

func (t *sixTimesTask) GetMetadata(ctx context.Context) (proto.Message, error) {
	return &empty.Empty{}, nil
}

func (t *sixTimesTask) GetResponse() proto.Message {
	return &wrappers.UInt64Value{
		Value: t.state.Value,
	}
}

func registerSixTimesTask(ctx context.Context, registry *tasks.Registry, scheduler tasks.Scheduler) error {
	return registry.Register(ctx, "sixTimes", func() tasks.Task {
		return &sixTimesTask{
			scheduler: scheduler,
		}
	})
}

func scheduleSixTimesTask(
	ctx context.Context,
	scheduler tasks.Scheduler,
	request uint64,
) (string, error) {

	return scheduler.ScheduleTask(ctx, "sixTimes", "SixTimes task", &wrappers.UInt64Value{
		Value: request,
	}, "", "")
}

////////////////////////////////////////////////////////////////////////////////

var regularTaskMutex sync.Mutex
var regularTaskCounter int

type regularTask struct {
	state *empty.Empty
}

func (t *regularTask) Init(ctx context.Context, request proto.Message) error {
	t.state = &empty.Empty{}
	return nil
}

func (t *regularTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *regularTask) Load(ctx context.Context, state []byte) error {
	t.state = &empty.Empty{}
	return proto.Unmarshal(state, t.state)
}

func (t *regularTask) Run(ctx context.Context, execCtx tasks.ExecutionContext) error {
	regularTaskMutex.Lock()
	regularTaskCounter++
	regularTaskMutex.Unlock()
	return nil
}

func (t *regularTask) Cancel(ctx context.Context, execCtx tasks.ExecutionContext) error {
	return nil
}

func (t *regularTask) GetMetadata(ctx context.Context) (proto.Message, error) {
	return &empty.Empty{}, nil
}

func (t *regularTask) GetResponse() proto.Message {
	return &empty.Empty{}
}

////////////////////////////////////////////////////////////////////////////////

var defaultTimeout = 10 * time.Minute

func waitTaskResponseWithTimeout(
	ctx context.Context,
	scheduler tasks.Scheduler,
	id string,
	timeout time.Duration,
) (uint64, error) {

	response, err := scheduler.WaitTaskResponse(ctx, id, timeout)
	if err != nil {
		return 0, err
	}

	return response.(*wrappers.UInt64Value).GetValue(), nil
}

func waitTaskResponse(
	ctx context.Context,
	scheduler tasks.Scheduler,
	id string,
) (uint64, error) {

	return waitTaskResponseWithTimeout(ctx, scheduler, id, defaultTimeout)
}

////////////////////////////////////////////////////////////////////////////////

func TestTasksInitInfra(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = s.startRunners(ctx)
	require.NoError(t, err)
}

func TestTasksRunningOneTask(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id, err := scheduleDoublerTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id)
	require.NoError(t, err)
	require.EqualValues(t, 2*123, response)
}

func TestTasksRunningWhitelist(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, []string{"doubler"}, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = registerSixTimesTask(ctx, s.registry, s.scheduler)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id, err := scheduleDoublerTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id)
	require.NoError(t, err)
	require.EqualValues(t, 2*123, response)

	// sixTimes task doesn't exist in whitelist. Shouldn't be executed
	reqCtx = getRequestContext(t, ctx)
	id, err = scheduleSixTimesTask(reqCtx, s.scheduler, 100)
	require.NoError(t, err)

	_, err = waitTaskResponseWithTimeout(
		ctx,
		s.scheduler,
		id,
		10*time.Second,
	)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))
}

func TestTasksRunningBlacklist(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, []string{"sixTimes"})

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = registerSixTimesTask(ctx, s.registry, s.scheduler)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id, err := scheduleDoublerTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id)
	require.NoError(t, err)
	require.EqualValues(t, 2*123, response)

	// sixTimes task in blacklist. Shouldn't be executed
	reqCtx = getRequestContext(t, ctx)
	id, err = scheduleSixTimesTask(reqCtx, s.scheduler, 100)
	require.NoError(t, err)

	_, err = waitTaskResponseWithTimeout(
		ctx,
		s.scheduler,
		id,
		10*time.Second,
	)
	require.Error(t, err)
	require.True(t, errors.Is(err, &errors.NonRetriableError{}))
}

func TestTasksShouldRestoreRunningAfterRetriableError(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = registerUnstableTask(
		ctx,
		s.registry,
		makeDefaultConfig().GetMaxRetriableErrorCount(),
	)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id, err := scheduleUnstableTask(reqCtx, s.scheduler)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id)
	require.NoError(t, err)
	require.EqualValues(t, 2, response)
}

func TestTasksShouldFailRunningAfterRetriableErrorCountExceeded(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = registerUnstableTask(
		ctx,
		s.registry,
		makeDefaultConfig().GetMaxRetriableErrorCount()+1,
	)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id, err := scheduleUnstableTask(reqCtx, s.scheduler)
	require.NoError(t, err)

	_, err = waitTaskResponse(ctx, s.scheduler, id)
	require.Error(t, err)

	expected := &errors.RetriableError{Err: assert.AnError}

	status, ok := grpc_status.FromError(err)
	require.True(t, ok)
	require.Equal(t, expected.Error(), status.Message())
}

func TestTasksShouldNotRestoreRunningAfterNonRetriableError(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	failure := &errors.NonRetriableError{Err: assert.AnError}

	err = registerFailureTask(ctx, s.registry, failure)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id, err := scheduleFailureTask(reqCtx, s.scheduler)
	require.NoError(t, err)

	_, err = waitTaskResponse(ctx, s.scheduler, id)
	require.Error(t, err)

	status, ok := grpc_status.FromError(err)
	require.True(t, ok)
	require.Equal(t, failure.Error(), status.Message())
}

func TestTasksRunningTwoConcurrentTasks(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id1, err := scheduleDoublerTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	id2, err := scheduleDoublerTask(reqCtx, s.scheduler, 456)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id1)
	require.NoError(t, err)
	require.EqualValues(t, 2*123, response)

	response, err = waitTaskResponse(ctx, s.scheduler, id2)
	require.NoError(t, err)
	require.EqualValues(t, 2*456, response)
}

func TestTasksRunningTwoConcurrentTasksReverseWaiting(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id1, err := scheduleDoublerTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	id2, err := scheduleDoublerTask(reqCtx, s.scheduler, 456)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id2)
	require.NoError(t, err)
	require.EqualValues(t, 2*456, response)

	response, err = waitTaskResponse(ctx, s.scheduler, id1)
	require.NoError(t, err)
	require.EqualValues(t, 2*123, response)
}

func TestTasksRunningTwoConcurrentTasksOnOneRunner(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 1, nil, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id1, err := scheduleDoublerTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	id2, err := scheduleDoublerTask(reqCtx, s.scheduler, 456)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id1)
	require.NoError(t, err)
	require.EqualValues(t, 2*123, response)

	response, err = waitTaskResponse(ctx, s.scheduler, id2)
	require.NoError(t, err)
	require.EqualValues(t, 2*456, response)
}

func TestTasksRunningTwoConcurrentTasksOnOneRunnerReverseWaiting(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 1, nil, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id1, err := scheduleDoublerTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	id2, err := scheduleDoublerTask(reqCtx, s.scheduler, 456)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id2)
	require.NoError(t, err)
	require.EqualValues(t, 2*456, response)

	response, err = waitTaskResponse(ctx, s.scheduler, id1)
	require.NoError(t, err)
	require.EqualValues(t, 2*123, response)
}

func TestTasksRunningDependentTask(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	err = registerSixTimesTask(reqCtx, s.registry, s.scheduler)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx = getRequestContext(t, ctx)
	id, err := scheduleSixTimesTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id)
	require.NoError(t, err)
	require.EqualValues(t, 6*123, response)
}

func TestTasksRunningDependentTaskOnOneRunner(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 1, nil, nil)

	err = registerDoublerTask(ctx, s.registry)
	require.NoError(t, err)

	err = registerSixTimesTask(ctx, s.registry, s.scheduler)
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	reqCtx := getRequestContext(t, ctx)
	id, err := scheduleSixTimesTask(reqCtx, s.scheduler, 123)
	require.NoError(t, err)

	response, err := waitTaskResponse(ctx, s.scheduler, id)

	require.NoError(t, err)
	require.EqualValues(t, 6*123, response)
}

func TestTasksRunningRegularTasks(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	db, err := createYDB(ctx)
	require.NoError(t, err)
	defer db.Close(ctx)

	s := createServices(t, ctx, db, 2, nil, nil)

	err = s.registry.Register(ctx, "regular", func() tasks.Task {
		return &regularTask{}
	})
	require.NoError(t, err)

	err = s.startRunners(ctx)
	require.NoError(t, err)

	regularTaskMutex.Lock()
	regularTaskCounter = 0
	regularTaskMutex.Unlock()

	s.scheduler.ScheduleRegularTasks(ctx, "regular", "Regular task", time.Millisecond, 2)

	for {
		<-time.After(10 * time.Millisecond)

		regularTaskMutex.Lock()

		if regularTaskCounter > 4 {
			regularTaskMutex.Unlock()
			break
		}

		regularTaskMutex.Unlock()
	}
}
