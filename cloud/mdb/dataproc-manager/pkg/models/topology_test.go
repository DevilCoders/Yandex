package models

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

func TestClusterTopology_ServiceNodes(t *testing.T) {
	topology := ClusterTopology{
		Cid:      "cid1",
		Revision: 1,
		FolderID: "folder1",
		Services: []service.Service{service.Hdfs, service.Yarn, service.Hbase},
		Subclusters: []SubclusterTopology{
			{
				Subcid:   "main",
				Role:     role.Main,
				Services: []service.Service{service.Hdfs, service.Yarn, service.Hbase},
				Hosts:    []string{"hdp-m0.internal"},
			},
			{
				Subcid:   "data",
				Role:     role.Data,
				Services: []service.Service{service.Hdfs, service.Yarn, service.Hbase},
				Hosts:    []string{"hdp-d0.internal", "hdp-d1.internal"},
			},
			{
				Subcid:   "data2",
				Role:     role.Data,
				Services: []service.Service{service.Hdfs, service.Yarn},
				Hosts:    []string{"hdp-d2.internal"},
			},
			{
				Subcid:              "compute",
				Role:                role.Compute,
				Services:            []service.Service{service.Yarn},
				Hosts:               []string{},
				InstanceGroupID:     "instance-group-id1",
				DecommissionTimeout: 60,
			},
		},
	}

	want := []string{}
	require.ElementsMatch(t, topology.ServiceNodes(service.Spark), want)

	want = []string{"hdp-m0.internal", "hdp-d0.internal", "hdp-d1.internal"}
	require.ElementsMatch(t, topology.ServiceNodes(service.Hbase), want)

	want = []string{"hdp-m0.internal", "hdp-d0.internal", "hdp-d1.internal", "hdp-d2.internal"}
	require.ElementsMatch(t, topology.ServiceNodes(service.Hdfs), want)

	want = []string{"instance-group-id1"}
	require.Equal(t, topology.GetInstanceGroupIDs(), want)

	wantSubcluster := &topology.Subclusters[3]
	actual := topology.GetSubclusterByInstanceGroupID("instance-group-id1")
	require.Equal(t, actual, wantSubcluster)
}
