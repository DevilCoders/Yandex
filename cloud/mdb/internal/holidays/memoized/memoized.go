package memoized

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
)

type memoKey struct {
	from time.Time
	to   time.Time
}

type Calendar struct {
	calendar holidays.Calendar
	lock     sync.Mutex
	memo     map[memoKey][]holidays.Day
}

var _ holidays.Calendar = &Calendar{}

func New(calendar holidays.Calendar) *Calendar {
	return &Calendar{calendar: calendar, memo: make(map[memoKey][]holidays.Day)}
}

func (c *Calendar) Range(ctx context.Context, from, to time.Time) ([]holidays.Day, error) {
	key := memoKey{
		from: from.Truncate(time.Hour * 24),
		to:   to.Truncate(time.Hour * 24),
	}
	c.lock.Lock()
	defer c.lock.Unlock()

	if _, ok := c.memo[key]; !ok {
		ret, err := c.calendar.Range(ctx, key.from, key.to)
		if err != nil {
			return nil, err
		}
		c.memo[key] = ret
	}
	return c.memo[key], nil
}
