package server

import (
	"testing"

	"github.com/stretchr/testify/require"

	pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

func Test_deduceClusterHealth_HbaseInfo(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hbase: &pb.HbaseInfo{
				Available:   true,
				Regions:     150,
				AverageLoad: 11.1,
				LiveNodes: []*pb.HbaseNodeInfo{
					{Name: "node1", Requests: 12, HeapSizeMb: 256, MaxHeapSizeMb: 1024},
					{Name: "node2", Requests: 13, HeapSizeMb: 257, MaxHeapSizeMb: 1025},
				},
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hbase},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Data,
				Services: []service.Service{service.Hbase},
				Hosts:    []string{"node1", "node2"},
			},
		},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Hbase: models.ServiceHbase{
				BasicHealthService: models.BasicHealthService{Health: health.Alive},
				Regions:            150,
				Requests:           0,
				AverageLoad:        11.1,
			},
		},
	}
	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hbase: models.ServiceHbaseNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					Requests:           12,
					HeapSizeMb:         256,
					MaxHeapSizeMb:      1024,
				},
			},
		},
		"node2": {
			Fqdn:   "node2",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hbase: models.ServiceHbaseNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					Requests:           13,
					HeapSizeMb:         257,
					MaxHeapSizeMb:      1025,
				},
			},
		},
	}
	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_HbaseHealth(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hbase: &pb.HbaseInfo{
				Available: true,
				LiveNodes: []*pb.HbaseNodeInfo{
					{Name: "node1"},
					{Name: "node2"},
				},
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hbase},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Data,
				Services: []service.Service{service.Hbase},
				Hosts:    []string{"node1", "node2", "node3"},
			},
		},
	}

	gotCluster, _ := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, health.Degraded, gotCluster.ServiceHealth(service.Hbase))

	topology.Subclusters[0].Hosts = []string{"node1", "node2"}
	gotCluster, _ = deduceClusterHealth(&report, topology, 0)
	require.Equal(t, health.Alive, gotCluster.ServiceHealth(service.Hbase))

	topology.Subclusters[0].Hosts = []string{"node1", "node2", "node3"}
	gotCluster, _ = deduceClusterHealth(&report, topology, 0)
	require.Equal(t, health.Degraded, gotCluster.ServiceHealth(service.Hbase))

	report.Info.Hbase.LiveNodes = []*pb.HbaseNodeInfo{}
	gotCluster, _ = deduceClusterHealth(&report, topology, 0)
	require.Equal(t, health.Dead, gotCluster.ServiceHealth(service.Hbase))
}

