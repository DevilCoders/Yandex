package provider

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/health"
	"a.yandex-team.ru/library/go/core/log"
)

type Health struct {
	client client.MDBHealthClient
	l      log.Logger
}

var _ health.Health = &Health{}

func NewHealth(client client.MDBHealthClient, l log.Logger) *Health {
	return &Health{client: client, l: l}
}
