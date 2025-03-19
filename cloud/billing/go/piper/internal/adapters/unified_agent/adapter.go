package unifiedagent

import (
	unifiedagent "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/unified_agent"
	"context"
)

type Adapter struct {
	runCtx context.Context

	uaClient unifiedagent.UAClient
}

func New(runCtx context.Context, uaClient unifiedagent.UAClient) (*Adapter, error) {
	adapter := &Adapter{
		runCtx:   runCtx,
		uaClient: uaClient,
	}
	return adapter, adapter.init()
}

func (a *Adapter) init() error {
	return nil
}

type Session struct {
	adapter *Adapter

	quantityMetrics []unifiedagent.SolomonMetric
}

func (a *Adapter) Session() *Session {
	return &Session{adapter: a}
}