func Test_deduceClusterHealth_HdfsInfo(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hdfs: &pb.HDFSInfo{
				Available:               true,
				PercentRemaining:        11.3,
				Used:                    12,
				Free:                    101,
				TotalBlocks:             13,
				MissingBlocks:           1,
				MissingBlocksReplicaOne: 2,
				LiveNodes: []*pb.HDFSNodeInfo{
					{Name: "node1", Used: 13, Remaining: 102, Capacity: 115, NumBlocks: 201, State: "In Service"},
					{Name: "node2", Used: 14, Remaining: 103, Capacity: 116, NumBlocks: 202, State: "In Service"},
				},
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hdfs},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Data,
				Services: []service.Service{service.Hdfs},
				Hosts:    []string{"node1", "node2"},
			},
		},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Hdfs: models.ServiceHdfs{
				BasicHealthService:      models.BasicHealthService{Health: health.Alive},
				PercentRemaining:        11.3,
				Used:                    12,
				Free:                    101,
				TotalBlocks:             13,
				MissingBlocks:           1,
				MissingBlocksReplicaOne: 2,
			},
		},
	}
	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hdfs: models.ServiceHdfsNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					Used:               13,
					Remaining:          102,
					Capacity:           115,
					NumBlocks:          201,
					State:              "In Service",
				},
			},
		},
		"node2": {
			Fqdn:   "node2",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hdfs: models.ServiceHdfsNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					Used:               14,
					Remaining:          103,
					Capacity:           116,
					NumBlocks:          202,
					State:              "In Service",
				},
			},
		},
	}
	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_YarnInfo(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Yarn: &pb.YarnInfo{
				Available: true,
				LiveNodes: []*pb.YarnNodeInfo{
					{Name: "node1", NumContainers: 102, UsedMemoryMb: 115, AvailableMemoryMb: 201, State: "RUNNING"},
					{Name: "node2", NumContainers: 103, UsedMemoryMb: 116, AvailableMemoryMb: 202, State: "RUNNING"},
				},
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Yarn},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Data,
				Services: []service.Service{service.Yarn},
				Hosts:    []string{"node1", "node2"},
			},
		},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Yarn: models.BasicHealthService{Health: health.Alive},
		},
	}
	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Yarn: models.ServiceYarnNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					NumContainers:      102,
					UsedMemoryMb:       115,
					AvailableMemoryMb:  201,
					State:              "RUNNING",
				},
			},
		},
		"node2": {
			Fqdn:   "node2",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Yarn: models.ServiceYarnNode{
					BasicHealthService: models.BasicHealthService{Health: health.Alive},
					NumContainers:      103,
					UsedMemoryMb:       116,
					AvailableMemoryMb:  202,
					State:              "RUNNING",
				},
			},
		},
	}
	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_HiveInfo(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hive: &pb.HiveInfo{
				Available:        true,
				QueriesSucceeded: 101,
				QueriesFailed:    11,
				QueriesExecuting: 2,
				SessionsOpen:     54,
				SessionsActive:   3,
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hive},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Hive: models.ServiceHive{
				BasicHealthService: models.BasicHealthService{Health: health.Alive},
				QueriesSucceeded:   101,
				QueriesFailed:      11,
				QueriesExecuting:   2,
				SessionsOpen:       54,
				SessionsActive:     3,
			},
		},
	}
	wantHosts := map[string]models.HostHealth{}

	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_HiveHealth(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hive: &pb.HiveInfo{
				Available: false,
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hive},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Dead,
		Services: map[service.Service]models.ServiceHealth{
			service.Hive: models.ServiceHive{
				BasicHealthService: models.BasicHealthService{
					Health:      health.Dead,
					Explanation: "service is not available",
				},
			},
		},
		Explanation: "all services are Dead: Hive is Dead (service is not available)",
	}
	wantHosts := map[string]models.HostHealth{}

	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_HiveHostHealth(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hive: &pb.HiveInfo{
				Available: true,
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hive},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Hive},
				Hosts:    []string{"node1"},
			},
		},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Hive: models.ServiceHive{
				BasicHealthService: models.BasicHealthService{Health: health.Alive},
			},
		},
	}

	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hive: models.BasicHealthService{Health: health.Alive},
			},
		},
	}

	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_OozieHostHealth(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Oozie: &pb.OozieInfo{
				Alive: true,
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Oozie},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Oozie},
				Hosts:    []string{"node1"},
			},
		},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Oozie: models.BasicHealthService{Health: health.Alive},
		},
	}

	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Oozie: models.BasicHealthService{Health: health.Alive},
			},
		},
	}

	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_ZookeeperInfo(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Zookeeper: &pb.ZookeeperInfo{
				Alive: true,
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Zookeeper},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Zookeeper},
				Hosts:    []string{"node1"},
			},
		},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Zookeeper: models.BasicHealthService{Health: health.Alive},
		},
	}
	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Zookeeper: models.BasicHealthService{Health: health.Alive},
			},
		},
	}

	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)

	report.Info.Zookeeper.Alive = false
	wantCluster.Health = health.Dead
	wantCluster.Services[service.Zookeeper] = models.BasicHealthService{
		Health:      health.Dead,
		Explanation: "service is not available",
	}
	wantCluster.Explanation = "all services are Dead: Zookeeper is Dead (service is not available)"
	node := wantHosts["node1"]
	node.Health = health.Dead
	node.Services[service.Zookeeper] = models.BasicHealthService{Health: health.Dead}
	wantHosts["node1"] = node

	gotCluster, gotHosts = deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_EmptyInput(t *testing.T) {
	report := pb.ReportRequest{}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hbase, service.Hdfs, service.Yarn, service.Hive, service.Flume, service.Zookeeper},
	}
	deduceClusterHealth(&report, topology, 0)
}

