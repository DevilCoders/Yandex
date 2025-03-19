package date

import "time"

/*
stripTime remove time components from t.
t.Truncate(time.Hour * 24) did the different thing:

	t, _ := time.Parse(time.RFC3339, "2022-02-28T00:02:03+03:00")
	fmt.Printf("t: %s, t.Truncate(day): %s", t, t.Truncate(time.Hour*24))

out
	t: 2022-02-28 00:02:03 +0300 +0300, t.Truncate(day): 2022-02-27 03:00:00 +0300 +0300

*/
func stripTime(t time.Time) time.Time {
	year, month, day := t.Date()
	return time.Date(year, month, day, 0, 0, 0, 0, t.Location())
}

// Range returns calendar in from to dates range.
// The calendar here means slice of Time where each Time element is a day-start.
func Range(from, to time.Time) []time.Time {
	from = stripTime(from)
	to = stripTime(to)
	var ret []time.Time
	for date := from; !date.After(to); date = date.Add(time.Hour * 24) {
		ret = append(ret, date)
	}
	return ret
}
