package racktables

import (
	"context"
)

//go:generate ../../scripts/mockgen.sh Client

type Client interface {
	GetMacro(ctx context.Context, name string) (Macro, error)
	GetMacrosWithOwners(ctx context.Context) (MacrosWithOwners, error)
}
