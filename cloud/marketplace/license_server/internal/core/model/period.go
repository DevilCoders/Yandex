package model

import (
	"fmt"
	"strconv"
	"strings"
	"time"

	"golang.org/x/xerrors"
)

var (
	ErrInvalidPeriod   = xerrors.New("invalid period, must be %v_%s_%s or %v_%s")
	ErrInvalidValue    = xerrors.New("invalid value in period")
	ErrInvalidMeasure  = xerrors.New("invalid measurement in period, must d, m or y")
	ErrUnexpectedError = xerrors.New("unexpected error")
)

type Duration string

const (
	DayDuration   Duration = "d"
	MonthDuration Duration = "m"
	YearDuration  Duration = "y"
)

type Period struct {
	value    int
	duration Duration
}

func DefaultPeriod() Period {
	return Period{
		value:    0,
		duration: DayDuration,
	}
}

func NewPeriod(period string) (Period, error) {
	strs := strings.Split(period, "_")
	if len(strs) != 2 {
		return Period{}, ErrInvalidPeriod
	}

	value, err := strconv.Atoi(strs[0])
	if err != nil || value < 0 {
		return Period{}, ErrInvalidValue
	}

	var dur Duration
	switch strs[1] {
	case "d":
		dur = DayDuration
	case "m":
		dur = MonthDuration
	case "y":
		dur = YearDuration
	default:
		return Period{}, ErrInvalidMeasure
	}

	return Period{value: value, duration: dur}, nil
}

func (p *Period) String() string {
	return fmt.Sprintf("%v_%s", p.value, p.duration)
}

func (p *Period) AddToTime(t time.Time) (time.Time, error) {
	switch p.duration {
	case DayDuration:
		return t.AddDate(0, 0, p.value), nil
	case MonthDuration:
		return t.AddDate(0, p.value, 0), nil
	case YearDuration:
		return t.AddDate(p.value, 0, 0), nil
	default:
		return t, ErrUnexpectedError
	}
}
