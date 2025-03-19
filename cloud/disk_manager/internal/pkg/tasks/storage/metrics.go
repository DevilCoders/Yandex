package storage

import (
	"sync"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/accounting"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
)

////////////////////////////////////////////////////////////////////////////////

type storageMetrics interface {
	OnTaskCreated(state TaskState, taskCount int)
	OnTaskUpdated(state TaskState)
}

////////////////////////////////////////////////////////////////////////////////

type taskMetrics struct {
	created      metrics.Counter
	timeTotal    metrics.Timer
	estimateMiss metrics.Timer
}

type storageMetricsImpl struct {
	registry metrics.Registry

	// By taskType
	taskMetrics       map[string]*taskMetrics
	tasksMetricsMutex sync.Mutex
}

func taskDurationBuckets() metrics.DurationBuckets {
	return metrics.NewDurationBuckets(
		5*time.Second, 10*time.Second, 30*time.Second,
		1*time.Minute, 2*time.Minute, 5*time.Minute,
		10*time.Minute, 30*time.Minute,
		1*time.Hour, 2*time.Hour, 5*time.Hour, 10*time.Hour,
	)
}

func (m *storageMetricsImpl) getOrCreateMetrics(taskType string) *taskMetrics {
	m.tasksMetricsMutex.Lock()
	defer m.tasksMetricsMutex.Unlock()

	t, ok := m.taskMetrics[taskType]
	if !ok {
		subRegistry := m.registry.WithTags(map[string]string{
			"type": taskType,
		})

		t = &taskMetrics{
			created:      subRegistry.Counter("created"),
			timeTotal:    subRegistry.DurationHistogram("time/total", taskDurationBuckets()),
			estimateMiss: subRegistry.DurationHistogram("time/estimateMiss", taskDurationBuckets()),
		}
		m.taskMetrics[taskType] = t
	}
	return t
}

////////////////////////////////////////////////////////////////////////////////

func (m *storageMetricsImpl) OnTaskCreated(state TaskState, taskCount int) {
	metrics := m.getOrCreateMetrics(state.TaskType)
	metrics.created.Add(int64(taskCount))

	accounting.OnTaskCreated(state.TaskType, state.CloudID, state.FolderID, taskCount)
}

func (m *storageMetricsImpl) OnTaskUpdated(state TaskState) {
	metrics := m.getOrCreateMetrics(state.TaskType)
	if state.Status == TaskStatusFinished {
		now := time.Now()
		metrics.timeTotal.RecordDuration(now.Sub(state.CreatedAt))

		if state.EstimatedTime.After(state.CreatedAt) {
			metrics.estimateMiss.RecordDuration(now.Sub(state.EstimatedTime))
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

func makeStorageMetrics(registry metrics.Registry) storageMetrics {
	return &storageMetricsImpl{
		registry:    registry,
		taskMetrics: make(map[string]*taskMetrics),
	}
}
