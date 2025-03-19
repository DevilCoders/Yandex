package resources

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Valid struct {
	ID            string
	Role          string
	CPU           int
	GPU           int
	Memory        int64
	DiskTypeExtID string
	ZoneID        string
	DiskSizeRange optional.IntervalInt64
	DiskSizes     []int64
	MinHosts      int64
	MaxHosts      int64
}

func (v Valid) Validate() error {
	if !v.DiskSizeRange.Valid && len(v.DiskSizes) == 0 {
		return xerrors.Errorf("both disk size range and disk sizes are not set: %+v", v)
	}

	return nil
}
