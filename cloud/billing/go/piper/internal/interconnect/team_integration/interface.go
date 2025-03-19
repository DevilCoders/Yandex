package teamintegration

import "context"

//go:generate mockery --name TIClient

type TIClient interface {
	ResolveABC(ctx context.Context, abcID int64) (result ResolvedFolder, err error)
	HealthCheck(context.Context) error
}
