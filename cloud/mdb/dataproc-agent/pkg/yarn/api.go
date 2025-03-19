package yarn

import "context"

//go:generate ../../../scripts/mockgen.sh API

type API interface {
	SearchApplication(ctx context.Context, tag string) ([]Application, error)
}
