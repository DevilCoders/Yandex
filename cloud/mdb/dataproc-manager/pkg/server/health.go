package server

import (
	"fmt"

	pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

func getHbaseNodes(report *pb.ReportRequest) map[string]models.ServiceHealth {
	gotNodes := make(map[string]models.ServiceHealth, len(report.Info.Hbase.LiveNodes))
	for _, node := range report.Info.Hbase.LiveNodes {
		nodeInfo := models.ServiceHbaseNode{
			Requests:      node.Requests,
			HeapSizeMb:    node.HeapSizeMb,
			MaxHeapSizeMb: node.MaxHeapSizeMb,
		}
		nodeInfo.Health = health.Alive
		gotNodes[node.Name] = nodeInfo
	}
	return gotNodes
}

func getHdfsNodes(report *pb.ReportRequest) map[string]models.ServiceHealth {
	nodes := append(report.Info.Hdfs.LiveNodes, report.Info.Hdfs.DecommissioningNodes...)
	nodes = append(nodes, report.Info.Hdfs.DecommissionedNodes...)
	gotNodes := make(map[string]models.ServiceHealth, len(nodes))

	for _, node := range nodes {
		nodeInfo := models.ServiceHdfsNode{
			Used:      node.Used,
			Remaining: node.Remaining,
			Capacity:  node.Capacity,
			NumBlocks: node.NumBlocks,
			State:     node.State,
		}
		switch node.State {
		case "In Service":
			nodeInfo.Health = health.Alive
		case "Decommission In Progress":
			nodeInfo.Health = health.Decommissioning
		case "Decommissioned":
			nodeInfo.Health = health.Decommissioned
		default:
			nodeInfo.Health = health.Unknown
		}
		gotNodes[node.Name] = nodeInfo
	}
	return gotNodes
}

func getYarnNodes(report *pb.ReportRequest) map[string]models.ServiceHealth {
	gotNodes := make(map[string]models.ServiceHealth, len(report.Info.Yarn.LiveNodes))
	for _, node := range report.Info.Yarn.LiveNodes {
		nodeInfo := models.ServiceYarnNode{
			State:             node.State,
			NumContainers:     node.NumContainers,
			UsedMemoryMb:      node.UsedMemoryMb,
			AvailableMemoryMb: node.AvailableMemoryMb,
		}
		switch node.State {
		case "RUNNING":
			nodeInfo.Health = health.Alive
		case "DECOMMISSIONING":
			nodeInfo.Health = health.Decommissioning
		case "DECOMMISSIONED":
			nodeInfo.Health = health.Decommissioned
		default:
			nodeInfo.Health = health.Unknown
		}
		gotNodes[node.Name] = nodeInfo
	}
	return gotNodes
}

func getInfoForService(report *pb.ReportRequest, serviceType service.Service) (map[string]models.ServiceHealth, bool) {
	switch serviceType {
	case service.Hbase:
		if report.Info == nil || report.Info.Hbase == nil {
			return map[string]models.ServiceHealth{}, false
		}
		return getHbaseNodes(report), report.Info.Hbase.Available
	case service.Hdfs:
		if report.Info == nil || report.Info.Hdfs == nil {
			return map[string]models.ServiceHealth{}, false
		}
		return getHdfsNodes(report), report.Info.Hdfs.Available
	case service.Yarn:
		if report.Info == nil || report.Info.Yarn == nil {
			return map[string]models.ServiceHealth{}, false
		}
		return getYarnNodes(report), report.Info.Yarn.Available
	case service.Hive:
		if report.Info == nil || report.Info.Hive == nil {
			return map[string]models.ServiceHealth{}, false
		}
		return make(map[string]models.ServiceHealth), report.Info.Hive.Available
	case service.Zookeeper:
		if report.Info == nil || report.Info.Zookeeper == nil {
			return map[string]models.ServiceHealth{}, false
		}
		return make(map[string]models.ServiceHealth), report.Info.Zookeeper.Alive
	case service.Oozie:
		if report.Info == nil || report.Info.Oozie == nil {
			return map[string]models.ServiceHealth{}, false
		}
		return make(map[string]models.ServiceHealth), report.Info.Oozie.Alive
	case service.Livy:
		if report.Info == nil || report.Info.Livy == nil {
			return map[string]models.ServiceHealth{}, false
		}
		return make(map[string]models.ServiceHealth), report.Info.Livy.Alive
	default:
		return make(map[string]models.ServiceHealth), false
	}
}

func listsIntersection(firstSlice, secondSlice []string) map[string]struct{} {
	firstSliceMap := make(map[string]struct{})
	for _, first := range firstSlice {
		firstSliceMap[first] = struct{}{}
	}

	result := make(map[string]struct{})
	for _, second := range secondSlice {
		if _, exists := firstSliceMap[second]; exists {
			result[second] = struct{}{}
		}
	}
	return result
}

func removeElements(slice []string, denylist map[string]struct{}) []string {
	result := make([]string, 0, len(slice))
	for _, element := range slice {
		if _, exists := denylist[element]; !exists {
			result = append(result, element)
		}
	}
	return result
}

func getRunningNodeFQDNs(input map[string]models.ServiceHealth) []string {
	result := make([]string, 0, len(input))
	for key, nodeInfo := range input {
		if nodeInfo.GetHealth() == health.Alive {
			result = append(result, key)
		}
	}
	return result
}

func deduceServiceHealth(wantNodes []string, gotNodes []string, available bool) (health.Health, string) {
	if !available {
		return health.Dead, "service is not available"
	}
	intersection := listsIntersection(wantNodes, gotNodes)
	if len(wantNodes) == 0 {
		return health.Alive, ""
	}
	if len(intersection) == 0 {
		return health.Dead, "service is not running on target nodes of the cluster"
	}
	if len(intersection) != len(wantNodes) || len(intersection) != len(gotNodes) {
		return health.Degraded, fmt.Sprintf("service is not alive on some of the target nodes of the cluster, expected nodes: %s, got nodes: %s", wantNodes, gotNodes)
	}
	return health.Alive, ""
}

func getInstanceGroupRunningNodes(gotNodes map[string]models.ServiceHealth, subclusters []models.SubclusterTopology) (map[string]struct{}, int) {
	wantInstanceGroupNodesNumber := 0
	for _, subcluster := range subclusters {
		if subcluster.InstanceGroupID != "" {
			wantInstanceGroupNodesNumber += int(subcluster.MinHostsCount)
		}
	}

	instanceGroupRunningNodes := make(map[string]struct{}, len(gotNodes))
	for fqdn, nodeInfo := range gotNodes {
		deducedRole, _ := role.DeduceRoleByNodeFQDN(fqdn)
		if deducedRole == role.ComputeAutoScaling && nodeInfo.GetHealth() == health.Alive {
			instanceGroupRunningNodes[fqdn] = struct{}{}
		}
	}
	return instanceGroupRunningNodes, wantInstanceGroupNodesNumber
}

func min(a, b int64) int64 {
	if a < b {
		return a
	}
	return b
}

func deduceClusterHealth(report *pb.ReportRequest, topology models.ClusterTopology, currentTime int64) (models.ClusterHealth, map[string]models.HostHealth) {
	cluster := models.NewClusterHealth(topology.Cid)
	hosts := make(map[string]models.HostHealth)
	var instanceGroupRunningNodes map[string]struct{}
	var wantInstanceGroupNodesNumber int
	for _, subcluster := range topology.Subclusters {
		for _, fqdn := range subcluster.Hosts {
			hosts[fqdn] = models.NewHostWithServices(fqdn, subcluster.Services)
		}
		if subcluster.InstanceGroupID != "" {
			gotNodes, _ := getInfoForService(report, service.Yarn)
			instanceGroupRunningNodes, wantInstanceGroupNodesNumber = getInstanceGroupRunningNodes(
				gotNodes,
				topology.Subclusters,
			)
			for fqdn := range instanceGroupRunningNodes {
				hosts[fqdn] = models.NewHostWithServices(fqdn, subcluster.Services)
			}
		}
	}

	// For UpdateTime motivation see https://st.yandex-team.ru/MDB-12290
	timeDrift := currentTime - report.GetCollectedAt().GetSeconds()
	if report.CollectedAt == nil {
		// suppose that old clients (those that do not send CollectedAt) do not have time drift
		timeDrift = 0
	}
	updateTime := currentTime

	for _, serviceType := range topology.Services {
		// for all services expect for yarn assume that service health update time is `now`
		// because it is collected directly before sending report
		serviceUpdateTime := currentTime
		wantNodes := topology.ServiceNodes(serviceType)
		gotNodes, available := getInfoForService(report, serviceType)
		// If we got health info from masternode assuming it's alive
		if available {
			for _, fqdn := range topology.MasterNodes() {
				if host, ok := hosts[fqdn]; ok {
					host.Services[serviceType] = models.BasicHealthService{Health: health.Alive}
					gotNodes[fqdn] = host.Services[serviceType]
				}
			}
		}
		runningNodeFQDNs := getRunningNodeFQDNs(gotNodes)
		switch serviceType {
		case service.Hbase:
			var serviceStruct models.ServiceHbase
			if report.Info != nil && report.Info.Hbase != nil {
				serviceStruct = models.ServiceHbase{
					Regions:     report.Info.Hbase.Regions,
					Requests:    report.Info.Hbase.Requests,
					AverageLoad: report.Info.Hbase.AverageLoad,
				}
			} else {
				serviceStruct = models.ServiceHbase{}
			}
			serviceStruct.Health, serviceStruct.Explanation = deduceServiceHealth(
				wantNodes,
				runningNodeFQDNs,
				available,
			)
			cluster.Services[serviceType] = serviceStruct
		case service.Hdfs:
			var serviceStruct models.ServiceHdfs
			if report.Info != nil && report.Info.Hdfs != nil {
				serviceStruct = models.ServiceHdfs{
					PercentRemaining:        report.Info.Hdfs.PercentRemaining,
					Used:                    report.Info.Hdfs.Used,
					Free:                    report.Info.Hdfs.Free,
					TotalBlocks:             report.Info.Hdfs.TotalBlocks,
					MissingBlocks:           report.Info.Hdfs.MissingBlocks,
					MissingBlocksReplicaOne: report.Info.Hdfs.MissingBlocksReplicaOne,
					Safemode:                report.Info.Hdfs.Safemode != "",
				}
			} else {
				serviceStruct = models.ServiceHdfs{}
			}
			serviceStruct.Health, serviceStruct.Explanation = deduceServiceHealth(
				wantNodes,
				runningNodeFQDNs,
				available,
			)
			cluster.HdfsInSafemode = serviceStruct.Safemode
			cluster.Services[serviceType] = serviceStruct
		case service.Hive:
			var serviceStruct models.ServiceHive
			if report.Info != nil && report.Info.Hive != nil {
				serviceStruct = models.ServiceHive{
					QueriesSucceeded: report.Info.Hive.QueriesSucceeded,
					QueriesFailed:    report.Info.Hive.QueriesFailed,
					QueriesExecuting: report.Info.Hive.QueriesExecuting,
					SessionsOpen:     report.Info.Hive.SessionsOpen,
					SessionsActive:   report.Info.Hive.SessionsActive,
				}
			} else {
				serviceStruct = models.ServiceHive{}
			}
			if available {
				serviceStruct.Health = health.Alive
			} else {
				serviceStruct.Health = health.Dead
				serviceStruct.Explanation = "service is not available"
			}
			cluster.Services[serviceType] = serviceStruct
		case service.Yarn:
			runningNodeFQDNs := removeElements(runningNodeFQDNs, instanceGroupRunningNodes)
			serviceHealth, explanation := deduceServiceHealth(wantNodes, runningNodeFQDNs, available)
			cluster.Services[serviceType] = models.BasicHealthService{
				Health:      serviceHealth,
				Explanation: explanation,
			}
			if serviceHealth == health.Alive && len(instanceGroupRunningNodes) < wantInstanceGroupNodesNumber {
				cluster.Services[serviceType] = models.BasicHealthService{
					Health:      health.Degraded,
					Explanation: "service is not alive on some of the target instance group nodes of the cluster",
				}
			}

			// Compute yarn service update time as minimum among UpdateTime-s of yarn nodes.
			// It is required because current report may be outdated (some yarn nodes may already be down).
			wantNodesMap := make(map[string]struct{}, len(wantNodes))
			for _, fqdn := range wantNodes {
				wantNodesMap[fqdn] = struct{}{}
			}
			for _, node := range report.GetInfo().GetYarn().GetLiveNodes() {
				// Take only those yarn nodes into account, that are expected to exist according to topology
				// and instance group content. This is required so that deleted nodes with stale UpdateTime
				// within yarn do not spoil serviceUpdateTime metric.
				_, expectedNonIG := wantNodesMap[node.Name]
				_, expectedIG := instanceGroupRunningNodes[node.Name]
				if expectedNonIG || expectedIG {
					serviceUpdateTime = min(serviceUpdateTime, node.UpdateTime+timeDrift)
				}
			}
		default:
			serviceHealth, explanation := deduceServiceHealth(wantNodes, runningNodeFQDNs, available)
			cluster.Services[serviceType] = models.BasicHealthService{
				Health:      serviceHealth,
				Explanation: explanation,
			}
		}
		for fqdn, nodeInfo := range gotNodes {
			if host, ok := hosts[fqdn]; ok {
				host.Services[serviceType] = nodeInfo
				hosts[fqdn] = host
			}
		}

		updateTime = min(updateTime, serviceUpdateTime)
	}

	cluster.Health, cluster.Explanation = cluster.DeduceHealth()
	for fqdn, host := range hosts {
		host.Health = host.DeduceHealth()
		hosts[fqdn] = host
	}

	cluster.UpdateTime = updateTime

	return cluster, hosts
}
