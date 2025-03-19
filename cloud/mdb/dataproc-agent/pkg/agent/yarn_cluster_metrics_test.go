package agent

import (
	"testing"

	"github.com/stretchr/testify/require"
)

// curl --silent  http://rc1b-dataproc-m-oroklvlc46dem0du.mdb.cloud-preprod.yandex.net:8088/ws/v1/cluster/metrics | jq
const yarnClusterMetricsResponseJSON = `
{
  "clusterMetrics": {
    "appsSubmitted": 7,
    "appsCompleted": 7,
    "appsPending": 0,
    "appsRunning": 0,
    "appsFailed": 0,
    "appsKilled": 0,
    "reservedMB": 0,
    "availableMB": 12288,
    "allocatedMB": 0,
    "reservedVirtualCores": 0,
    "availableVirtualCores": 4,
    "allocatedVirtualCores": 0,
    "containersAllocated": 0,
    "containersReserved": 0,
    "containersPending": 0,
    "totalMB": 12288,
    "totalVirtualCores": 4,
    "utilizedMBPercent": 11,
    "utilizedVirtualCoresPercent": 0,
    "totalNodes": 4,
    "lostNodes": 0,
    "unhealthyNodes": 0,
    "decommissioningNodes": 0,
    "decommissionedNodes": 0,
    "rebootedNodes": 0,
    "activeNodes": 2,
    "shutdownNodes": 2,
    "totalUsedResourcesAcrossPartition": {
      "memory": 0,
      "vCores": 0,
      "resourceInformations": {
        "resourceInformation": [
          {
            "maximumAllocation": 9223372036854776000,
            "minimumAllocation": 0,
            "name": "memory-mb",
            "resourceType": "COUNTABLE",
            "units": "Mi",
            "value": 0
          },
          {
            "maximumAllocation": 9223372036854776000,
            "minimumAllocation": 0,
            "name": "vcores",
            "resourceType": "COUNTABLE",
            "units": "",
            "value": 0
          }
        ]
      }
    },
    "totalClusterResourcesAcrossPartition": {
      "memory": 12288,
      "vCores": 4,
      "resourceInformations": {
        "resourceInformation": [
          {
            "maximumAllocation": 9223372036854776000,
            "minimumAllocation": 0,
            "name": "memory-mb",
            "resourceType": "COUNTABLE",
            "units": "Mi",
            "value": 12288
          },
          {
            "maximumAllocation": 9223372036854776000,
            "minimumAllocation": 0,
            "name": "vcores",
            "resourceType": "COUNTABLE",
            "units": "",
            "value": 4
          }
        ]
      }
    },
    "totalReservedResourcesAcrossPartition": {
      "memory": 0,
      "vCores": 0,
      "resourceInformations": {
        "resourceInformation": [
          {
            "maximumAllocation": 9223372036854776000,
            "minimumAllocation": 0,
            "name": "memory-mb",
            "resourceType": "COUNTABLE",
            "units": "Mi",
            "value": 0
          },
          {
            "maximumAllocation": 9223372036854776000,
            "minimumAllocation": 0,
            "name": "vcores",
            "resourceType": "COUNTABLE",
            "units": "",
            "value": 0
          }
        ]
      }
    },
    "totalAllocatedContainersAcrossPartition": 0,
    "crossPartitionMetricsAvailable": true
  }
}
`

func Test_parseYarnClusterMetricsResponse(t *testing.T) {
	want := YarnClusterMetricsResponse{
		ClusterMetrics: ClusterMetrics{
			ReservedMB:            0,
			AvailableMB:           12288,
			AllocatedMB:           0,
			ReservedVirtualCores:  0,
			AvailableVirtualCores: 4,
			AllocatedVirtualCores: 0,
			ContainersAllocated:   0,
			ContainersReserved:    0,
			ContainersPending:     0,
			ActiveNodes:           2,
		},
	}
	got, err := parseYarnClusterMetricsResponse([]byte(yarnClusterMetricsResponseJSON))
	require.NoError(t, err)
	require.Equal(t, want, got)
}
