package httpapi

import (
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
)

type ConductorConfig struct {
	OAuth  secret.String         `json:"oauth"`
	Client httputil.ClientConfig `json:"client"`
}

func DefaultConductorConfig() ConductorConfig {
	return ConductorConfig{
		Client: httputil.ClientConfig{
			Name:      "MDB Conductor Client",
			Transport: httputil.DefaultTransportConfig(),
		},
	}
}
