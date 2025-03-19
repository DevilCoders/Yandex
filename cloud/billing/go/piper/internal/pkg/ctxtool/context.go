package ctxtool

import (
	"context"
	"time"
)

func NoCancel(ctx context.Context) context.Context {
	return &noCancelWrapper{ctx: ctx}
}

type noCancelWrapper struct {
	ctx context.Context
}

func (*noCancelWrapper) Deadline() (deadline time.Time, ok bool) {
	return
}

func (*noCancelWrapper) Done() <-chan struct{} {
	return nil
}

func (*noCancelWrapper) Err() error {
	return nil
}

func (w *noCancelWrapper) Value(key interface{}) interface{} {
	return w.ctx.Value(key)
}

func WithGlobalCancel(valuesCtx context.Context, runCtx context.Context) context.Context {
	return &mergedContext{
		values: valuesCtx,
		runCtx: runCtx,
	}
}

type mergedContext struct {
	values context.Context
	runCtx context.Context
}

func (m *mergedContext) Deadline() (deadline time.Time, ok bool) {
	return m.runCtx.Deadline()
}

func (m *mergedContext) Done() <-chan struct{} {
	return m.runCtx.Done()
}

func (m *mergedContext) Err() error {
	return m.runCtx.Err()
}

func (m *mergedContext) Value(key interface{}) interface{} {
	return m.values.Value(key)
}
