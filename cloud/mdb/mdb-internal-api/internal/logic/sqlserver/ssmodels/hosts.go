package ssmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

type HostSpec struct {
	ZoneID         string
	SubnetID       optional.String
	AssignPublicIP bool
}

type UpdateHostSpec struct {
	HostName       string
	AssignPublicIP bool
}

func CollectZones(hosts []HostSpec) []string {
	zones := make([]string, 0, len(hosts))
	for _, host := range hosts {
		zones = append(zones, host.ZoneID)
	}
	return zones
}
