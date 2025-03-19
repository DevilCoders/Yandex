package models

import (
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
)

type SortOrder string

const (
	SortOrderAscending  SortOrder = "ASC"
	SortOrderDescending SortOrder = "DESC"
)

const (
	defaultFromTSModifier = -time.Hour
)

func DefaultFrom(clock clockwork.Clock) optional.Time {
	return optional.NewTime(clock.Now().Add(defaultFromTSModifier).UTC())
}

func DefaultTo(clock clockwork.Clock) optional.Time {
	return optional.NewTime(clock.Now().UTC())
}

type Criteria struct {
	Sources []LogSource
	From    optional.Time
	To      optional.Time
	Levels  []LogLevel
	Order   SortOrder
	Filters []sqlfilter.Term

	Offset optional.Int64
	Limit  optional.Int64
}
