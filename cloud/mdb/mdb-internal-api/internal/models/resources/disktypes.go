package resources

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
)

type DiskType int

const (
	DiskTypeUnknown DiskType = iota
	DiskTypeSSD
	DiskTypeHDD
)

func (dt DiskType) Apply(space int64, r quota.Resources) quota.Resources {
	switch dt {
	case DiskTypeSSD:
		r.SSDSpace += space
	case DiskTypeHDD:
		r.HDDSpace += space
	}

	return r
}

type DiskTypes struct {
	extIDToDiskType map[string]DiskType
}

func NewDiskTypes(mapping map[string]DiskType) DiskTypes {
	return DiskTypes{extIDToDiskType: mapping}
}

func (dts DiskTypes) Apply(r quota.Resources, space int64, dtExtID string) (quota.Resources, error) {
	dt, ok := dts.extIDToDiskType[dtExtID]
	if !ok {
		return quota.Resources{}, semerr.InvalidInputf("disk type %q is not valid", dtExtID)
	}

	return dt.Apply(space, r), nil
}

func (dts DiskTypes) Validate(dtExtID string) error {
	if _, ok := dts.extIDToDiskType[dtExtID]; !ok {
		return semerr.InvalidInputf("disk type %q is not valid", dtExtID)
	}

	return nil
}

func (dts DiskTypes) IsNetworkDisk(dtExtID string) bool {
	return strings.HasPrefix(dtExtID, "network-")
}

func (dts DiskTypes) IsLocalDisk(dtExtID string) bool {
	return strings.HasPrefix(dtExtID, "local-")
}

const (
	LocalSSD                = "local-ssd"
	LocalHDD                = "local-hdd"
	NetworkSSD              = "network-ssd"
	NetworkHDD              = "network-hdd"
	NetworkSSDNonreplicated = "network-ssd-nonreplicated"
)
