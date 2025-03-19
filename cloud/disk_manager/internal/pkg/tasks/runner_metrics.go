package tasks

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

const (
	checkTaskHangingPeriod = 15 * time.Second
)

////////////////////////////////////////////////////////////////////////////////

type runnerMetrics interface {
	OnExecutionStarted(state tasks_storage.TaskState)
	OnExecutionStopped()
	OnExecutionError(err error)
	OnError(err error)
}

////////////////////////////////////////////////////////////////////////////////

type taskMetrics struct {
	publicErrorsCounter          metrics.Counter
	wrongGenerationErrorsCounter metrics.Counter
	retriableErrorsCounter       metrics.Counter
	nonRetriableErrorsCounter    metrics.Counter
	nonCancellableErrorsCounter  metrics.Counter
	hangingTasksGauge            metrics.Gauge
	isTaskHanging                bool
	inflightTasksGauge           metrics.Gauge
	taskID                       string
	taskType                     string
}

type runnerMetricsImpl struct {
	registry                     metrics.Registry
	otherErrorsCounter           metrics.Counter
	wrongGenerationErrorsCounter metrics.Counter
	hangingTaskTimeout           time.Duration
	taskMetrics                  *taskMetrics
	taskMetricsMutex             sync.Mutex
	onExecutionStopped           func()
	logger                       logging.Logger
}

func (m *runnerMetricsImpl) setTaskHanging(value bool) {
	prevValue := m.taskMetrics.isTaskHanging
	m.taskMetrics.isTaskHanging = value

	gauge := m.taskMetrics.hangingTasksGauge
	switch {
	case !prevValue && value:
		m.logger.Fmt().Infof(
			"Task %v with id=%v is hanging",
			m.taskMetrics.taskType,
			m.taskMetrics.taskID,
		)
		gauge.Add(1)
	case prevValue && !value:
		gauge.Add(-1)
	}
}

func (m *runnerMetricsImpl) checkTaskHangingImpl(deadline time.Time) {
	if m.taskMetrics == nil {
		return
	}

	m.setTaskHanging(time.Now().After(deadline))
}

func (m *runnerMetricsImpl) checkTaskHanging(
	ctx context.Context,
	deadline time.Time,
) {

	m.taskMetricsMutex.Lock()
	defer m.taskMetricsMutex.Unlock()

	if ctx.Err() != nil {
		return
	}

	m.checkTaskHangingImpl(deadline)
}

func (m *runnerMetricsImpl) OnExecutionStarted(state tasks_storage.TaskState) {
	m.taskMetricsMutex.Lock()
	defer m.taskMetricsMutex.Unlock()

	subRegistry := m.registry.WithTags(map[string]string{
		"type": state.TaskType,
	})

	m.taskMetrics = &taskMetrics{
		publicErrorsCounter:          subRegistry.Counter("errors/public"),
		wrongGenerationErrorsCounter: subRegistry.Counter("errors/wrongGeneration"),
		retriableErrorsCounter:       subRegistry.Counter("errors/retriable"),
		nonRetriableErrorsCounter:    subRegistry.Counter("errors/nonRetriable"),
		nonCancellableErrorsCounter:  subRegistry.Counter("errors/nonCancellable"),
		hangingTasksGauge:            subRegistry.Gauge("hangingTasks"),
		inflightTasksGauge:           subRegistry.Gauge("inflightTasks"),
		taskID:                       state.ID,
		taskType:                     state.TaskType,
	}

	ctx, cancel := context.WithCancel(context.Background())
	m.onExecutionStopped = cancel

	hangingDeadline := state.CreatedAt.Add(m.hangingTaskTimeout)

	go func() {
		for {
			select {
			case <-ctx.Done():
				return
			case <-time.After(checkTaskHangingPeriod):
			}

			m.checkTaskHanging(ctx, hangingDeadline)
		}
	}()

	m.checkTaskHangingImpl(hangingDeadline)
	m.taskMetrics.inflightTasksGauge.Add(1)
}

func (m *runnerMetricsImpl) OnExecutionStopped() {
	m.taskMetricsMutex.Lock()
	defer m.taskMetricsMutex.Unlock()

	if m.taskMetrics == nil {
		// Nothing to do.
		return
	}

	m.setTaskHanging(false)
	m.taskMetrics.inflightTasksGauge.Add(-1)

	m.taskMetrics = nil
	m.onExecutionStopped()
}

func (m *runnerMetricsImpl) OnExecutionError(err error) {
	m.taskMetricsMutex.Lock()
	defer m.taskMetricsMutex.Unlock()

	if errors.IsPublic(err) {
		m.taskMetrics.publicErrorsCounter.Inc()
	} else if errors.Is(err, &errors.WrongGenerationError{}) {
		m.taskMetrics.wrongGenerationErrorsCounter.Inc()
	} else if errors.Is(err, &errors.InterruptExecutionError{}) {
		// InterruptExecutionError is not a failure.
	} else if errors.Is(err, &errors.NonCancellableError{}) {
		m.taskMetrics.nonCancellableErrorsCounter.Inc()
	} else if errors.Is(err, &errors.NonRetriableError{}) {
		e := &errors.NonRetriableError{}
		errors.As(err, &e)

		if !e.Silent {
			m.taskMetrics.nonRetriableErrorsCounter.Inc()
		}
	} else if errors.Is(err, &errors.RetriableError{}) {
		m.taskMetrics.retriableErrorsCounter.Inc()
	} else if errors.Is(err, &errors.DetailedError{}) {
		e := &errors.DetailedError{}
		errors.As(err, &e)

		if !e.Silent {
			m.taskMetrics.nonRetriableErrorsCounter.Inc()
		}
	} else {
		// All other execution errors should be interpreted as non retriable.
		m.taskMetrics.nonRetriableErrorsCounter.Inc()
	}
}

func (m *runnerMetricsImpl) OnError(err error) {
	m.taskMetricsMutex.Lock()
	defer m.taskMetricsMutex.Unlock()

	if errors.Is(err, &errors.WrongGenerationError{}) {
		if m.taskMetrics != nil {
			m.taskMetrics.wrongGenerationErrorsCounter.Inc()
		} else {
			m.wrongGenerationErrorsCounter.Inc()
		}
	} else {
		// Other errors.
		m.otherErrorsCounter.Inc()
	}
}

func newRunnerMetrics(
	ctx context.Context,
	registry metrics.Registry,
	hangingTaskTimeout time.Duration,
) *runnerMetricsImpl {

	return &runnerMetricsImpl{
		registry:                     registry,
		otherErrorsCounter:           registry.Counter("errors/other"),
		wrongGenerationErrorsCounter: registry.Counter("errors/wrongGeneration"),
		hangingTaskTimeout:           hangingTaskTimeout,
		onExecutionStopped:           func() {},
		logger:                       logging.GetLogger(ctx),
	}
}
