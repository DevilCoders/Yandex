package agent

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

// curl "http://$(hostname):9870/jmx?qry=Hadoop:service=NameNode,name=NameNodeInfo" | jq
const jmxHdfs = `{
  "beans": [
    {
      "name": "Hadoop:service=NameNode,name=NameNodeInfo",
      "modelerType": "org.apache.hadoop.hdfs.server.namenode.FSNamesystem",
      "Total": 15790075904,
      "ClusterId": "CID-050e5ee0-b119-422d-9451-840d93ece2c0",
      "UpgradeFinalized": true,
      "Version": "2.10.0, r447d5c8b300ad5e17b52568c4d6030477f0c52ff",
      "Used": 86016,
      "Free": 4714311680,
      "Safemode": "",
      "NonDfsUsedSpace": 10391302144,
      "PercentUsed": 0.0005447472,
      "BlockPoolUsedSpace": 86016,
      "PercentBlockPoolUsed": 0.0005447472,
      "PercentRemaining": 29.856169,
      "CacheCapacity": 0,
      "CacheUsed": 0,
      "TotalBlocks": 6,
      "TotalFiles": 1034,
      "NumberOfMissingBlocks": 0,
      "NumberOfMissingBlocksWithReplicationFactorOne": 0,
      "LiveNodes": "{\"rc1b-dataproc-d-gk2y0rv2th3dd8e9.mdb.cloud-preprod.yandex.net:9866\":{\"infoAddr\":\"192.168.1.50:9864\",\"infoSecureAddr\":\"192.168.1.50:0\",\"xferaddr\":\"192.168.1.50:9866\",\"lastContact\":2,\"usedSpace\":86016,\"adminState\":\"In Service\",\"nonDfsUsedSpace\":10391302144,\"capacity\":15790075904,\"numBlocks\":6,\"version\":\"2.10.0\",\"used\":86016,\"remaining\":4714311680,\"blockScheduled\":0,\"blockPoolUsed\":86016,\"blockPoolUsedPercent\":5.447472E-4,\"volfails\":0,\"lastBlockReport\":98},\"rc1b-dataproc-d-zr1ofngkg9w7hi48.mdb.cloud-preprod.yandex.net:9866\":{\"infoAddr\":\"192.168.1.22:9864\",\"infoSecureAddr\":\"192.168.1.22:0\",\"xferaddr\":\"192.168.1.22:9866\",\"lastContact\":1,\"usedSpace\":24576,\"adminState\":\"Decommissioned\",\"nonDfsUsedSpace\":10389950464,\"capacity\":15790075904,\"numBlocks\":0,\"version\":\"2.10.0\",\"used\":24576,\"remaining\":4715724800,\"blockScheduled\":0,\"blockPoolUsed\":24576,\"blockPoolUsedPercent\":1.5564206E-4,\"volfails\":0,\"lastBlockReport\":6},\"rc1b-dataproc-d-tga77smp5jkhp16p.mdb.cloud-preprod.yandex.net:9866\":{\"infoAddr\":\"192.168.1.36:9864\",\"infoSecureAddr\":\"192.168.1.36:0\",\"xferaddr\":\"192.168.1.36:9866\",\"lastContact\":1,\"usedSpace\":24576,\"adminState\":\"Decommission In Progress\",\"nonDfsUsedSpace\":10389950464,\"capacity\":15790075904,\"numBlocks\":0,\"version\":\"2.10.0\",\"used\":24576,\"remaining\":4715724800,\"blockScheduled\":0,\"blockPoolUsed\":24576,\"blockPoolUsedPercent\":1.5564206E-4,\"volfails\":0,\"lastBlockReport\":6}}",
      "DeadNodes": "{\"rc1b-dataproc-d-aj489jc0ajce90cs.mdb.cloud-preprod.yandex.net:9866\":{\"lastContact\":647,\"decommissioned\":false,\"adminState\":\"In Service\",\"xferaddr\":\"192.168.1.50:9866\"}}",
      "DecomNodes": "{}",
      "EnteringMaintenanceNodes": "{}",
      "BlockPoolId": "BP-1177500400-192.168.1.11-1586205534856",
      "NameDirStatuses": "{\"active\":{\"/hadoop/dfs/name\":\"IMAGE_AND_EDITS\"},\"failed\":{}}",
      "NodeUsage": "{\"nodeUsage\":{\"min\":\"0.00%\",\"median\":\"0.00%\",\"max\":\"0.00%\",\"stdDev\":\"0.00%\"}}",
      "NameJournalStatus": "[{\"manager\":\"FileJournalManager(root=/hadoop/dfs/name)\",\"stream\":\"EditLogFileOutputStream(/hadoop/dfs/name/current/edits_inprogress_0000000000000001081)\",\"disabled\":\"false\",\"required\":\"false\"}]",
      "JournalTransactionInfo": "{\"MostRecentCheckpointTxId\":\"1080\",\"LastAppliedOrWrittenTxId\":\"1081\"}",
      "NNStarted": "Mon Apr 06 21:25:10 UTC 2020",
      "NNStartedTimeInMillis": 1586208310786,
      "CompileInfo": "2020-03-03T16:32Z by dataproc from (HEAD detached at 447d5c8)",
      "CorruptFiles": "[]",
      "NumberOfSnapshottableDirs": 0,
      "DistinctVersionCount": 1,
      "DistinctVersions": [
        {
          "key": "2.10.0",
          "value": 3
        }
      ],
      "SoftwareVersion": "2.10.0",
      "NameDirSize": "{\"/hadoop/dfs/name\":3148258}",
      "RollingUpgradeStatus": null,
      "Threads": 93
    }
  ]
}`

