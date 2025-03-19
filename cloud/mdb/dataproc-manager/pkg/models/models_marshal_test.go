package models

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

func TestClusterHealth_Marshaling(t *testing.T) {
	ch := ClusterHealth{
		Cid:    "cid1",
		Health: health.Alive,
		Services: map[service.Service]ServiceHealth{
			service.Hbase: ServiceHbase{
				BasicHealthService: BasicHealthService{Health: health.Alive},
				Regions:            123,
				Requests:           124,
				AverageLoad:        0.21,
			},
			service.Hdfs: ServiceHdfs{
				BasicHealthService:      BasicHealthService{Health: health.Degraded},
				PercentRemaining:        0.33,
				Used:                    123,
				Free:                    33,
				TotalBlocks:             4312,
				MissingBlocks:           1,
				MissingBlocksReplicaOne: 4,
			},
			service.Hive: ServiceHive{
				BasicHealthService: BasicHealthService{Health: health.Degraded},
				QueriesSucceeded:   123,
				QueriesExecuting:   5,
				QueriesFailed:      10,
				SessionsOpen:       1011,
				SessionsActive:     3,
			},
		},
	}

	data, err := ch.MarshalBinary()
	require.NoError(t, err)

	new := ClusterHealth{}
	err = new.UnmarshalBinary(data)
	require.NoError(t, err)

	require.Equal(t, ch, new)
}

func TestHostHealth_Marshaling(t *testing.T) {
	hh := HostHealth{
		Fqdn:   "host1",
		Health: health.Alive,
		Services: map[service.Service]ServiceHealth{
			service.Hbase: ServiceHbaseNode{
				BasicHealthService: BasicHealthService{Health: health.Alive},
				Requests:           123,
				HeapSizeMb:         567,
				MaxHeapSizeMb:      1024,
			},
		},
	}

	data, err := hh.MarshalBinary()
	require.NoError(t, err)

	new := HostHealth{}
	err = new.UnmarshalBinary(data)
	require.NoError(t, err)

	require.Equal(t, hh, new)
}
