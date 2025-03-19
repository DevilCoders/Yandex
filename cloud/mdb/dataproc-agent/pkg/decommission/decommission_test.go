package decommission

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

func Test_getYarnNodes(t *testing.T) {
	info := models.Info{
		YARN: models.YARNInfo{
			LiveNodes: []models.YARNNodeInfo{
				{Name: "node1.com", State: "RUNNING"},
				{Name: "node2.com", State: "DECOMMISSIONING"},
				{Name: "node3.com", State: "DECOMMISSIONED"},
				{Name: "node4.com", State: "STOPPED"},
			},
		},
	}
	yarnNodes := getYarnNodesAvailableForDecommission(info)

	correctYarnNodes := make(map[string]struct{})
	correctYarnNodes["node1.com"] = struct{}{}
	correctYarnNodes["node2.com"] = struct{}{}
	require.Equal(t, correctYarnNodes, yarnNodes)
}

func Test_getHdfsNodes(t *testing.T) {
	info := models.Info{
		HDFS: models.HDFSInfo{
			LiveNodes: []models.HDFSNodeInfo{
				{Name: "node1.com", State: "In Service"},
				{Name: "node2.com", State: "Decommission In Progress"},
				{Name: "node3.com", State: "Decommissioned"},
			},
		},
	}
	hdfsNodes := getHdfsNodesAvailableForDecommission(info)
	correctHdfsNodes := make(map[string]struct{})
	correctHdfsNodes["node1.com"] = struct{}{}
	correctHdfsNodes["node2.com"] = struct{}{}
	require.Equal(t, correctHdfsNodes, hdfsNodes)
}
