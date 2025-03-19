package tasks

import (
	"context"
	"fmt"
	"sync"
)

////////////////////////////////////////////////////////////////////////////////

// Task factories are expected to be registered on a single thread at the
// program start.
type Registry struct {
	taskFactories     map[string](func() Task)
	taskFactoriesLock sync.RWMutex
}

func (r *Registry) Register(
	ctx context.Context,
	taskType string,
	taskFactory func() Task,
) error {

	r.taskFactoriesLock.Lock()
	defer r.taskFactoriesLock.Unlock()

	_, ok := r.taskFactories[taskType]
	if ok {
		return fmt.Errorf("Task factory with type %v already exists", taskType)
	}

	r.taskFactories[taskType] = taskFactory

	return nil
}

func (r *Registry) CreateTask(
	ctx context.Context,
	taskType string,
) (Task, error) {

	r.taskFactoriesLock.RLock()
	defer r.taskFactoriesLock.RUnlock()

	factory, ok := r.taskFactories[taskType]
	if !ok {
		return nil, fmt.Errorf(
			"Task factory with type %v can not be found",
			taskType,
		)
	}

	return factory(), nil
}

////////////////////////////////////////////////////////////////////////////////

func CreateRegistry() *Registry {
	return &Registry{
		taskFactories: make(map[string](func() Task)),
	}
}
