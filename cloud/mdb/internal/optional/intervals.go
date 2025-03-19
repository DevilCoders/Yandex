package optional

import "a.yandex-team.ru/cloud/mdb/internal/intervals"

type IntervalInt64 struct {
	IntervalInt64 intervals.Int64
	Valid         bool
}

func NewIntervalInt64(v intervals.Int64) IntervalInt64 {
	return IntervalInt64{IntervalInt64: v, Valid: true}
}

func (o *IntervalInt64) Set(v intervals.Int64) {
	o.IntervalInt64 = v
	o.Valid = true
}

func (o *IntervalInt64) Get() (intervals.Int64, error) {
	if !o.Valid {
		return intervals.Int64{}, ErrMissing
	}

	return o.IntervalInt64, nil
}

func (o *IntervalInt64) Must() intervals.Int64 {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.IntervalInt64
}
