package teamintegration

import (
	"context"

	team_integration_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/team_integration"
)

type Adapter struct {
	runCtx context.Context

	tiClient team_integration_ic.TIClient

	abc abcResolveStorage
}

func New(runCtx context.Context, tiClient team_integration_ic.TIClient) (*Adapter, error) {
	adapter := &Adapter{
		runCtx:   runCtx,
		tiClient: tiClient,
	}
	return adapter, adapter.init()
}

func (a *Adapter) init() error {
	return nil
}

type Session struct {
	adapter *Adapter

	batchSizeOverride int
}

func (a *Adapter) Session() *Session {
	return &Session{adapter: a}
}