func Test_parseHdfsResponse(t *testing.T) {
	want := models.HDFSInfo{
		Available:             true,
		PercentRemaining:      29.856169,
		Used:                  86016,
		Free:                  4714311680,
		TotalBlocks:           6,
		NumberOfMissingBlocks: 0,
		NumberOfMissingBlocksWithReplicationFactorOne: 0,
		LiveNodes: []models.HDFSNodeInfo{
			{Name: "rc1b-dataproc-d-gk2y0rv2th3dd8e9.mdb.cloud-preprod.yandex.net", Used: 86016, Remaining: 4714311680, Capacity: 15790075904, NumBlocks: 6, State: "In Service"},
		},
		DecommissioningNodes: []models.HDFSNodeInfo{
			{Name: "rc1b-dataproc-d-tga77smp5jkhp16p.mdb.cloud-preprod.yandex.net", Used: 24576, Remaining: 4715724800, Capacity: 15790075904, NumBlocks: 0, State: "Decommission In Progress"},
		},
		DecommissionedNodes: []models.HDFSNodeInfo{
			{Name: "rc1b-dataproc-d-zr1ofngkg9w7hi48.mdb.cloud-preprod.yandex.net", Used: 24576, Remaining: 4715724800, Capacity: 15790075904, NumBlocks: 0, State: "Decommissioned"},
		},
		DeadNodes: []models.HDFSNodeInfo{
			{Name: "rc1b-dataproc-d-aj489jc0ajce90cs.mdb.cloud-preprod.yandex.net", Used: 0, Remaining: 0, Capacity: 0, NumBlocks: 0, State: "In Service"},
		},
	}
	got, err := parseHdfsResponse([]byte(jmxHdfs))
	require.NoError(t, err)

	// Order of elements flaping
	require.ElementsMatch(t, want.LiveNodes, got.LiveNodes)
	want.LiveNodes = []models.HDFSNodeInfo{}
	got.LiveNodes = []models.HDFSNodeInfo{}

	require.ElementsMatch(t, want.DecommissioningNodes, got.DecommissioningNodes)
	want.DecommissioningNodes = []models.HDFSNodeInfo{}
	got.DecommissioningNodes = []models.HDFSNodeInfo{}

	require.ElementsMatch(t, want.DecommissionedNodes, got.DecommissionedNodes)
	want.DecommissionedNodes = []models.HDFSNodeInfo{}
	got.DecommissionedNodes = []models.HDFSNodeInfo{}

	require.ElementsMatch(t, want.DeadNodes, got.DeadNodes)
	want.DeadNodes = []models.HDFSNodeInfo{}
	got.DeadNodes = []models.HDFSNodeInfo{}

	require.Equal(t, want, got)
}
