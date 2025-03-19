package provider

import (
	"context"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

func TestConsole_GetConnectionDomain_Compute(t *testing.T) {
	cfg := logic.Config{
		VTypes: map[environment.VType]string{
			environment.VTypeCompute: "cloud.yandex.net",
			environment.VTypePorto:   "db.yandex.net",
		},
		EnvironmentVType: environment.VTypeCompute,
	}

	console := New(cfg, nil, nil, nil, nil, nil, nil)
	domain, err := console.GetConnectionDomain(context.Background())

	require.NoError(t, err)
	require.Equal(t, "cloud.yandex.net", domain)
}

func TestConsole_GetConnectionDomain_Porto(t *testing.T) {
	cfg := logic.Config{
		VTypes: map[environment.VType]string{
			environment.VTypeCompute: "cloud.yandex.net",
			environment.VTypePorto:   "db.yandex.net",
		},
		EnvironmentVType: environment.VTypePorto,
	}

	console := New(cfg, nil, nil, nil, nil, nil, nil)
	domain, err := console.GetConnectionDomain(context.Background())

	require.NoError(t, err)
	require.Equal(t, "db.yandex.net", domain)
}
