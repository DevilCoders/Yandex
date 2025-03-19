package chmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

type ResourcePreset struct {
	ID        string
	ZoneIds   []string
	Cores     int64
	Memory    int64
	HostRoles []hosts.Role
}
