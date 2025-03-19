package timetool

import (
	"time"
	_ "time/tzdata"
)

var defaultTz = ForceParseTz("Europe/Moscow")

func DefaultTz() *time.Location {
	return defaultTz
}

func ParseTz(name string) (*time.Location, error) {
	loc, err := time.LoadLocation(name)
	return loc, err
}

func ForceParseTz(name string) *time.Location {
	loc, err := time.LoadLocation(name)
	if err != nil {
		panic(err)
	}
	return loc
}