func Test_deduceClusterHealth_MasterDead(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hbase: nil},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hbase},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Hbase},
				Hosts:    []string{"node1"},
			},
		},
	}

	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Dead,
			Services: map[service.Service]models.ServiceHealth{
				service.Hbase: models.BasicHealthService{Health: health.Dead},
			},
		},
	}
	_, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_MasterAlive(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hbase: &pb.HbaseInfo{
				Available: true,
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Hbase},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Hbase},
				Hosts:    []string{"node1"},
			},
		},
	}

	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Hbase: models.BasicHealthService{Health: health.Alive},
			},
		},
	}
	_, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_LivyHostHealth(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Livy: &pb.LivyInfo{
				Alive: true,
			}},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Livy},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Livy},
				Hosts:    []string{"node1"},
			},
		},
	}

	wantCluster := models.ClusterHealth{
		Health: health.Alive,
		Services: map[service.Service]models.ServiceHealth{
			service.Livy: models.BasicHealthService{Health: health.Alive},
		},
	}

	wantHosts := map[string]models.HostHealth{
		"node1": {
			Fqdn:   "node1",
			Health: health.Alive,
			Services: map[service.Service]models.ServiceHealth{
				service.Livy: models.BasicHealthService{Health: health.Alive},
			},
		},
	}

	gotCluster, gotHosts := deduceClusterHealth(&report, topology, 0)
	require.Equal(t, wantCluster, gotCluster)
	require.Equal(t, wantHosts, gotHosts)
}

func Test_deduceClusterHealth_UpdateTimeWithYarn(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Yarn: &pb.YarnInfo{
				Available: true,
				LiveNodes: []*pb.YarnNodeInfo{
					{Name: "node1", State: "RUNNING", UpdateTime: 101},
					{Name: "node2", State: "RUNNING", UpdateTime: 102},
				},
			},
			Hive: &pb.HiveInfo{
				Available: true,
			},
		},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Yarn},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Yarn, service.Hive},
				Hosts:    []string{"node1"},
			},
			{
				Role:     role.Data,
				Services: []service.Service{service.Yarn},
				Hosts:    []string{"node2"},
			},
		},
	}

	gotCluster, _ := deduceClusterHealth(&report, topology, 200)
	require.Equal(t, int64(101), gotCluster.UpdateTime)
}

func Test_deduceClusterHealth_UpdateTimeWithoutYarn(t *testing.T) {
	report := pb.ReportRequest{
		Info: &pb.Info{
			Hive: &pb.HiveInfo{
				Available: true,
			},
			Hdfs: &pb.HDFSInfo{
				Available: true,
				LiveNodes: []*pb.HDFSNodeInfo{
					{Name: "node2", State: "In Service"},
					{Name: "node3", State: "In Service"},
				},
			},
		},
	}
	topology := models.ClusterTopology{
		Services: []service.Service{service.Yarn},
		Subclusters: []models.SubclusterTopology{
			{
				Role:     role.Main,
				Services: []service.Service{service.Hive},
				Hosts:    []string{"node1"},
			},
			{
				Role:     role.Data,
				Services: []service.Service{service.Hdfs},
				Hosts:    []string{"node2", "node3"},
			},
		},
	}

	gotCluster, _ := deduceClusterHealth(&report, topology, 200)
	require.Equal(t, int64(200), gotCluster.UpdateTime)
}
