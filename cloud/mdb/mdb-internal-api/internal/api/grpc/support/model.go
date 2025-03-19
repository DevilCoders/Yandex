package support

import (
	"strings"

	"google.golang.org/protobuf/types/known/timestamppb"

	support "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/support"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/redis"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/support/clmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	servmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
)

var serviceTypeNameMap = map[servmodels.Type]string{
	servmodels.TypeSQLServer:              "SQLSERVER",
	servmodels.TypeClickHouse:             "CLICKHOUSE",
	servmodels.TypeZooKeeper:              "ZOOKEEPER",
	servmodels.TypeGreenplumMasterServer:  "GREENPLUM_MASTER",
	servmodels.TypeGreenplumSegmentServer: "GREENPLUM_SEGMENT",
	servmodels.TypeKafka:                  "KAFKA",
	servmodels.TypeKafkaConnect:           "KAFKA_CONNECT",
	servmodels.TypeWindowsWitnessNode:     "WITNESS",
	servmodels.TypeRedisNonsharded:        "REDIS",
	servmodels.TypeRedisSharded:           "REDIS_CLUSTER",
	servmodels.TypeRedisSentinel:          "SENTINEL",
}

func serviceTypeToString(t servmodels.Type) string {
	stype := "UNKNOWN"
	val, ok := serviceTypeNameMap[t]
	if ok {
		stype = val
	}
	return stype
}

func resolveHostRole(ct clusters.Type, hh hosts.Health, r []hosts.Role) string {
	result := "UNKNOWN"
	switch ct {
	case clusters.TypeClickHouse:
		return clickhouse.HostRoleToGRPC(r[0]).String()
	case clusters.TypeSQLServer:
		return sqlserver.HostRoleToGRPC(hh).String()
	case clusters.TypeGreenplumCluster:
		return greenplum.HostRoleToGRPC(r[0]).String()
	case clusters.TypeElasticSearch:
		return elasticsearch.HostRoleToGRPC(r[0]).String()
	case clusters.TypeKafka:
		role, _ := kafka.HostRolesToGRPC(r)
		return role.String()
	case clusters.TypeRedis:
		return redis.HostRoleToGRPC(hh).String()
	}
	return result
}

func ResultToGRPC(cr clmodels.ClusterResult) *support.Cluster {
	hosts := make([]*support.Host, len(cr.Hosts))
	for ih, h := range cr.Hosts {
		health := cr.Health.Hosts[h.FQDN]
		services := make([]*support.Service, len(health.Services))
		for is, s := range health.Services {
			services[is] = &support.Service{
				Type:   serviceTypeToString(s.Type),
				Role:   s.Role.String(),
				Status: s.Status.String(),
			}
		}
		resources := &support.Resources{
			Preset:   h.ResourcePresetExtID,
			DiskType: h.DiskTypeExtID,
			DiskSize: h.SpaceLimit,
		}
		host := &support.Host{
			Name:       h.FQDN,
			InstanceId: h.VTypeID.String,
			Status:     health.Status.String(),
			Services:   services,
			PublicIp:   h.AssignPublicIP,
			Role:       resolveHostRole(cr.Cluster.Type, cr.Health.Hosts[h.FQDN], h.Roles),
			Resources:  resources,
		}
		hosts[ih] = host
	}
	grpcresult := &support.Cluster{
		Id:          cr.Cluster.ClusterID,
		Type:        cr.Cluster.Type.Stringified(),
		Name:        cr.Cluster.Name,
		FolderId:    cr.FolderCoord.FolderExtID,
		CloudId:     cr.FolderCoord.CloudExtID,
		Status:      cr.Cluster.Status.String(),
		Health:      strings.ToUpper(cr.Health.Cluster.String()),
		CreatedAt:   timestamppb.New(cr.Cluster.CreatedAt),
		Environment: string(cr.Cluster.Environment),
		NetId:       cr.Cluster.NetworkID,
		SgsId:       cr.Cluster.SecurityGroupIDs,
		Hosts:       hosts,
		Version:     cr.Version,
	}
	return grpcresult
}
