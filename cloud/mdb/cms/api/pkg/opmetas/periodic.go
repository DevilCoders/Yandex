package opmetas

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

type PeriodicStateMeta struct {
	LastRun encodingutil.DateTime `json:"last_run"`
}

func NewPeriodicStateMeta(lastRun time.Time) *PeriodicStateMeta {
	return &PeriodicStateMeta{LastRun: encodingutil.DateTimeFromTime(lastRun)}
}
