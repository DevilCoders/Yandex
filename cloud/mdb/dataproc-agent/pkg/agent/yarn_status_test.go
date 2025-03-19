package agent

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

// curl http://$(hostname):8088/ws/v1/cluster/nodes | jq
const yarnAPIResponseJSON = `
{
  "nodes": {
    "node": [
      {
        "rack": "/default-rack",
        "state": "RUNNING",
        "id": "rc1b-dataproc-d-w4ly1997k9rgv705.mdb.cloud-preprod.yandex.net:34381",
        "nodeHostName": "rc1b-dataproc-d-w4ly1997k9rgv705.mdb.cloud-preprod.yandex.net",
        "nodeHTTPAddress": "rc1b-dataproc-d-w4ly1997k9rgv705.mdb.cloud-preprod.yandex.net:8042",
        "lastHealthUpdate": 1585330403286,
        "version": "2.10.0",
        "healthReport": "",
        "numContainers": 0,
        "usedMemoryMB": 0,
        "availMemoryMB": 6144,
        "usedVirtualCores": 0,
        "availableVirtualCores": 2,
        "numRunningOpportContainers": 0,
        "usedMemoryOpportGB": 0,
        "usedVirtualCoresOpport": 0,
        "numQueuedContainers": 0,
        "resourceUtilization": {
          "nodePhysicalMemoryMB": 1149,
          "nodeVirtualMemoryMB": 1149,
          "nodeCPUUsage": 0.0033322228118777275,
          "aggregatedContainersPhysicalMemoryMB": 0,
          "aggregatedContainersVirtualMemoryMB": 0,
          "containersCPUUsage": 0
        },
        "usedResource": {
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
        "availableResource": {
          "memory": 6144,
          "vCores": 2,
          "resourceInformations": {
            "resourceInformation": [
              {
                "maximumAllocation": 9223372036854776000,
                "minimumAllocation": 0,
                "name": "memory-mb",
                "resourceType": "COUNTABLE",
                "units": "Mi",
                "value": 6144
              },
              {
                "maximumAllocation": 9223372036854776000,
                "minimumAllocation": 0,
                "name": "vcores",
                "resourceType": "COUNTABLE",
                "units": "",
                "value": 2
              }
            ]
          }
        }
      },
      {
        "rack": "/default-rack",
        "state": "RUNNING",
        "id": "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net:40796",
        "nodeHostName": "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net",
        "nodeHTTPAddress": "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net:8042",
        "lastHealthUpdate": 1585330317308,
        "version": "2.10.0",
        "healthReport": "",
        "numContainers": 1,
        "usedMemoryMB": 0,
        "availMemoryMB": 6144,
        "usedVirtualCores": 0,
        "availableVirtualCores": 2,
        "numRunningOpportContainers": 0,
        "usedMemoryOpportGB": 0,
        "usedVirtualCoresOpport": 0,
        "numQueuedContainers": 0,
        "resourceUtilization": {
          "nodePhysicalMemoryMB": 845,
          "nodeVirtualMemoryMB": 845,
          "nodeCPUUsage": 0.0033333334140479565,
          "aggregatedContainersPhysicalMemoryMB": 0,
          "aggregatedContainersVirtualMemoryMB": 0,
          "containersCPUUsage": 0
        },
        "usedResource": {
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
        "availableResource": {
          "memory": 6144,
          "vCores": 2,
          "resourceInformations": {
            "resourceInformation": [
              {
                "maximumAllocation": 9223372036854776000,
                "minimumAllocation": 0,
                "name": "memory-mb",
                "resourceType": "COUNTABLE",
                "units": "Mi",
                "value": 6144
              },
              {
                "maximumAllocation": 9223372036854776000,
                "minimumAllocation": 0,
                "name": "vcores",
                "resourceType": "COUNTABLE",
                "units": "",
                "value": 2
              }
            ]
          }
        }
      },
      {
        "rack": "/default-rack",
        "state": "DECOMMISSIONED",
        "id": "rc1b-dataproc-c-2xwiveffsecaca8t.mdb.cloud-preprod.yandex.net:39561",
        "nodeHostName": "rc1b-dataproc-c-2xwiveffsecaca8t.mdb.cloud-preprod.yandex.net",
        "nodeHTTPAddress": "",
        "lastHealthUpdate": 1585320022282,
        "version": "2.10.0",
        "healthReport": "",
        "numContainers": 0,
        "usedMemoryMB": 0,
        "availMemoryMB": 0,
        "usedVirtualCores": 0,
        "availableVirtualCores": 0,
        "numRunningOpportContainers": 0,
        "usedMemoryOpportGB": 0,
        "usedVirtualCoresOpport": 0,
        "numQueuedContainers": 0,
        "resourceUtilization": {
          "nodePhysicalMemoryMB": 745,
          "nodeVirtualMemoryMB": 745,
          "nodeCPUUsage": 0,
          "aggregatedContainersPhysicalMemoryMB": 0,
          "aggregatedContainersVirtualMemoryMB": 0,
          "containersCPUUsage": 0
        }
      },
      {
        "rack": "/default-rack",
        "state": "SHUTDOWN",
        "id": "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net:38662",
        "nodeHostName": "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net",
        "nodeHTTPAddress": "",
        "lastHealthUpdate": 1566394698434,
        "version": "2.10.0",
        "healthReport": "Healthy",
        "numContainers": 0,
        "usedMemoryMB": 0,
        "availMemoryMB": 0,
        "usedVirtualCores": 0,
        "availableVirtualCores": 0,
        "numRunningOpportContainers": 0,
        "usedMemoryOpportGB": 0,
        "usedVirtualCoresOpport": 0,
        "numQueuedContainers": 0,
        "resourceUtilization": {
          "nodePhysicalMemoryMB": 0,
          "nodeVirtualMemoryMB": 0,
          "nodeCPUUsage": 0,
          "aggregatedContainersPhysicalMemoryMB": 0,
          "aggregatedContainersVirtualMemoryMB": 0,
          "containersCPUUsage": 0
        }
      },
      {
        "rack": "/default-rack",
        "state": "SHUTDOWN",
        "id": "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net:40887",
        "nodeHostName": "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net",
        "nodeHTTPAddress": "",
        "lastHealthUpdate": 1556395421098,
        "version": "2.10.0",
        "healthReport": "",
        "numContainers": 0,
        "usedMemoryMB": 0,
        "availMemoryMB": 0,
        "usedVirtualCores": 0,
        "availableVirtualCores": 0,
        "numRunningOpportContainers": 0,
        "usedMemoryOpportGB": 0,
        "usedVirtualCoresOpport": 0,
        "numQueuedContainers": 0,
        "resourceUtilization": {
          "nodePhysicalMemoryMB": 717,
          "nodeVirtualMemoryMB": 717,
          "nodeCPUUsage": 0.0033322228118777275,
          "aggregatedContainersPhysicalMemoryMB": 0,
          "aggregatedContainersVirtualMemoryMB": 0,
          "containersCPUUsage": 0
        }
      }
    ]
  }
}
`

