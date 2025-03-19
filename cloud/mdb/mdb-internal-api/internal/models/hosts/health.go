package hosts

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
)

type Health struct {
	Status   Status
	Services []services.Health
	System   *system.Metrics
}

type Status int

const (
	StatusUnknown Status = iota
	StatusDead
	StatusDegraded
	StatusAlive
)

var mapStatusToString = map[Status]string{
	StatusUnknown:  "Unknown",
	StatusDead:     "Dead",
	StatusDegraded: "Degraded",
	StatusAlive:    "Alive",
}

func (h Status) String() string {
	str, ok := mapStatusToString[h]
	if !ok {
		panic(fmt.Sprintf("invalid host health %d", h))
	}

	return str
}
