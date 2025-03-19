package features

import (
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
)

var defaultFlags = Flags{
	localTimezone: timetool.DefaultTz(),
}

type Flags struct {
	dropDuplicates bool
	localTimezone  *time.Location
}

func (f Flags) DropDuplicates() bool {
	return f.dropDuplicates
}

func (f Flags) LocalTimezone() *time.Location {
	return f.localTimezone
}

func DropDuplicates(v bool) Setter {
	return setterFunc(
		func(f *Flags) { f.dropDuplicates = v },
	)
}

func LocalTimezone(v *time.Location) Setter {
	return setterFunc(
		func(f *Flags) { f.localTimezone = v },
	)
}

type setterFunc func(*Flags)

func (sf setterFunc) set(f *Flags) {
	sf(f)
}
