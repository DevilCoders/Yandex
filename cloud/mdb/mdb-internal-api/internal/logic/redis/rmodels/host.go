package rmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

type HostSpec struct {
	ZoneID          string
	ReplicaPriority optional.Int64
	SubnetID        string
	ShardName       string
	AssignPublicIP  bool
}

type UpdateHostSpec struct {
	HostName        string
	ReplicaPriority optional.Int64
	AssignPublicIP  optional.Bool
}
