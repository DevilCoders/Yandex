package misc

import (
	"context"
	"sync"
	"time"
)

type compoundContext struct {
	main      context.Context
	secondary context.Context

	done chan struct{}

	mtx sync.Mutex
	err error
}

func (c *compoundContext) Value(key interface{}) interface{} {
	v1, v2 := c.main.Value(key), c.secondary.Value(key)
	if v1 == nil {
		return v2
	}
	return v1
}

func (c *compoundContext) Deadline() (time.Time, bool) {
	t1, ok1 := c.main.Deadline()
	t2, ok2 := c.secondary.Deadline()
	switch {
	case !ok1:
		return t2, ok2
	case !ok2:
		return t1, ok1
	case t1.Before(t2):
		return t1, ok1
	default:
		return t2, ok2
	}
}

func (c *compoundContext) Done() <-chan struct{} {
	return c.done
}

func (c *compoundContext) Err() error {
	c.mtx.Lock()
	defer c.mtx.Unlock()
	if c.err != nil {
		return c.err
	}
	err1, err2 := c.main.Err(), c.secondary.Err()
	switch {
	case err1 != nil:
		c.err = err1
	case err2 != nil:
		c.err = err2
	}

	return c.err
}

// First context have priority against second on Value(key) and Err() methods
// hence comp.Value(key) will return first.Value(key) if first.Value(key) != nil and second.Value(key) != nil
// and comp.Err() will return first.Err()  if first.Err() != nil and second.Err() != nil
// At least one context should be canceled at some point, or else goroutine leakage will occur
// Panics if both contexts are not cancelable
func CompoundContext(main, secondary context.Context) context.Context {
	if main.Done() == nil && secondary.Done() == nil {
		panic("contexts with no cancel provided for compound context")
	}
	done := make(chan struct{})
	go func() {
		select {
		case <-main.Done():
		case <-secondary.Done():
		}
		close(done)
	}()
	return &compoundContext{main: main, secondary: secondary, done: done}
}
