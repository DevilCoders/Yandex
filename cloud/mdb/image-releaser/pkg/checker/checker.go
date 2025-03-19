package checker

import (
	"context"
	"time"
)

//go:generate ../../../scripts/mockgen.sh Checker

type Checker interface {
	IsStable(ctx context.Context, since, now time.Time) error
}
