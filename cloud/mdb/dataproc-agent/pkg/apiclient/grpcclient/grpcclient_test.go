package grpcclient

import (
	"testing"

	pb "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/dataproc/manager/v1"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/test/requirepb"
)

func Test_buildRequest_Hbase(t *testing.T) {
	info := models.Info{
		HBase: models.HbaseInfo{
			Available:   true,
			Regions:     10,
			Requests:    101,
			AverageLoad: 0.11,
			LiveNodes: []models.HbaseNodeInfo{
				{Name: "node1", Requests: 20, HeapSizeMB: 256, MaxHeapSizeMB: 1024},
				{Name: "node2", Requests: 21, HeapSizeMB: 257, MaxHeapSizeMB: 1023},
				{Name: "node2", Requests: 22, HeapSizeMB: 257, MaxHeapSizeMB: 1022},
			},
			DeadNodes: []models.HbaseNodeInfo{
				{Name: "dnode1", Requests: 20, HeapSizeMB: 256, MaxHeapSizeMB: 1024},
				{Name: "dnode2", Requests: 21, HeapSizeMB: 257, MaxHeapSizeMB: 1023},
			},
		},
	}
	want := &pb.HbaseInfo{
		Available:   true,
		Regions:     10,
		Requests:    101,
		AverageLoad: 0.11,
		LiveNodes: []*pb.HbaseNodeInfo{
			{Name: "node1", Requests: 20, HeapSizeMb: 256, MaxHeapSizeMb: 1024},
			{Name: "node2", Requests: 21, HeapSizeMb: 257, MaxHeapSizeMb: 1023},
			{Name: "node2", Requests: 22, HeapSizeMb: 257, MaxHeapSizeMb: 1022},
		},
		DeadNodes: []*pb.HbaseNodeInfo{
			{Name: "dnode1", Requests: 20, HeapSizeMb: 256, MaxHeapSizeMb: 1024},
			{Name: "dnode2", Requests: 21, HeapSizeMb: 257, MaxHeapSizeMb: 1023},
		},
	}

	got := buildRequest(info)
	requirepb.Equal(t, want, got.Info.Hbase)
}

func Test_buildRequest_Hdfs(t *testing.T) {
	info := models.Info{
		HDFS: models.HDFSInfo{
			Available:             true,
			PercentRemaining:      90.1,
			Used:                  101,
			Free:                  899,
			TotalBlocks:           1203,
			NumberOfMissingBlocks: 2,
			NumberOfMissingBlocksWithReplicationFactorOne: 1,
			LiveNodes: []models.HDFSNodeInfo{
				{Name: "node1", Used: 10, Remaining: 90, Capacity: 100, NumBlocks: 123, State: "RUNNING"},
				{Name: "node2", Used: 11, Remaining: 88, Capacity: 101, NumBlocks: 124, State: "RUNNING"},
			},
			DecommissioningNodes: []models.HDFSNodeInfo{
				{Name: "node3", Used: 13, Remaining: 92, Capacity: 103, NumBlocks: 14, State: "DECOMMISSIONING"},
			},
			DecommissionedNodes: []models.HDFSNodeInfo{
				{Name: "node4", Used: 13, Remaining: 92, Capacity: 103, NumBlocks: 14, State: "DECOMMISSIONED"},
			},
			DeadNodes: []models.HDFSNodeInfo{
				{Name: "node5", Used: 12, Remaining: 91, Capacity: 102, NumBlocks: 13, State: "DEAD"},
			},
		},
	}
	want := &pb.HDFSInfo{
		Available:               true,
		PercentRemaining:        90.1,
		Used:                    101,
		Free:                    899,
		TotalBlocks:             1203,
		MissingBlocks:           2,
		MissingBlocksReplicaOne: 1,
		LiveNodes: []*pb.HDFSNodeInfo{
			{Name: "node1", Used: 10, Remaining: 90, Capacity: 100, NumBlocks: 123, State: "RUNNING"},
			{Name: "node2", Used: 11, Remaining: 88, Capacity: 101, NumBlocks: 124, State: "RUNNING"},
		},
		DecommissioningNodes: []*pb.HDFSNodeInfo{
			{Name: "node3", Used: 13, Remaining: 92, Capacity: 103, NumBlocks: 14, State: "DECOMMISSIONING"},
		},
		DecommissionedNodes: []*pb.HDFSNodeInfo{
			{Name: "node4", Used: 13, Remaining: 92, Capacity: 103, NumBlocks: 14, State: "DECOMMISSIONED"},
		},
		DeadNodes: []*pb.HDFSNodeInfo{
			{Name: "node5", Used: 12, Remaining: 91, Capacity: 102, NumBlocks: 13, State: "DEAD"},
		},
	}

	got := buildRequest(info)
	requirepb.Equal(t, want, got.Info.Hdfs)
}

func Test_buildRequest_Hive(t *testing.T) {
	info := models.Info{
		Hive: models.HiveInfo{
			Available:        true,
			QueriesSucceeded: 101,
			QueriesExecuting: 5,
			QueriesFailed:    4,
			SessionsOpen:     44,
			SessionsActive:   3,
		},
	}
	want := &pb.HiveInfo{
		Available:        true,
		QueriesSucceeded: 101,
		QueriesExecuting: 5,
		QueriesFailed:    4,
		SessionsOpen:     44,
		SessionsActive:   3,
	}

	got := buildRequest(info)
	requirepb.Equal(t, want, got.Info.Hive)
}

func Test_buildRequest_Yarn(t *testing.T) {
	info := models.Info{
		YARN: models.YARNInfo{
			Available: true,
			LiveNodes: []models.YARNNodeInfo{
				{Name: "node1", State: "RUNNING", NumContainers: 1, UsedMemoryMB: 128, AvailableMemoryMB: 1024},
				{Name: "node2", State: "RUNNING", NumContainers: 2, UsedMemoryMB: 512, AvailableMemoryMB: 2048},
			},
		},
	}
	want := &pb.YarnInfo{
		Available: true,
		LiveNodes: []*pb.YarnNodeInfo{
			{Name: "node1", State: "RUNNING", NumContainers: 1, UsedMemoryMb: 128, AvailableMemoryMb: 1024},
			{Name: "node2", State: "RUNNING", NumContainers: 2, UsedMemoryMb: 512, AvailableMemoryMb: 2048},
		},
	}

	got := buildRequest(info)
	requirepb.Equal(t, want, got.Info.Yarn)
}

func Test_buildRequest_Oozie(t *testing.T) {
	info := models.Info{
		Oozie: models.OozieInfo{
			Available: true,
		},
	}
	want := &pb.OozieInfo{
		Alive: true,
	}

	got := buildRequest(info)
	requirepb.Equal(t, want, got.Info.Oozie)
}

func Test_buildRequest_Livy(t *testing.T) {
	info := models.Info{
		Livy: models.LivyInfo{
			Available: true,
		},
	}
	want := &pb.LivyInfo{
		Alive: true,
	}

	got := buildRequest(info)
	requirepb.Equal(t, want, got.Info.Livy)
}
