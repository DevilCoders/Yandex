package misc

import (
	"context"
	"time"
)

type noCancelInContext struct {
	context.Context
	deadline time.Time
	done     chan struct{}
}

func (c noCancelInContext) Deadline() (time.Time, bool) {
	return time.Time{}, false
}

func (c noCancelInContext) Done() <-chan struct{} {
	return c.done
}

func (c noCancelInContext) Err() error {
	select {
	case <-c.done:
		return c.Context.Err()
	default:
		return nil
	}
}

func WithNoCancelIn(ctx context.Context, duration time.Duration) context.Context {
	deadline := time.Now().Add(duration)
	done := make(chan struct{})
	go callAfterBothClosed(func() {
		close(done)
	}, time.After(duration), ctx.Done())
	return noCancelInContext{Context: ctx, deadline: deadline, done: done}
}

// callAfterBothClosed call function f after both c1 and c2 closed
func callAfterBothClosed(f func(), c1 <-chan time.Time, c2 <-chan struct{}) {
	// wait close first channel
	select {
	case <-c1:
		c1 = nil
	case <-c2:
		c2 = nil
	}

	// wait close second channel
	//noinspection GoNilness
	select {
	case <-c1:
	case <-c2:
	}

	f()
}

type noCancelContext struct {
	ctx context.Context
}

func (n noCancelContext) Deadline() (deadline time.Time, ok bool) {
	return time.Time{}, false
}

func (n noCancelContext) Done() <-chan struct{} {
	return nil
}

func (n noCancelContext) Err() error {
	return nil
}

func (n noCancelContext) Value(key interface{}) interface{} {
	return n.ctx.Value(key)
}

func WithNoCancel(ctx context.Context) context.Context {
	return noCancelContext{ctx: ctx}
}
