package logbroker

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

const systemName = "logbroker"

type Adapter struct {
	runCtx context.Context

	writers Writers

	inflyLimiter limiter
}

func New(runCtx context.Context, w Writers) (*Adapter, error) {
	adapter := &Adapter{
		runCtx:  runCtx,
		writers: w,
	}
	return adapter, adapter.init()
}

func (a *Adapter) SetInflyLimit(n int) {
	a.inflyLimiter.SetSize(n)
}

func (a *Adapter) init() error {
	return nil
}

type Session struct {
	retrier

	adapter *Adapter

	invalidMetrics  []lbtypes.ShardMessage
	enrichedMetrics [][]lbtypes.ShardMessage

	scope entities.ProcessingScope
}

func (a *Adapter) Session() *Session {
	return &Session{adapter: a}
}

type Writers struct {
	IncorrectMetrics lbtypes.ShardProducer
	ReshardedMetrics lbtypes.ShardProducer
}