func Test_parseYarnResponse(t *testing.T) {
	want := models.YARNInfo{
		LiveNodes: []models.YARNNodeInfo{
			{
				Name:              "rc1b-dataproc-d-w4ly1997k9rgv705.mdb.cloud-preprod.yandex.net",
				State:             "RUNNING",
				NumContainers:     0,
				UsedMemoryMB:      0,
				AvailableMemoryMB: 6144,
				UpdateTime:        1585330403286,
			},
			{
				Name:              "rc1b-dataproc-c-xfrkhe3c2rz2zd4m.mdb.cloud-preprod.yandex.net",
				State:             "RUNNING",
				NumContainers:     1,
				UsedMemoryMB:      0,
				AvailableMemoryMB: 6144,
				UpdateTime:        1585330317308,
			},
			{
				Name:              "rc1b-dataproc-c-2xwiveffsecaca8t.mdb.cloud-preprod.yandex.net",
				State:             "DECOMMISSIONED",
				NumContainers:     0,
				UsedMemoryMB:      0,
				AvailableMemoryMB: 0,
				UpdateTime:        1585320022282,
			},
		},
	}
	got, err := parseYarnResponse([]byte(yarnAPIResponseJSON))
	require.NoError(t, err)
	require.ElementsMatch(t, want.LiveNodes, got.LiveNodes)
}
