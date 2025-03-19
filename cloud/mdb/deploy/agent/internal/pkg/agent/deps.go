package agent

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/salt"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrSrvNotInitialized = xerrors.NewSentinel("/srv not initialized")
	ErrSrvMalformed      = xerrors.NewSentinel("/srv malformed")
)

//go:generate ../../../../../scripts/mockgen.sh CallManager,SrvManager

type CallManager interface {
	Changes() <-chan salt.Change
	Run(ctx context.Context, job salt.Job, progress bool)
	Shutdown()
}

type SrvManager interface {
	Version() (string, error)
	Update(image datasource.NamedReadCloser) error
}
