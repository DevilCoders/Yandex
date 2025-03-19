package iam

import (
	"context"

	iam_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/iam"
)

type Adapter struct {
	runCtx context.Context

	rmClient iam_ic.RMClient

	folders foldersResolveStorage
}

func New(runCtx context.Context, rmClient iam_ic.RMClient) (*Adapter, error) {
	adapter := &Adapter{
		runCtx:   runCtx,
		rmClient: rmClient,
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

func (s *Session) paramsBatchSize() int {
	if s.batchSizeOverride > 0 {
		return s.batchSizeOverride
	}
	return paramsBatchSize
}
