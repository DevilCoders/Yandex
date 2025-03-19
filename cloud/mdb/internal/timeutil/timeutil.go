package timeutil

import "time"

func SplitHours(start, finish time.Time) []time.Time {
	res := []time.Time{start}
	t := start.Truncate(time.Hour).Add(time.Hour)
	for t.Before(finish) {
		res = append(res, t)
		t = t.Add(time.Hour)
	}
	res = append(res, finish)
	return res
}
