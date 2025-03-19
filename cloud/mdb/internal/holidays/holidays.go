package holidays

import (
	"context"
	"time"
)

type DayType int

const (
	Weekday DayType = iota
	Weekend
	Holiday
)

type Day struct {
	Date time.Time
	Type DayType
}

//go:generate ../../scripts/mockgen.sh Calendar

type Calendar interface {
	// Range returns days in from to range
	Range(ctx context.Context, from, to time.Time) ([]Day, error)
}
