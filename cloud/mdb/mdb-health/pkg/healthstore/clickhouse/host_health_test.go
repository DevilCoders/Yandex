package clickhouse_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
)

func hostHealthWithMode() types.HostHealth {
	hh := testhelpers.NewHostHealth(3, 3)
	mode := &types.Mode{
		Timestamp:       time.Now(),
		Read:            true,
		Write:           true,
		UserFaultBroken: true,
	}
	return types.NewHostHealthWithStatusSystemAndMode(hh.ClusterID(), hh.FQDN(), hh.Services(), hh.System(), mode, hh.Status())
}

func TestStoreHostsHealth(t *testing.T) {
	b := initClickhouse(t)

	hh1 := hostHealthWithMode()
	hh2 := hostHealthWithMode()

	require.NoError(t, b.StoreHostsHealth(context.Background(), []healthstore.HostHealthToStore{
		{
			Health: hh1,
			TTL:    hostHealthTimeout,
		},
		{
			Health: hh2,
			TTL:    hostHealthTimeout,
		},
	}))
}
