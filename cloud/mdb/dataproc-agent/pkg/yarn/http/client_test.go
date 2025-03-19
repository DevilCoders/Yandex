package http

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
)

func TestYarnClientSearchApplicationReturnsApp(t *testing.T) {
	response := `{"apps": {"app": [{"state": "RUNNING"}]}}`
	apps, err := parseAppsResponse([]byte(response))
	require.NoError(t, err)
	require.Equal(t, 1, len(apps))
	require.Equal(t, yarn.StateRunning, apps[0].State)
}

func TestYarnClientSearchApplicationWhenAppNotFound(t *testing.T) {
	response := `{"apps": {"app": []}}`
	app, err := parseAppsResponse([]byte(response))
	require.NoError(t, err)
	require.Nil(t, app)
}

func TestYarnClientSearchApplicationUnexpectedResponseFormat(t *testing.T) {
	response := `{"apps": {"app": {}}}`
	_, err := parseAppsResponse([]byte(response))
	require.Error(t, err, "json: cannot unmarshal")
}

func TestYarnFailedJob(t *testing.T) {
	response := `{
		"apps": {
			"app": [{
					"id": "application_1617981999778_0029",
					"user": "root",
					"name": "word_count_infratest.py",
					"queue": "default",
					"state": "FINISHED",
					"finalStatus": "FAILED",
					"progress": 100.0,
					"trackingUI": "History",
					"trackingUrl": "http://rc1b-dataproc-m-c5umc4crjipexwuq.mdb.cloud-preprod.yandex.net:8088/proxy/application_1617981999778_0029/",
					"diagnostics": "User application exited with status 1",
					"clusterId": 1617981999778,
					"applicationType": "SPARK",
					"applicationTags": "dataproc_job_e4ur5o1in8atgkr6dr1n",
					"priority": 0,
					"startedTime": 1618239984492,
					"launchTime": 1618239984976,
					"finishedTime": 1618240027819,
					"elapsedTime": 43327,
					"amContainerLogs": "http://rc1b-dataproc-d-9nlgfwyvmafme5t7.mdb.cloud-preprod.yandex.net:8042/node/containerlogs/container_1617981999778_0029_02_000001/root",
					"amHostHttpAddress": "rc1b-dataproc-d-9nlgfwyvmafme5t7.mdb.cloud-preprod.yandex.net:8042",
					"amRPCAddress": "rc1b-dataproc-d-9nlgfwyvmafme5t7.mdb.cloud-preprod.yandex.net:39027",
					"allocatedMB": -1,
					"allocatedVCores": -1,
					"reservedMB": -1,
					"reservedVCores": -1,
					"runningContainers": -1,
					"memorySeconds": 267329,
					"vcoreSeconds": 70,
					"queueUsagePercentage": 0.0,
					"clusterUsagePercentage": 0.0,
					"resourceSecondsMap": {
						"entry": {
							"key": "memory-mb",
							"value": "267329"
						},
						"entry": {
							"key": "vcores",
							"value": "70"
						}
					},
					"preemptedResourceMB": 0,
					"preemptedResourceVCores": 0,
					"numNonAMContainerPreempted": 0,
					"numAMContainerPreempted": 0,
					"preemptedMemorySeconds": 0,
					"preemptedVcoreSeconds": 0,
					"preemptedResourceSecondsMap": null,
					"logAggregationStatus": "DISABLED",
					"unmanagedApplication": false,
					"amNodeLabelExpression": "",
					"timeouts": {
						"timeout": [{
								"type": "LIFETIME",
								"expiryTime": "UNLIMITED",
								"remainingTimeInSeconds": -1
							}
						]
					}
				}
			]
		}
	}`
	apps, err := parseAppsResponse([]byte(response))
	require.NoError(t, err)
	require.Equal(t, yarn.FinalStatusFailed, apps[0].FinalStatus)
	require.Equal(t, "application_1617981999778_0029", apps[0].ID)
}

// curl http://$(hostname):8088/ws/v1/cluster/apps/application_1632832880650_0014/appattempts | jq
func TestParseApplicationAttempt(t *testing.T) {
	response := `
	{
	  "appAttempts": {
		"appAttempt": [
		  {
			"id": 1,
			"startTime": 1632922850369,
			"finishedTime": 1632922874933,
			"containerId": "container_1632832880650_0014_01_000001",
			"nodeHttpAddress": "rc1c-dataproc-d-g4snfldenu0la5xe.mdb.cloud-preprod.yandex.net:8042",
			"nodeId": "rc1c-dataproc-d-g4snfldenu0la5xe.mdb.cloud-preprod.yandex.net:44749",
			"logsLink": "http://rc1c-dataproc-d-g4snfldenu0la5xe.mdb.cloud-preprod.yandex.net:8042/node/containerlogs/container_1632832880650_0014_01_000001/dataproc-agent",
			"blacklistedNodes": "",
			"nodesBlacklistedBySystem": "",
			"appAttemptId": "appattempt_1632832880650_0014_000001"
		  }
		]
	  }
	}
	`
	applicationAttempts, err := parseApplicationAttemptResponse([]byte(response))
	require.NoError(t, err)
	require.Equal(t, "container_1632832880650_0014_01_000001", applicationAttempts[0].AMContainerID)
	require.Equal(t, "appattempt_1632832880650_0014_000001", applicationAttempts[0].AppAttemptID)
}
