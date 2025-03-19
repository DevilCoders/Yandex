package csv

import "errors"

var (
	ErrNoDateField     = errors.New("no date fields")
	ErrInvalidSchedule = errors.New("invalid schedule format")
)
