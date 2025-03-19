package ready

import (
	"context"
	"sync"
)

type Aggregator struct {
	mu       sync.Mutex
	checkers []Checker
}

// Aggregator is itself a Checker
var _ Checker = &Aggregator{}

func (a *Aggregator) IsReady(ctx context.Context) error {
	checkers := a.loadCheckers()

	// TOOD: make it parallel
	for _, checker := range checkers {
		err := checker.IsReady(ctx)
		if err != nil {
			return err
		}
	}

	return nil
}

func (a *Aggregator) loadCheckers() []Checker {
	a.mu.Lock()
	defer a.mu.Unlock()
	return a.checkers[:]
}

func (a *Aggregator) Register(c Checker) {
	a.mu.Lock()
	defer a.mu.Unlock()
	a.checkers = append(a.checkers, c)
}
