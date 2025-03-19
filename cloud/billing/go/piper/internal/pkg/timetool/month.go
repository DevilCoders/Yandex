package timetool

import "time"

// MonthStart calculates start of month which arg belongs to in arguments timezone
func MonthStart(t time.Time) time.Time {
	y, m, _ := t.Date()
	return time.Date(y, m, 1, 0, 0, 0, 0, t.Location())
}
