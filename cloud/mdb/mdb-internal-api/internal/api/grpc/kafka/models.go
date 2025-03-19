package kafka

import (
	"context"
	"fmt"
	"sort"
	"strings"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/wrappers"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	kfv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/validation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/system"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

var (
	mapAccessRoleToGRPC = map[kfmodels.AccessRoleType]kfv1.Permission_AccessRole{
		kfmodels.AccessRoleUnspecified: kfv1.Permission_ACCESS_ROLE_UNSPECIFIED,
		kfmodels.AccessRoleProducer:    kfv1.Permission_ACCESS_ROLE_PRODUCER,
		kfmodels.AccessRoleConsumer:    kfv1.Permission_ACCESS_ROLE_CONSUMER,
		kfmodels.AccessRoleAdmin:       kfv1.Permission_ACCESS_ROLE_ADMIN,
	}
	mapAccessRoleFromGRPC = reflectutil.ReverseMap(mapAccessRoleToGRPC).(map[kfv1.Permission_AccessRole]kfmodels.AccessRoleType)
)

func AccessRoleToGRPC(ar kfmodels.AccessRoleType) kfv1.Permission_AccessRole {
	v, ok := mapAccessRoleToGRPC[ar]
	if !ok {
		return kfv1.Permission_ACCESS_ROLE_UNSPECIFIED
	}

	return v
}

func AccessRoleFromGRPC(ar kfv1.Permission_AccessRole) kfmodels.AccessRoleType {
	v, ok := mapAccessRoleFromGRPC[ar]
	if !ok {
		// TODO: make invalid or throw an error?
		return kfmodels.AccessRoleUnspecified
	}

	return v
}

var (
	mapStatusToGRCP = map[clusters.Status]kfv1.Cluster_Status{
		clusters.StatusCreating:             kfv1.Cluster_CREATING,
		clusters.StatusCreateError:          kfv1.Cluster_ERROR,
		clusters.StatusRunning:              kfv1.Cluster_RUNNING,
		clusters.StatusModifying:            kfv1.Cluster_UPDATING,
		clusters.StatusModifyError:          kfv1.Cluster_ERROR,
		clusters.StatusStopping:             kfv1.Cluster_STOPPING,
		clusters.StatusStopped:              kfv1.Cluster_STOPPED,
		clusters.StatusStopError:            kfv1.Cluster_ERROR,
		clusters.StatusStarting:             kfv1.Cluster_STARTING,
		clusters.StatusStartError:           kfv1.Cluster_ERROR,
		clusters.StatusMaintainingOffline:   kfv1.Cluster_UPDATING,
		clusters.StatusMaintainOfflineError: kfv1.Cluster_ERROR,
	}
)

func StatusToGRPC(status clusters.Status) kfv1.Cluster_Status {
	v, ok := mapStatusToGRCP[status]
	if !ok {
		return kfv1.Cluster_STATUS_UNKNOWN
	}
	return v
}

func ClusterToGRPC(cluster kfmodels.MDBCluster, saltEnvMapper grpcapi.SaltEnvMapper) *kfv1.Cluster {
	v := &kfv1.Cluster{
		Id:                 cluster.ClusterID,
		FolderId:           cluster.FolderExtID,
		CreatedAt:          grpcapi.TimeToGRPC(cluster.CreatedAt),
		Name:               cluster.Name,
		Description:        cluster.Description,
		Labels:             cluster.Labels,
		Environment:        kfv1.Cluster_Environment(saltEnvMapper.ToGRPC(cluster.Environment)),
		NetworkId:          cluster.NetworkID,
		SecurityGroupIds:   cluster.SecurityGroupIDs,
		MaintenanceWindow:  MaintenanceWindowToGRPC(cluster.MaintenanceInfo.Window),
		PlannedOperation:   MaintenanceOperationToGRPC(cluster.MaintenanceInfo.Operation),
		HostGroupIds:       cluster.HostGroupIDs,
		Status:             StatusToGRPC(cluster.Status),
		Health:             ClusterHealthToGRPC(cluster.Health),
		DeletionProtection: cluster.DeletionProtection,
	}
	return v
}

var (
	mapClusterHealthToGRPC = map[clusters.Health]kfv1.Cluster_Health{
		clusters.HealthAlive:    kfv1.Cluster_ALIVE,
		clusters.HealthDegraded: kfv1.Cluster_DEGRADED,
		clusters.HealthDead:     kfv1.Cluster_DEAD,
		clusters.HealthUnknown:  kfv1.Cluster_HEALTH_UNKNOWN,
	}
)

func ClusterHealthToGRPC(env clusters.Health) kfv1.Cluster_Health {
	v, ok := mapClusterHealthToGRPC[env]
	if !ok {
		return kfv1.Cluster_HEALTH_UNKNOWN
	}
	return v
}

var (
	mapConnectorStatusToGRCP = map[kfmodels.ConnectorStatus]kfv1.Connector_Status{
		kfmodels.ConnectorStatusUnspecified: kfv1.Connector_STATUS_UNKNOWN,
		kfmodels.ConnectorStatusRunning:     kfv1.Connector_RUNNING,
		kfmodels.ConnectorStatusPaused:      kfv1.Connector_PAUSED,
		kfmodels.ConnectorStatusError:       kfv1.Connector_ERROR,
	}
)

func ConnectorStatusToGRPC(status kfmodels.ConnectorStatus) kfv1.Connector_Status {
	v, ok := mapConnectorStatusToGRCP[status]
	if !ok {
		return kfv1.Connector_STATUS_UNKNOWN
	}
	return v
}

var (
	mapConnectorHealthToGRCP = map[kfmodels.ConnectorHealth]kfv1.Connector_Health{
		kfmodels.ConnectorHealthUnknown: kfv1.Connector_HEALTH_UNKNOWN,
		kfmodels.ConnectorHealthAlive:   kfv1.Connector_ALIVE,
		kfmodels.ConnectorHealthDead:    kfv1.Connector_DEAD,
	}
)

func ConnectorHealthToGRPC(health kfmodels.ConnectorHealth) kfv1.Connector_Health {
	v, ok := mapConnectorHealthToGRCP[health]
	if !ok {
		return kfv1.Connector_HEALTH_UNKNOWN
	}
	return v
}

func ResourcesToGRPC(resources models.ClusterResources) *kfv1.Resources {
	return &kfv1.Resources{
		ResourcePresetId: resources.ResourcePresetExtID,
		DiskTypeId:       resources.DiskTypeExtID,
		DiskSize:         resources.DiskSize,
	}
}

func AccessFromGRPC(a *kfv1.Access) *clusters.Access {
	return &clusters.Access{
		DataTransfer: optional.NewBool(a.GetDataTransfer()),
		Serverless:   optional.NewBool(a.GetServerless()),
		WebSQL:       optional.NewBool(a.GetWebSql()),
	}

}

func AccessToGRPC(a kfmodels.Access) *kfv1.Access {
	if a.DataTransfer.Valid || a.WebSQL.Valid || a.Serverless.Valid {
		return &kfv1.Access{
			DataTransfer: a.DataTransfer.Valid && a.DataTransfer.Bool,
			WebSql:       a.WebSQL.Valid && a.WebSQL.Bool,
			Serverless:   a.Serverless.Valid && a.Serverless.Bool,
		}
	}
	return nil
}

func ClusterWithPillarToGRPC(cluster kfmodels.MDBCluster, saltEnvMapper grpcapi.SaltEnvMapper) *kfv1.Cluster {
	v := ClusterToGRPC(cluster, saltEnvMapper)
	v.Config = &kfv1.ConfigSpec{
		Kafka: &kfv1.ConfigSpec_Kafka{
			Resources: ResourcesToGRPC(cluster.Config.Kafka.Resources),
		},
		AssignPublicIp:  cluster.Config.AssignPublicIP,
		BrokersCount:    &wrappers.Int64Value{Value: cluster.Config.BrokersCount},
		ZoneId:          cluster.Config.ZoneID,
		Version:         cluster.Config.Version,
		UnmanagedTopics: cluster.Config.UnmanagedTopics,
		SchemaRegistry:  cluster.Config.SchemaRegistry,
		SyncTopics:      cluster.Config.SyncTopics,
		Access:          AccessToGRPC(cluster.Config.Access),
	}
	if (cluster.Config.ZooKeeper.Resources != models.ClusterResources{}) {
		v.Config.Zookeeper = &kfv1.ConfigSpec_Zookeeper{
			Resources: ResourcesToGRPC(cluster.Config.ZooKeeper.Resources),
		}
	}
	KafkaConfigToGRPC(cluster.Config, v.Config.Kafka)
	v.Monitoring = make([]*kfv1.Monitoring, 0, len(cluster.Monitoring.Charts))
	for _, chart := range cluster.Monitoring.Charts {
		mon := &kfv1.Monitoring{
			Name:        chart.Name,
			Description: chart.Description,
			Link:        chart.Link,
		}
		v.Monitoring = append(v.Monitoring, mon)
	}
	return v
}

func ClustersToGRPC(clusters []kfmodels.MDBCluster, saltEnvMapper grpcapi.SaltEnvMapper) []*kfv1.Cluster {
	var v []*kfv1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster, saltEnvMapper))
	}
	return v
}

func UserPermissionFromGRPC(perm *kfv1.Permission) kfmodels.Permission {
	return kfmodels.Permission{
		TopicName:  perm.GetTopicName(),
		AccessRole: AccessRoleFromGRPC(perm.GetRole()),
		Group:      perm.GetGroup(),
		Host:       perm.GetHost(),
	}
}

func UserPermissionsFromGRPC(permissions []*kfv1.Permission) []kfmodels.Permission {
	res := make([]kfmodels.Permission, 0, len(permissions))
	for _, perm := range permissions {
		res = append(res, UserPermissionFromGRPC(perm))
	}
	return res
}

func UserSpecFromGRPC(spec *kfv1.UserSpec) kfmodels.UserSpec {
	return kfmodels.UserSpec{
		Name:        spec.Name,
		Password:    secret.NewString(spec.GetPassword()),
		Permissions: UserPermissionsFromGRPC(spec.GetPermissions()),
	}
}

func UserSpecsFromGRPC(specs []*kfv1.UserSpec) []kfmodels.UserSpec {
	var v []kfmodels.UserSpec
	for _, spec := range specs {
		if spec != nil {
			v = append(v, UserSpecFromGRPC(spec))
		}
	}
	return v
}

func UserToGRPC(user kfmodels.User) *kfv1.User {
	v := &kfv1.User{Name: user.Name, ClusterId: user.ClusterID}
	for _, perm := range user.Permissions {
		v.Permissions = append(v.Permissions, &kfv1.Permission{
			TopicName: perm.TopicName,
			Role:      AccessRoleToGRPC(perm.AccessRole),
			Group:     perm.Group,
			Host:      perm.Host,
		})
	}
	return v
}

func UsersToGRPC(users []kfmodels.User) []*kfv1.User {
	var v []*kfv1.User
	for _, user := range users {
		v = append(v, UserToGRPC(user))
	}
	return v
}

var (
	mapCleanupPolicy2_1FromGRPC = map[kfv1.TopicConfig2_1_CleanupPolicy]string{
		kfv1.TopicConfig2_1_CLEANUP_POLICY_UNSPECIFIED:        kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig2_1_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig2_1_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig2_1_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy2_1ToGRPC = reflectutil.ReverseMap(mapCleanupPolicy2_1FromGRPC).(map[string]kfv1.TopicConfig2_1_CleanupPolicy)

	mapCleanupPolicy2_6FromGRPC = map[kfv1.TopicConfig2_6_CleanupPolicy]string{
		kfv1.TopicConfig2_6_CLEANUP_POLICY_UNSPECIFIED:        kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig2_6_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig2_6_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig2_6_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy2_6ToGRPC = reflectutil.ReverseMap(mapCleanupPolicy2_6FromGRPC).(map[string]kfv1.TopicConfig2_6_CleanupPolicy)

	mapCleanupPolicy2_8FromGRPC = map[kfv1.TopicConfig2_8_CleanupPolicy]string{
		kfv1.TopicConfig2_8_CLEANUP_POLICY_UNSPECIFIED:        kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig2_8_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig2_8_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig2_8_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy2_8ToGRPC = reflectutil.ReverseMap(mapCleanupPolicy2_8FromGRPC).(map[string]kfv1.TopicConfig2_8_CleanupPolicy)

	mapCleanupPolicy3_0FromGRPC = map[kfv1.TopicConfig3_0_CleanupPolicy]string{
		kfv1.TopicConfig3_0_CLEANUP_POLICY_UNSPECIFIED:        kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig3_0_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig3_0_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig3_0_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy3_0ToGRPC = reflectutil.ReverseMap(mapCleanupPolicy3_0FromGRPC).(map[string]kfv1.TopicConfig3_0_CleanupPolicy)

	mapCleanupPolicy3_1FromGRPC = map[kfv1.TopicConfig3_1_CleanupPolicy]string{
		kfv1.TopicConfig3_1_CLEANUP_POLICY_UNSPECIFIED:        kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig3_1_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig3_1_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig3_1_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy3_1ToGRPC = reflectutil.ReverseMap(mapCleanupPolicy3_1FromGRPC).(map[string]kfv1.TopicConfig3_1_CleanupPolicy)

	mapCleanupPolicy3FromGRPC = map[kfv1.TopicConfig3_CleanupPolicy]string{
		kfv1.TopicConfig3_CLEANUP_POLICY_UNSPECIFIED:        kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig3_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig3_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig3_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy3ToGRPC = reflectutil.ReverseMap(mapCleanupPolicy3FromGRPC).(map[string]kfv1.TopicConfig3_CleanupPolicy)
)

func CleanupPolicyFromGRPC(cp interface{}) string {
	switch v := cp.(type) {
	case kfv1.TopicConfig2_6_CleanupPolicy:
		ret, ok := mapCleanupPolicy2_6FromGRPC[v]
		if !ok {
			return ""
		}
		return ret
	case kfv1.TopicConfig2_1_CleanupPolicy:
		ret, ok := mapCleanupPolicy2_1FromGRPC[v]
		if !ok {
			return ""
		}
		return ret
	case kfv1.TopicConfig2_8_CleanupPolicy:
		ret, ok := mapCleanupPolicy2_8FromGRPC[v]
		if !ok {
			return ""
		}
		return ret
	case kfv1.TopicConfig3_0_CleanupPolicy:
		ret, ok := mapCleanupPolicy3_0FromGRPC[v]
		if !ok {
			return ""
		}
		return ret
	case kfv1.TopicConfig3_1_CleanupPolicy:
		ret, ok := mapCleanupPolicy3_1FromGRPC[v]
		if !ok {
			return ""
		}
		return ret
	case kfv1.TopicConfig3_CleanupPolicy:
		ret, ok := mapCleanupPolicy3FromGRPC[v]
		if !ok {
			return ""
		}
		return ret
	}
	return ""
}

func CleanupPolicy2_1ToGRPC(cp string) kfv1.TopicConfig2_1_CleanupPolicy {
	v, ok := mapCleanupPolicy2_1ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig2_1_CLEANUP_POLICY_UNSPECIFIED
	}
	return v
}

func CleanupPolicy2_6ToGRPC(cp string) kfv1.TopicConfig2_6_CleanupPolicy {
	v, ok := mapCleanupPolicy2_6ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig2_6_CLEANUP_POLICY_UNSPECIFIED
	}
	return v
}

func CleanupPolicy2_8ToGRPC(cp string) kfv1.TopicConfig2_8_CleanupPolicy {
	v, ok := mapCleanupPolicy2_8ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig2_8_CLEANUP_POLICY_UNSPECIFIED
	}
	return v
}

func CleanupPolicy3_0ToGRPC(cp string) kfv1.TopicConfig3_0_CleanupPolicy {
	v, ok := mapCleanupPolicy3_0ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig3_0_CLEANUP_POLICY_UNSPECIFIED
	}
	return v
}

func CleanupPolicy3_1ToGRPC(cp string) kfv1.TopicConfig3_1_CleanupPolicy {
	v, ok := mapCleanupPolicy3_1ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig3_1_CLEANUP_POLICY_UNSPECIFIED
	}
	return v
}

func CleanupPolicy3ToGRPC(cp string) kfv1.TopicConfig3_CleanupPolicy {
	v, ok := mapCleanupPolicy3ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig3_CLEANUP_POLICY_UNSPECIFIED
	}
	return v
}

var (
	mapCompressionTypeFromGRPC = map[kfv1.CompressionType]string{
		kfv1.CompressionType_COMPRESSION_TYPE_UNSPECIFIED:  kfmodels.CompressionTypeUnspecified,
		kfv1.CompressionType_COMPRESSION_TYPE_UNCOMPRESSED: kfmodels.CompressionTypeUncompressed,
		kfv1.CompressionType_COMPRESSION_TYPE_ZSTD:         kfmodels.CompressionTypeZstd,
		kfv1.CompressionType_COMPRESSION_TYPE_LZ4:          kfmodels.CompressionTypeLz4,
		kfv1.CompressionType_COMPRESSION_TYPE_SNAPPY:       kfmodels.CompressionTypeSnappy,
		kfv1.CompressionType_COMPRESSION_TYPE_GZIP:         kfmodels.CompressionTypeGzip,
		kfv1.CompressionType_COMPRESSION_TYPE_PRODUCER:     kfmodels.CompressionTypeProducer,
	}
	mapCompressionTypeToGRPC = reflectutil.ReverseMap(mapCompressionTypeFromGRPC).(map[string]kfv1.CompressionType)
)

func CompressionTypeFromGRPC(cp kfv1.CompressionType) string {
	v, ok := mapCompressionTypeFromGRPC[cp]
	if !ok {
		return ""
	}
	return v
}

func CompressionTypeToGRPC(cp string) kfv1.CompressionType {
	v, ok := mapCompressionTypeToGRPC[cp]
	if !ok {
		return kfv1.CompressionType_COMPRESSION_TYPE_UNSPECIFIED
	}
	return v
}

func TopicConfigFromGRPC(spec *kfv1.TopicSpec) kfmodels.TopicConfig {
	if spec.GetTopicConfig() == nil {
		return kfmodels.TopicConfig{}
	}

	var config GRPCTopicConfig
	cleanupPolicy := ""

	version := ""
	switch topicConfig := spec.GetTopicConfig().(type) {
	case *kfv1.TopicSpec_TopicConfig_2_1:
		version = kfmodels.Version2_1
		config = topicConfig.TopicConfig_2_1
		cleanupPolicy = CleanupPolicyFromGRPC(topicConfig.TopicConfig_2_1.GetCleanupPolicy())
	case *kfv1.TopicSpec_TopicConfig_2_6:
		version = kfmodels.Version2_6
		config = topicConfig.TopicConfig_2_6
		cleanupPolicy = CleanupPolicyFromGRPC(topicConfig.TopicConfig_2_6.GetCleanupPolicy())
	case *kfv1.TopicSpec_TopicConfig_2_8:
		version = kfmodels.Version2_8
		config = topicConfig.TopicConfig_2_8
		cleanupPolicy = CleanupPolicyFromGRPC(topicConfig.TopicConfig_2_8.GetCleanupPolicy())
	case *kfv1.TopicSpec_TopicConfig_3_0:
		version = kfmodels.Version3_0
		config = topicConfig.TopicConfig_3_0
		cleanupPolicy = CleanupPolicyFromGRPC(topicConfig.TopicConfig_3_0.GetCleanupPolicy())
	case *kfv1.TopicSpec_TopicConfig_3_1:
		version = kfmodels.Version3_1
		config = topicConfig.TopicConfig_3_1
		cleanupPolicy = CleanupPolicyFromGRPC(topicConfig.TopicConfig_3_1.GetCleanupPolicy())
	case *kfv1.TopicSpec_TopicConfig_3:
		version = kfmodels.СonfigVersion3
		config = topicConfig.TopicConfig_3
		cleanupPolicy = CleanupPolicyFromGRPC(topicConfig.TopicConfig_3.GetCleanupPolicy())
	}
	if config == nil {
		panic(fmt.Sprintf("unexpected TopicConfig type: %+v", spec.GetTopicConfig()))
	}

	return kfmodels.TopicConfig{
		Version:            version,
		CleanupPolicy:      cleanupPolicy,
		CompressionType:    CompressionTypeFromGRPC(config.GetCompressionType()),
		DeleteRetentionMs:  api.UnwrapInt64Value(config.GetDeleteRetentionMs()),
		FileDeleteDelayMs:  api.UnwrapInt64Value(config.GetFileDeleteDelayMs()),
		FlushMessages:      api.UnwrapInt64Value(config.GetFlushMessages()),
		FlushMs:            api.UnwrapInt64Value(config.GetFlushMs()),
		MinCompactionLagMs: api.UnwrapInt64Value(config.GetMinCompactionLagMs()),
		RetentionBytes:     api.UnwrapInt64Value(config.GetRetentionBytes()),
		RetentionMs:        api.UnwrapInt64Value(config.GetRetentionMs()),
		MaxMessageBytes:    api.UnwrapInt64Value(config.GetMaxMessageBytes()),
		MinInsyncReplicas:  api.UnwrapInt64Value(config.GetMinInsyncReplicas()),
		SegmentBytes:       api.UnwrapInt64Value(config.GetSegmentBytes()),
		Preallocate:        api.UnwrapBoolValue(config.GetPreallocate()),
	}
}

func TopicConfigToGRPC(tp kfmodels.Topic, topic *kfv1.Topic) {
	config := tp.Config
	switch config.Version {
	case kfmodels.Version2_1:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_2_1{
			TopicConfig_2_1: &kfv1.TopicConfig2_1{
				CleanupPolicy:      CleanupPolicy2_1ToGRPC(config.CleanupPolicy),
				CompressionType:    CompressionTypeToGRPC(config.CompressionType),
				DeleteRetentionMs:  api.WrapInt64Pointer(config.DeleteRetentionMs),
				FileDeleteDelayMs:  api.WrapInt64Pointer(config.FileDeleteDelayMs),
				FlushMessages:      api.WrapInt64Pointer(config.FlushMessages),
				FlushMs:            api.WrapInt64Pointer(config.FlushMs),
				MinCompactionLagMs: api.WrapInt64Pointer(config.MinCompactionLagMs),
				RetentionBytes:     api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:        api.WrapInt64Pointer(config.RetentionMs),
				MaxMessageBytes:    api.WrapInt64Pointer(config.MaxMessageBytes),
				MinInsyncReplicas:  api.WrapInt64Pointer(config.MinInsyncReplicas),
				SegmentBytes:       api.WrapInt64Pointer(config.SegmentBytes),
				Preallocate:        api.WrapBoolPointer(config.Preallocate),
			},
		}
	case kfmodels.Version2_6:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_2_6{
			TopicConfig_2_6: &kfv1.TopicConfig2_6{
				CleanupPolicy:      CleanupPolicy2_6ToGRPC(config.CleanupPolicy),
				CompressionType:    CompressionTypeToGRPC(config.CompressionType),
				DeleteRetentionMs:  api.WrapInt64Pointer(config.DeleteRetentionMs),
				FileDeleteDelayMs:  api.WrapInt64Pointer(config.FileDeleteDelayMs),
				FlushMessages:      api.WrapInt64Pointer(config.FlushMessages),
				FlushMs:            api.WrapInt64Pointer(config.FlushMs),
				MinCompactionLagMs: api.WrapInt64Pointer(config.MinCompactionLagMs),
				RetentionBytes:     api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:        api.WrapInt64Pointer(config.RetentionMs),
				MaxMessageBytes:    api.WrapInt64Pointer(config.MaxMessageBytes),
				MinInsyncReplicas:  api.WrapInt64Pointer(config.MinInsyncReplicas),
				SegmentBytes:       api.WrapInt64Pointer(config.SegmentBytes),
				Preallocate:        api.WrapBoolPointer(config.Preallocate),
			},
		}
	case kfmodels.Version2_8:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_2_8{
			TopicConfig_2_8: &kfv1.TopicConfig2_8{
				CleanupPolicy:      CleanupPolicy2_8ToGRPC(config.CleanupPolicy),
				CompressionType:    CompressionTypeToGRPC(config.CompressionType),
				DeleteRetentionMs:  api.WrapInt64Pointer(config.DeleteRetentionMs),
				FileDeleteDelayMs:  api.WrapInt64Pointer(config.FileDeleteDelayMs),
				FlushMessages:      api.WrapInt64Pointer(config.FlushMessages),
				FlushMs:            api.WrapInt64Pointer(config.FlushMs),
				MinCompactionLagMs: api.WrapInt64Pointer(config.MinCompactionLagMs),
				RetentionBytes:     api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:        api.WrapInt64Pointer(config.RetentionMs),
				MaxMessageBytes:    api.WrapInt64Pointer(config.MaxMessageBytes),
				MinInsyncReplicas:  api.WrapInt64Pointer(config.MinInsyncReplicas),
				SegmentBytes:       api.WrapInt64Pointer(config.SegmentBytes),
				Preallocate:        api.WrapBoolPointer(config.Preallocate),
			},
		}
	case kfmodels.Version3_0:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_3_0{
			TopicConfig_3_0: &kfv1.TopicConfig3_0{
				CleanupPolicy:      CleanupPolicy3_0ToGRPC(config.CleanupPolicy),
				CompressionType:    CompressionTypeToGRPC(config.CompressionType),
				DeleteRetentionMs:  api.WrapInt64Pointer(config.DeleteRetentionMs),
				FileDeleteDelayMs:  api.WrapInt64Pointer(config.FileDeleteDelayMs),
				FlushMessages:      api.WrapInt64Pointer(config.FlushMessages),
				FlushMs:            api.WrapInt64Pointer(config.FlushMs),
				MinCompactionLagMs: api.WrapInt64Pointer(config.MinCompactionLagMs),
				RetentionBytes:     api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:        api.WrapInt64Pointer(config.RetentionMs),
				MaxMessageBytes:    api.WrapInt64Pointer(config.MaxMessageBytes),
				MinInsyncReplicas:  api.WrapInt64Pointer(config.MinInsyncReplicas),
				SegmentBytes:       api.WrapInt64Pointer(config.SegmentBytes),
				Preallocate:        api.WrapBoolPointer(config.Preallocate),
			},
		}
	case kfmodels.Version3_1:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_3_1{
			TopicConfig_3_1: &kfv1.TopicConfig3_1{
				CleanupPolicy:      CleanupPolicy3_1ToGRPC(config.CleanupPolicy),
				CompressionType:    CompressionTypeToGRPC(config.CompressionType),
				DeleteRetentionMs:  api.WrapInt64Pointer(config.DeleteRetentionMs),
				FileDeleteDelayMs:  api.WrapInt64Pointer(config.FileDeleteDelayMs),
				FlushMessages:      api.WrapInt64Pointer(config.FlushMessages),
				FlushMs:            api.WrapInt64Pointer(config.FlushMs),
				MinCompactionLagMs: api.WrapInt64Pointer(config.MinCompactionLagMs),
				RetentionBytes:     api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:        api.WrapInt64Pointer(config.RetentionMs),
				MaxMessageBytes:    api.WrapInt64Pointer(config.MaxMessageBytes),
				MinInsyncReplicas:  api.WrapInt64Pointer(config.MinInsyncReplicas),
				SegmentBytes:       api.WrapInt64Pointer(config.SegmentBytes),
				Preallocate:        api.WrapBoolPointer(config.Preallocate),
			},
		}
	case kfmodels.СonfigVersion3:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_3{
			TopicConfig_3: &kfv1.TopicConfig3{
				CleanupPolicy:      CleanupPolicy3ToGRPC(config.CleanupPolicy),
				CompressionType:    CompressionTypeToGRPC(config.CompressionType),
				DeleteRetentionMs:  api.WrapInt64Pointer(config.DeleteRetentionMs),
				FileDeleteDelayMs:  api.WrapInt64Pointer(config.FileDeleteDelayMs),
				FlushMessages:      api.WrapInt64Pointer(config.FlushMessages),
				FlushMs:            api.WrapInt64Pointer(config.FlushMs),
				MinCompactionLagMs: api.WrapInt64Pointer(config.MinCompactionLagMs),
				RetentionBytes:     api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:        api.WrapInt64Pointer(config.RetentionMs),
				MaxMessageBytes:    api.WrapInt64Pointer(config.MaxMessageBytes),
				MinInsyncReplicas:  api.WrapInt64Pointer(config.MinInsyncReplicas),
				SegmentBytes:       api.WrapInt64Pointer(config.SegmentBytes),
				Preallocate:        api.WrapBoolPointer(config.Preallocate),
			},
		}
	default:
		panic(fmt.Sprintf("do not know how to convert TopicConfig of kafka v%s to grpc format", config.Version))
	}
}

func TopicSpecFromGRPC(spec *kfv1.TopicSpec) kfmodels.TopicSpec {
	ret := kfmodels.TopicSpec{
		Name:              spec.GetName(),
		Partitions:        spec.GetPartitions().GetValue(),
		ReplicationFactor: spec.GetReplicationFactor().GetValue(),
		Config:            TopicConfigFromGRPC(spec),
	}
	return ret
}

func TopicSpecsFromGRPC(specs []*kfv1.TopicSpec) []kfmodels.TopicSpec {
	var v []kfmodels.TopicSpec
	for _, spec := range specs {
		if spec != nil {
			v = append(v, TopicSpecFromGRPC(spec))
		}
	}
	return v
}

func TopicToGRPC(tp kfmodels.Topic) *kfv1.Topic {
	topic := &kfv1.Topic{
		Name:              tp.Name,
		ClusterId:         tp.ClusterID,
		Partitions:        &wrappers.Int64Value{Value: tp.Partitions},
		ReplicationFactor: &wrappers.Int64Value{Value: tp.ReplicationFactor},
	}
	TopicConfigToGRPC(tp, topic)

	return topic
}

func TopicsToGRPC(tps []kfmodels.Topic) []*kfv1.Topic {
	var v []*kfv1.Topic
	for _, tp := range tps {
		v = append(v, TopicToGRPC(tp))
	}
	return v
}

var hostRoleToGRPC = map[hosts.Role]kfv1.Host_Role{
	hosts.RoleKafka:     kfv1.Host_KAFKA,
	hosts.RoleZooKeeper: kfv1.Host_ZOOKEEPER,
}

func HostRoleToGRPC(hostRole hosts.Role) (kfv1.Host_Role, error) {
	role, ok := hostRoleToGRPC[hostRole]
	if !ok {
		return kfv1.Host_ROLE_UNSPECIFIED, xerrors.Errorf("unknown host role: %s", hostRole)
	}
	return role, nil
}

func HostRolesToGRPC(hostRoles []hosts.Role) (kfv1.Host_Role, error) {
	if len(hostRoles) < 1 {
		return kfv1.Host_ROLE_UNSPECIFIED, xerrors.Errorf("host role is unspecified")
	}
	return HostRoleToGRPC(hostRoles[0])
}

func HostToGRPC(host hosts.HostExtended) (*kfv1.Host, error) {
	hostRole, err := HostRolesToGRPC(host.Roles)
	if err != nil {
		return nil, err
	}
	h := &kfv1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		ZoneId:    host.ZoneID,
		Role:      hostRole,
		Resources: &kfv1.Resources{
			ResourcePresetId: host.ResourcePresetExtID,
			DiskSize:         host.SpaceLimit,
			DiskTypeId:       host.DiskTypeExtID,
		},
		Health:         HostHealthToGRPC(host.Health),
		System:         HostSystemMetricsToGRPC(host.Health.System),
		SubnetId:       host.SubnetID,
		AssignPublicIp: host.AssignPublicIP,
	}
	return h, nil
}

func HostsToGRPC(hosts []hosts.HostExtended) ([]*kfv1.Host, error) {
	v := make([]*kfv1.Host, 0, len(hosts))
	for _, host := range hosts {
		h, err := HostToGRPC(host)
		if err != nil {
			return nil, err
		}
		v = append(v, h)
	}
	return v, nil
}

var (
	mapHostHealthToGRPC = map[hosts.Status]kfv1.Host_Health{
		hosts.StatusAlive:    kfv1.Host_ALIVE,
		hosts.StatusDegraded: kfv1.Host_DEGRADED,
		hosts.StatusDead:     kfv1.Host_DEAD,
		hosts.StatusUnknown:  kfv1.Host_UNKNOWN,
	}
)

func HostHealthToGRPC(hh hosts.Health) kfv1.Host_Health {
	v, ok := mapHostHealthToGRPC[hh.Status]
	if !ok {
		return kfv1.Host_UNKNOWN
	}
	return v
}

func cpuMetricToGRPC(cpu *system.CPUMetric) *kfv1.Host_CPUMetric {
	if cpu == nil {
		return nil
	}

	return &kfv1.Host_CPUMetric{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToGRPC(memory *system.MemoryMetric) *kfv1.Host_MemoryMetric {
	if memory == nil {
		return nil
	}

	return &kfv1.Host_MemoryMetric{
		Timestamp: memory.Timestamp,
		Used:      memory.Used,
		Total:     memory.Total,
	}
}

func diskMetricToGRPC(disk *system.DiskMetric) *kfv1.Host_DiskMetric {
	if disk == nil {
		return nil
	}

	return &kfv1.Host_DiskMetric{
		Timestamp: disk.Timestamp,
		Used:      disk.Used,
		Total:     disk.Total,
	}
}

func HostSystemMetricsToGRPC(sm *system.Metrics) *kfv1.Host_SystemMetrics {
	if sm == nil {
		return nil
	}

	return &kfv1.Host_SystemMetrics{
		Cpu:    cpuMetricToGRPC(sm.CPU),
		Memory: memoryMetricToGRPC(sm.Memory),
		Disk:   diskMetricToGRPC(sm.Disk),
	}
}

type GRPCKafkaConfig interface {
	GetCompressionType() kfv1.CompressionType
	GetLogFlushIntervalMessages() *wrappers.Int64Value
	GetLogFlushIntervalMs() *wrappers.Int64Value
	GetLogFlushSchedulerIntervalMs() *wrappers.Int64Value
	GetLogRetentionBytes() *wrappers.Int64Value
	GetLogRetentionHours() *wrappers.Int64Value
	GetLogRetentionMinutes() *wrappers.Int64Value
	GetLogRetentionMs() *wrappers.Int64Value
	GetLogSegmentBytes() *wrappers.Int64Value
	GetLogPreallocate() *wrappers.BoolValue
	GetSocketSendBufferBytes() *wrappers.Int64Value
	GetSocketReceiveBufferBytes() *wrappers.Int64Value
	GetAutoCreateTopicsEnable() *wrappers.BoolValue
	GetNumPartitions() *wrappers.Int64Value
	GetDefaultReplicationFactor() *wrappers.Int64Value
	GetMessageMaxBytes() *wrappers.Int64Value
	GetReplicaFetchMaxBytes() *wrappers.Int64Value
	GetSslCipherSuites() []string
	GetOffsetsRetentionMinutes() *wrappers.Int64Value
}

func SslCipherSuitesFromGRPC(suites []string) []string {
	var result []string = nil
	if len(suites) > 0 {
		suites = slices.Dedup[string](suites)
		sort.Strings(suites)
		result = suites
	}
	return result
}

func KafkaConfigFromGRPC(spec *kfv1.ConfigSpec_Kafka) (kfmodels.KafkaConfig, string) {
	if spec.GetKafkaConfig() == nil {
		return kfmodels.KafkaConfig{}, ""
	}

	version := ""
	var config GRPCKafkaConfig
	// TODO refactor change literal versions to constants
	switch kafkaConfig := spec.GetKafkaConfig().(type) {
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_2_1:
		config = kafkaConfig.KafkaConfig_2_1
		version = "2.1"
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_2_6:
		config = kafkaConfig.KafkaConfig_2_6
		version = "2.6"
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_2_8:
		config = kafkaConfig.KafkaConfig_2_8
		version = "2.8"
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_3_0:
		config = kafkaConfig.KafkaConfig_3_0
		version = "3.0"
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_3_1:
		config = kafkaConfig.KafkaConfig_3_1
		version = "3.1"
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_3:
		config = kafkaConfig.KafkaConfig_3
		version = kfmodels.СonfigVersion3
	}
	if config == nil {
		panic(fmt.Sprintf("unexpected KafkaConfig type: %+v", spec.GetKafkaConfig()))
	}

	return kfmodels.KafkaConfig{
		CompressionType:             CompressionTypeFromGRPC(config.GetCompressionType()),
		LogFlushIntervalMessages:    api.UnwrapInt64Value(config.GetLogFlushIntervalMessages()),
		LogFlushIntervalMs:          api.UnwrapInt64Value(config.GetLogFlushIntervalMs()),
		LogFlushSchedulerIntervalMs: api.UnwrapInt64Value(config.GetLogFlushSchedulerIntervalMs()),
		LogRetentionBytes:           api.UnwrapInt64Value(config.GetLogRetentionBytes()),
		LogRetentionHours:           api.UnwrapInt64Value(config.GetLogRetentionHours()),
		LogRetentionMinutes:         api.UnwrapInt64Value(config.GetLogRetentionMinutes()),
		LogRetentionMs:              api.UnwrapInt64Value(config.GetLogRetentionMs()),
		LogSegmentBytes:             api.UnwrapInt64Value(config.GetLogSegmentBytes()),
		LogPreallocate:              api.UnwrapBoolValue(config.GetLogPreallocate()),
		SocketSendBufferBytes:       api.UnwrapInt64Value(config.GetSocketSendBufferBytes()),
		SocketReceiveBufferBytes:    api.UnwrapInt64Value(config.GetSocketReceiveBufferBytes()),
		AutoCreateTopicsEnable:      api.UnwrapBoolValue(config.GetAutoCreateTopicsEnable()),
		NumPartitions:               api.UnwrapInt64Value(config.GetNumPartitions()),
		DefaultReplicationFactor:    api.UnwrapInt64Value(config.GetDefaultReplicationFactor()),
		MessageMaxBytes:             api.UnwrapInt64Value(config.GetMessageMaxBytes()),
		ReplicaFetchMaxBytes:        api.UnwrapInt64Value(config.GetReplicaFetchMaxBytes()),
		SslCipherSuites:             SslCipherSuitesFromGRPC(config.GetSslCipherSuites()),
		OffsetsRetentionMinutes:     api.UnwrapInt64Value(config.GetOffsetsRetentionMinutes()),
	}, version
}

func KafkaConfigToGRPC(clusterConfig kfmodels.MDBClusterSpec, kafkaSpec *kfv1.ConfigSpec_Kafka) {
	if kfmodels.KafkaConfigIsEmpty(clusterConfig.Kafka.Config) {
		return
	}

	config := clusterConfig.Kafka.Config
	switch clusterConfig.Version {
	case kfmodels.Version2_1:
		kafkaSpec.KafkaConfig = &kfv1.ConfigSpec_Kafka_KafkaConfig_2_1{
			KafkaConfig_2_1: &kfv1.KafkaConfig2_1{
				CompressionType:             CompressionTypeToGRPC(config.CompressionType),
				LogFlushIntervalMessages:    api.WrapInt64Pointer(config.LogFlushIntervalMessages),
				LogFlushIntervalMs:          api.WrapInt64Pointer(config.LogFlushIntervalMs),
				LogFlushSchedulerIntervalMs: api.WrapInt64Pointer(config.LogFlushSchedulerIntervalMs),
				LogRetentionBytes:           api.WrapInt64Pointer(config.LogRetentionBytes),
				LogRetentionHours:           api.WrapInt64Pointer(config.LogRetentionHours),
				LogRetentionMinutes:         api.WrapInt64Pointer(config.LogRetentionMinutes),
				LogRetentionMs:              api.WrapInt64Pointer(config.LogRetentionMs),
				LogSegmentBytes:             api.WrapInt64Pointer(config.LogSegmentBytes),
				LogPreallocate:              api.WrapBoolPointer(config.LogPreallocate),
				SocketSendBufferBytes:       api.WrapInt64Pointer(config.SocketSendBufferBytes),
				SocketReceiveBufferBytes:    api.WrapInt64Pointer(config.SocketReceiveBufferBytes),
				AutoCreateTopicsEnable:      api.WrapBoolPointer(config.AutoCreateTopicsEnable),
				NumPartitions:               api.WrapInt64Pointer(config.NumPartitions),
				DefaultReplicationFactor:    api.WrapInt64Pointer(config.DefaultReplicationFactor),
				MessageMaxBytes:             api.WrapInt64Pointer(config.MessageMaxBytes),
				ReplicaFetchMaxBytes:        api.WrapInt64Pointer(config.ReplicaFetchMaxBytes),
				SslCipherSuites:             config.SslCipherSuites,
				OffsetsRetentionMinutes:     api.WrapInt64Pointer(config.OffsetsRetentionMinutes),
			},
		}
	case kfmodels.Version2_6:
		kafkaSpec.KafkaConfig = &kfv1.ConfigSpec_Kafka_KafkaConfig_2_6{
			KafkaConfig_2_6: &kfv1.KafkaConfig2_6{
				CompressionType:             CompressionTypeToGRPC(config.CompressionType),
				LogFlushIntervalMessages:    api.WrapInt64Pointer(config.LogFlushIntervalMessages),
				LogFlushIntervalMs:          api.WrapInt64Pointer(config.LogFlushIntervalMs),
				LogFlushSchedulerIntervalMs: api.WrapInt64Pointer(config.LogFlushSchedulerIntervalMs),
				LogRetentionBytes:           api.WrapInt64Pointer(config.LogRetentionBytes),
				LogRetentionHours:           api.WrapInt64Pointer(config.LogRetentionHours),
				LogRetentionMinutes:         api.WrapInt64Pointer(config.LogRetentionMinutes),
				LogRetentionMs:              api.WrapInt64Pointer(config.LogRetentionMs),
				LogSegmentBytes:             api.WrapInt64Pointer(config.LogSegmentBytes),
				LogPreallocate:              api.WrapBoolPointer(config.LogPreallocate),
				SocketSendBufferBytes:       api.WrapInt64Pointer(config.SocketSendBufferBytes),
				SocketReceiveBufferBytes:    api.WrapInt64Pointer(config.SocketReceiveBufferBytes),
				AutoCreateTopicsEnable:      api.WrapBoolPointer(config.AutoCreateTopicsEnable),
				NumPartitions:               api.WrapInt64Pointer(config.NumPartitions),
				DefaultReplicationFactor:    api.WrapInt64Pointer(config.DefaultReplicationFactor),
				MessageMaxBytes:             api.WrapInt64Pointer(config.MessageMaxBytes),
				ReplicaFetchMaxBytes:        api.WrapInt64Pointer(config.ReplicaFetchMaxBytes),
				SslCipherSuites:             config.SslCipherSuites,
				OffsetsRetentionMinutes:     api.WrapInt64Pointer(config.OffsetsRetentionMinutes),
			},
		}
	case kfmodels.Version2_8:
		kafkaSpec.KafkaConfig = &kfv1.ConfigSpec_Kafka_KafkaConfig_2_8{
			KafkaConfig_2_8: &kfv1.KafkaConfig2_8{
				CompressionType:             CompressionTypeToGRPC(config.CompressionType),
				LogFlushIntervalMessages:    api.WrapInt64Pointer(config.LogFlushIntervalMessages),
				LogFlushIntervalMs:          api.WrapInt64Pointer(config.LogFlushIntervalMs),
				LogFlushSchedulerIntervalMs: api.WrapInt64Pointer(config.LogFlushSchedulerIntervalMs),
				LogRetentionBytes:           api.WrapInt64Pointer(config.LogRetentionBytes),
				LogRetentionHours:           api.WrapInt64Pointer(config.LogRetentionHours),
				LogRetentionMinutes:         api.WrapInt64Pointer(config.LogRetentionMinutes),
				LogRetentionMs:              api.WrapInt64Pointer(config.LogRetentionMs),
				LogSegmentBytes:             api.WrapInt64Pointer(config.LogSegmentBytes),
				LogPreallocate:              api.WrapBoolPointer(config.LogPreallocate),
				SocketSendBufferBytes:       api.WrapInt64Pointer(config.SocketSendBufferBytes),
				SocketReceiveBufferBytes:    api.WrapInt64Pointer(config.SocketReceiveBufferBytes),
				AutoCreateTopicsEnable:      api.WrapBoolPointer(config.AutoCreateTopicsEnable),
				NumPartitions:               api.WrapInt64Pointer(config.NumPartitions),
				DefaultReplicationFactor:    api.WrapInt64Pointer(config.DefaultReplicationFactor),
				MessageMaxBytes:             api.WrapInt64Pointer(config.MessageMaxBytes),
				ReplicaFetchMaxBytes:        api.WrapInt64Pointer(config.ReplicaFetchMaxBytes),
				SslCipherSuites:             config.SslCipherSuites,
				OffsetsRetentionMinutes:     api.WrapInt64Pointer(config.OffsetsRetentionMinutes),
			},
		}
	case kfmodels.Version3_0:
		kafkaSpec.KafkaConfig = &kfv1.ConfigSpec_Kafka_KafkaConfig_3_0{
			KafkaConfig_3_0: &kfv1.KafkaConfig3_0{
				CompressionType:             CompressionTypeToGRPC(config.CompressionType),
				LogFlushIntervalMessages:    api.WrapInt64Pointer(config.LogFlushIntervalMessages),
				LogFlushIntervalMs:          api.WrapInt64Pointer(config.LogFlushIntervalMs),
				LogFlushSchedulerIntervalMs: api.WrapInt64Pointer(config.LogFlushSchedulerIntervalMs),
				LogRetentionBytes:           api.WrapInt64Pointer(config.LogRetentionBytes),
				LogRetentionHours:           api.WrapInt64Pointer(config.LogRetentionHours),
				LogRetentionMinutes:         api.WrapInt64Pointer(config.LogRetentionMinutes),
				LogRetentionMs:              api.WrapInt64Pointer(config.LogRetentionMs),
				LogSegmentBytes:             api.WrapInt64Pointer(config.LogSegmentBytes),
				LogPreallocate:              api.WrapBoolPointer(config.LogPreallocate),
				SocketSendBufferBytes:       api.WrapInt64Pointer(config.SocketSendBufferBytes),
				SocketReceiveBufferBytes:    api.WrapInt64Pointer(config.SocketReceiveBufferBytes),
				AutoCreateTopicsEnable:      api.WrapBoolPointer(config.AutoCreateTopicsEnable),
				NumPartitions:               api.WrapInt64Pointer(config.NumPartitions),
				DefaultReplicationFactor:    api.WrapInt64Pointer(config.DefaultReplicationFactor),
				MessageMaxBytes:             api.WrapInt64Pointer(config.MessageMaxBytes),
				ReplicaFetchMaxBytes:        api.WrapInt64Pointer(config.ReplicaFetchMaxBytes),
				SslCipherSuites:             config.SslCipherSuites,
				OffsetsRetentionMinutes:     api.WrapInt64Pointer(config.OffsetsRetentionMinutes),
			},
		}
	case kfmodels.Version3_1:
		kafkaSpec.KafkaConfig = &kfv1.ConfigSpec_Kafka_KafkaConfig_3_1{
			KafkaConfig_3_1: &kfv1.KafkaConfig3_1{
				CompressionType:             CompressionTypeToGRPC(config.CompressionType),
				LogFlushIntervalMessages:    api.WrapInt64Pointer(config.LogFlushIntervalMessages),
				LogFlushIntervalMs:          api.WrapInt64Pointer(config.LogFlushIntervalMs),
				LogFlushSchedulerIntervalMs: api.WrapInt64Pointer(config.LogFlushSchedulerIntervalMs),
				LogRetentionBytes:           api.WrapInt64Pointer(config.LogRetentionBytes),
				LogRetentionHours:           api.WrapInt64Pointer(config.LogRetentionHours),
				LogRetentionMinutes:         api.WrapInt64Pointer(config.LogRetentionMinutes),
				LogRetentionMs:              api.WrapInt64Pointer(config.LogRetentionMs),
				LogSegmentBytes:             api.WrapInt64Pointer(config.LogSegmentBytes),
				LogPreallocate:              api.WrapBoolPointer(config.LogPreallocate),
				SocketSendBufferBytes:       api.WrapInt64Pointer(config.SocketSendBufferBytes),
				SocketReceiveBufferBytes:    api.WrapInt64Pointer(config.SocketReceiveBufferBytes),
				AutoCreateTopicsEnable:      api.WrapBoolPointer(config.AutoCreateTopicsEnable),
				NumPartitions:               api.WrapInt64Pointer(config.NumPartitions),
				DefaultReplicationFactor:    api.WrapInt64Pointer(config.DefaultReplicationFactor),
				MessageMaxBytes:             api.WrapInt64Pointer(config.MessageMaxBytes),
				ReplicaFetchMaxBytes:        api.WrapInt64Pointer(config.ReplicaFetchMaxBytes),
				SslCipherSuites:             config.SslCipherSuites,
				OffsetsRetentionMinutes:     api.WrapInt64Pointer(config.OffsetsRetentionMinutes),
			},
		}
	// Into this case add all 3x versions after 3.1. Separate them by ','.
	case kfmodels.Version3_2:
		kafkaSpec.KafkaConfig = &kfv1.ConfigSpec_Kafka_KafkaConfig_3{
			KafkaConfig_3: &kfv1.KafkaConfig3{
				CompressionType:             CompressionTypeToGRPC(config.CompressionType),
				LogFlushIntervalMessages:    api.WrapInt64Pointer(config.LogFlushIntervalMessages),
				LogFlushIntervalMs:          api.WrapInt64Pointer(config.LogFlushIntervalMs),
				LogFlushSchedulerIntervalMs: api.WrapInt64Pointer(config.LogFlushSchedulerIntervalMs),
				LogRetentionBytes:           api.WrapInt64Pointer(config.LogRetentionBytes),
				LogRetentionHours:           api.WrapInt64Pointer(config.LogRetentionHours),
				LogRetentionMinutes:         api.WrapInt64Pointer(config.LogRetentionMinutes),
				LogRetentionMs:              api.WrapInt64Pointer(config.LogRetentionMs),
				LogSegmentBytes:             api.WrapInt64Pointer(config.LogSegmentBytes),
				LogPreallocate:              api.WrapBoolPointer(config.LogPreallocate),
				SocketSendBufferBytes:       api.WrapInt64Pointer(config.SocketSendBufferBytes),
				SocketReceiveBufferBytes:    api.WrapInt64Pointer(config.SocketReceiveBufferBytes),
				AutoCreateTopicsEnable:      api.WrapBoolPointer(config.AutoCreateTopicsEnable),
				NumPartitions:               api.WrapInt64Pointer(config.NumPartitions),
				DefaultReplicationFactor:    api.WrapInt64Pointer(config.DefaultReplicationFactor),
				MessageMaxBytes:             api.WrapInt64Pointer(config.MessageMaxBytes),
				ReplicaFetchMaxBytes:        api.WrapInt64Pointer(config.ReplicaFetchMaxBytes),
				SslCipherSuites:             config.SslCipherSuites,
				OffsetsRetentionMinutes:     api.WrapInt64Pointer(config.OffsetsRetentionMinutes),
			},
		}
	}
}

func modifyClusterArgsFromGRPC(request *kfv1.UpdateClusterRequest) (kafka.ModifyMDBClusterArgs, error) {
	if request.GetClusterId() == "" {
		return kafka.ModifyMDBClusterArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	args := kafka.ModifyMDBClusterArgs{ClusterID: request.GetClusterId()}
	paths := grpcapi.NewFieldPaths(request.UpdateMask.GetPaths())
	paths.Remove("cluster_id") // just ignore cluster_id

	if paths.Empty() {
		return kafka.ModifyMDBClusterArgs{}, semerr.InvalidInput("no fields to change in update request")
	}

	if paths.Remove("name") {
		args.Name.Set(request.GetName())
	}
	if paths.Remove("description") {
		args.Description.Set(request.GetDescription())
	}
	if paths.Remove("labels") {
		args.Labels.Set(request.GetLabels())
	}
	if paths.Remove("security_group_ids") {
		args.SecurityGroupIDs.Set(request.GetSecurityGroupIds())
	}
	if paths.Remove("deletion_protection") {
		args.DeletionProtection.Set(request.GetDeletionProtection())
	}
	if paths.Remove("maintenance_window") {
		window, err := MaintenanceWindowFromGRPC(request.GetMaintenanceWindow())
		if err != nil {
			return kafka.ModifyMDBClusterArgs{}, err
		}
		args.MaintenanceWindow.Set(window)
	}

	configSpecPaths := paths.Subtree("config_spec.")
	if !configSpecPaths.Empty() {
		configSpec, err := clusterConfigSpecUpdateFromGRPC(request.ConfigSpec, configSpecPaths)
		if err != nil {
			return kafka.ModifyMDBClusterArgs{}, err
		}
		args.ConfigSpec = configSpec
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return kafka.ModifyMDBClusterArgs{}, err
	}

	return args, nil
}

func clusterConfigSpecUpdateFromGRPC(spec *kfv1.ConfigSpec, paths *grpcapi.FieldPaths) (kfmodels.ClusterConfigSpecMDBUpdate, error) {
	update := kfmodels.ClusterConfigSpecMDBUpdate{}

	if paths.Remove("version") {
		update.Version.Set(spec.GetVersion())
	}
	if paths.Remove("zone_id") {
		update.ZoneID.Set(slices.DedupStrings(spec.GetZoneId()))
	}
	if paths.Remove("brokers_count") {
		update.BrokersCount.Set(spec.GetBrokersCount().GetValue())
	}
	if paths.Remove("assign_public_ip") {
		update.AssignPublicIP.Set(spec.GetAssignPublicIp())
	}
	if paths.Remove("unmanaged_topics") {
		update.UnmanagedTopics.Set(spec.GetUnmanagedTopics())
	}
	if paths.Remove("schema_registry") {
		update.SchemaRegistry.Set(spec.GetSchemaRegistry())
	}
	if paths.Remove("access") {
		update.Access = *AccessFromGRPC(spec.GetAccess())
	}

	kafkaPaths := paths.Subtree("kafka.")
	if !kafkaPaths.Empty() {
		kafkaConfigSpecUpdate, err := kafkaConfigSpecUpdateFromGRPC(spec.Kafka, kafkaPaths)
		if err != nil {
			return kfmodels.ClusterConfigSpecMDBUpdate{}, err
		}
		update.Kafka = kafkaConfigSpecUpdate
	}

	zookeeperPaths := paths.Subtree("zookeeper.")
	if !zookeeperPaths.Empty() {
		zookeeperUpdate, err := zookeeperConfigSpecUpdateFromGRPC(spec.Zookeeper, zookeeperPaths)
		if err != nil {
			return kfmodels.ClusterConfigSpecMDBUpdate{}, err
		}
		update.ZooKeeper = zookeeperUpdate
	}

	return update, nil
}

func kafkaConfigSpecUpdateFromGRPC(spec *kfv1.ConfigSpec_Kafka, paths *grpcapi.FieldPaths) (kfmodels.KafkaConfigSpecUpdate, error) {
	update := kfmodels.KafkaConfigSpecUpdate{}

	resourcesPaths := paths.Subtree("resources.")
	if !resourcesPaths.Empty() {
		resourcesUpdate, err := clusterResourcesUpdateFromGRPC(spec.GetResources(), resourcesPaths)
		if err != nil {
			return kfmodels.KafkaConfigSpecUpdate{}, err
		}
		update.Resources = resourcesUpdate
	}

	kafkaConfigUpdate, err := kafkaConfigUpdateFromGRPC(spec, paths)
	if err != nil {
		return kfmodels.KafkaConfigSpecUpdate{}, err
	}
	update.Config = kafkaConfigUpdate

	return update, nil
}

func clusterResourcesUpdateFromGRPC(resources *kfv1.Resources, paths *grpcapi.FieldPaths) (models.ClusterResourcesSpec, error) {
	update := models.ClusterResourcesSpec{}

	if paths.Remove("resource_preset_id") {
		update.ResourcePresetExtID.Set(resources.GetResourcePresetId())
	}
	if paths.Remove("disk_size") {
		update.DiskSize.Set(resources.GetDiskSize())
	}
	if paths.Remove("disk_type_id") {
		update.DiskTypeExtID.Set(resources.GetDiskTypeId())
	}

	return update, nil
}

func kafkaConfigUpdateFromGRPC(spec *kfv1.ConfigSpec_Kafka, rootPaths *grpcapi.FieldPaths) (kfmodels.KafkaConfigUpdate, error) {
	if spec.GetKafkaConfig() == nil {
		return kfmodels.KafkaConfigUpdate{}, nil
	}

	kafkaConfigPath := ""
	var config GRPCKafkaConfig

	switch kafkaConfig := spec.GetKafkaConfig().(type) {
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_2_1:
		config = kafkaConfig.KafkaConfig_2_1
		kafkaConfigPath = "kafka_config_2_1."
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_2_6:
		config = kafkaConfig.KafkaConfig_2_6
		kafkaConfigPath = "kafka_config_2_6."
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_2_8:
		config = kafkaConfig.KafkaConfig_2_8
		kafkaConfigPath = "kafka_config_2_8."
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_3_0:
		config = kafkaConfig.KafkaConfig_3_0
		kafkaConfigPath = "kafka_config_3_0."
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_3_1:
		config = kafkaConfig.KafkaConfig_3_1
		kafkaConfigPath = "kafka_config_3_1."
	case *kfv1.ConfigSpec_Kafka_KafkaConfig_3:
		config = kafkaConfig.KafkaConfig_3
		kafkaConfigPath = "kafka_config_3."
	}
	if config == nil {
		panic(fmt.Sprintf("unexpected KafkaConfig type: %+v", spec.GetKafkaConfig()))
	}

	paths := rootPaths.Subtree(kafkaConfigPath)
	if paths.Empty() {
		return kfmodels.KafkaConfigUpdate{}, nil
	}

	update := kfmodels.KafkaConfigUpdate{}

	if paths.Remove("compression_type") {
		update.CompressionType.Set(CompressionTypeFromGRPC(config.GetCompressionType()))
	}
	if paths.Remove("log_flush_interval_messages") {
		update.LogFlushIntervalMessages.Set(api.UnwrapInt64Value(config.GetLogFlushIntervalMessages()))
	}
	if paths.Remove("log_flush_interval_ms") {
		update.LogFlushIntervalMs.Set(api.UnwrapInt64Value(config.GetLogFlushIntervalMs()))
	}
	if paths.Remove("log_flush_scheduler_interval_ms") {
		update.LogFlushSchedulerIntervalMs.Set(api.UnwrapInt64Value(config.GetLogFlushSchedulerIntervalMs()))
	}
	if paths.Remove("log_retention_bytes") {
		update.LogRetentionBytes.Set(api.UnwrapInt64Value(config.GetLogRetentionBytes()))
	}
	if paths.Remove("log_retention_hours") {
		update.LogRetentionHours.Set(api.UnwrapInt64Value(config.GetLogRetentionHours()))
	}
	if paths.Remove("log_retention_minutes") {
		update.LogRetentionMinutes.Set(api.UnwrapInt64Value(config.GetLogRetentionMinutes()))
	}
	if paths.Remove("log_retention_ms") {
		update.LogRetentionMs.Set(api.UnwrapInt64Value(config.GetLogRetentionMs()))
	}
	if paths.Remove("log_segment_bytes") {
		update.LogSegmentBytes.Set(api.UnwrapInt64Value(config.GetLogSegmentBytes()))
	}
	if paths.Remove("log_preallocate") {
		update.LogPreallocate.Set(api.UnwrapBoolValue(config.GetLogPreallocate()))
	}
	if paths.Remove("socket_send_buffer_bytes") {
		update.SocketSendBufferBytes.Set(api.UnwrapInt64Value(config.GetSocketSendBufferBytes()))
	}
	if paths.Remove("socket_receive_buffer_bytes") {
		update.SocketReceiveBufferBytes.Set(api.UnwrapInt64Value(config.GetSocketReceiveBufferBytes()))
	}
	if paths.Remove("auto_create_topics_enable") {
		update.AutoCreateTopicsEnable.Set(api.UnwrapBoolValue(config.GetAutoCreateTopicsEnable()))
	}
	if paths.Remove("num_partitions") {
		update.NumPartitions.Set(api.UnwrapInt64Value(config.GetNumPartitions()))
	}
	if paths.Remove("default_replication_factor") {
		update.DefaultReplicationFactor.Set(api.UnwrapInt64Value(config.GetDefaultReplicationFactor()))
	}
	if paths.Remove("message_max_bytes") {
		update.MessageMaxBytes.Set(api.UnwrapInt64Value(config.GetMessageMaxBytes()))
	}
	if paths.Remove("replica_fetch_max_bytes") {
		update.ReplicaFetchMaxBytes.Set(api.UnwrapInt64Value(config.GetReplicaFetchMaxBytes()))
	}
	if paths.Remove("ssl_cipher_suites") {
		suites := config.GetSslCipherSuites()
		if err := validation.IsValidSslCipherSuitesSlice(suites); err != nil {
			return kfmodels.KafkaConfigUpdate{}, err
		}
		update.SslCipherSuites.Set(SslCipherSuitesFromGRPC(suites))
	}
	if paths.Remove("offsets_retention_minutes") {
		update.OffsetsRetentionMinutes.Set(api.UnwrapInt64Value(config.GetOffsetsRetentionMinutes()))
	}
	return update, nil
}

func zookeeperConfigSpecUpdateFromGRPC(spec *kfv1.ConfigSpec_Zookeeper, paths *grpcapi.FieldPaths) (kfmodels.ZookeeperConfigSpecUpdate, error) {
	update := kfmodels.ZookeeperConfigSpecUpdate{}

	resourcesPaths := paths.Subtree("resources.")
	if !resourcesPaths.Empty() {
		resourcesUpdate, err := clusterResourcesUpdateFromGRPC(spec.GetResources(), resourcesPaths)
		if err != nil {
			return kfmodels.ZookeeperConfigSpecUpdate{}, err
		}
		update.Resources = resourcesUpdate
	}

	return update, nil
}

func updateUserArgsFromGRPC(request *kfv1.UpdateUserRequest) (kfmodels.UpdateUserArgs, error) {
	if request.GetClusterId() == "" {
		return kfmodels.UpdateUserArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	if request.GetUserName() == "" {
		return kfmodels.UpdateUserArgs{}, semerr.InvalidInput("missing required argument: name")
	}

	args := kfmodels.UpdateUserArgs{
		ClusterID: request.GetClusterId(),
		Name:      request.GetUserName(),
	}
	paths := grpcapi.NewFieldPaths(request.GetUpdateMask().GetPaths())
	paths.Remove("cluster_id") // just ignore cluster_id
	paths.Remove("name")       // just ignore name

	if paths.Empty() {
		return kfmodels.UpdateUserArgs{}, semerr.InvalidInput("no fields to change in update request")
	}

	if paths.Remove("password") {
		if request.GetPassword() == "" {
			return kfmodels.UpdateUserArgs{}, semerr.InvalidInput("user password cannot be empty")
		}
		args.Password = secret.NewString(request.GetPassword())
	}
	if paths.Remove("permissions") {
		args.Permissions = UserPermissionsFromGRPC(request.GetPermissions())
		args.PermissionsChanged = true
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return kfmodels.UpdateUserArgs{}, err
	}

	return args, nil
}

func updateTopicArgsFromGRPC(request *kfv1.UpdateTopicRequest) (kafka.UpdateTopicArgs, error) {
	if request.GetClusterId() == "" {
		return kafka.UpdateTopicArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	if request.GetTopicName() == "" {
		return kafka.UpdateTopicArgs{}, semerr.InvalidInput("missing required argument: name")
	}

	args := kafka.UpdateTopicArgs{
		ClusterID: request.GetClusterId(),
		Name:      request.GetTopicName(),
	}
	paths := grpcapi.NewFieldPaths(request.UpdateMask.GetPaths())
	paths.Remove("cluster_id") // just ignore cluster_id
	paths.Remove("name")       // just ignore name
	paths.Remove("topic_name") // just ignore name

	if paths.Empty() {
		return kafka.UpdateTopicArgs{}, semerr.InvalidInput("no fields to change in update request")
	}

	topicSpecPaths := paths.Subtree("topic_spec.")
	if !topicSpecPaths.Empty() {
		topicSpec, err := topicSpecUpdateFromGRPC(request.TopicSpec, topicSpecPaths)
		if err != nil {
			return kafka.UpdateTopicArgs{}, err
		}
		args.TopicSpec = topicSpec
	}

	err := paths.MustBeEmpty()
	if err != nil {
		return kafka.UpdateTopicArgs{}, err
	}

	return args, nil
}

func topicSpecUpdateFromGRPC(spec *kfv1.TopicSpec, paths *grpcapi.FieldPaths) (kfmodels.TopicSpecUpdate, error) {
	update := kfmodels.TopicSpecUpdate{}

	if paths.Remove("partitions") {
		update.Partitions.Set(spec.GetPartitions().GetValue())
	}
	if paths.Remove("replication_factor") {
		update.ReplicationFactor.Set(spec.GetReplicationFactor().GetValue())
	}

	topicConfigUpdate, err := topicConfigUpdateFromGRPC(spec, paths)
	if err != nil {
		return kfmodels.TopicSpecUpdate{}, err
	}
	update.Config = topicConfigUpdate

	return update, nil
}

type GRPCTopicConfig interface {
	GetCompressionType() kfv1.CompressionType
	GetDeleteRetentionMs() *wrappers.Int64Value
	GetFileDeleteDelayMs() *wrappers.Int64Value
	GetFlushMessages() *wrappers.Int64Value
	GetFlushMs() *wrappers.Int64Value
	GetMinCompactionLagMs() *wrappers.Int64Value
	GetRetentionBytes() *wrappers.Int64Value
	GetRetentionMs() *wrappers.Int64Value
	GetMaxMessageBytes() *wrappers.Int64Value
	GetMinInsyncReplicas() *wrappers.Int64Value
	GetSegmentBytes() *wrappers.Int64Value
	GetPreallocate() *wrappers.BoolValue
}

func topicConfigUpdateFromGRPC(spec *kfv1.TopicSpec, rootPaths *grpcapi.FieldPaths) (kfmodels.TopicConfigUpdate, error) {
	if spec.GetTopicConfig() == nil {
		return kfmodels.TopicConfigUpdate{}, nil
	}

	topicConfigPath := ""
	var config GRPCTopicConfig
	cleanupPolicy := ""

	switch specTopicConfig := spec.GetTopicConfig().(type) {
	case *kfv1.TopicSpec_TopicConfig_2_1:
		config = specTopicConfig.TopicConfig_2_1
		cleanupPolicy = CleanupPolicyFromGRPC(specTopicConfig.TopicConfig_2_1.GetCleanupPolicy())
		topicConfigPath = "topic_config_2_1."
	case *kfv1.TopicSpec_TopicConfig_2_6:
		config = specTopicConfig.TopicConfig_2_6
		cleanupPolicy = CleanupPolicyFromGRPC(specTopicConfig.TopicConfig_2_6.GetCleanupPolicy())
		topicConfigPath = "topic_config_2_6."
	case *kfv1.TopicSpec_TopicConfig_2_8:
		config = specTopicConfig.TopicConfig_2_8
		cleanupPolicy = CleanupPolicyFromGRPC(specTopicConfig.TopicConfig_2_8.GetCleanupPolicy())
		topicConfigPath = "topic_config_2_8."
	case *kfv1.TopicSpec_TopicConfig_3_0:
		config = specTopicConfig.TopicConfig_3_0
		cleanupPolicy = CleanupPolicyFromGRPC(specTopicConfig.TopicConfig_3_0.GetCleanupPolicy())
		topicConfigPath = "topic_config_3_0."
	case *kfv1.TopicSpec_TopicConfig_3_1:
		config = specTopicConfig.TopicConfig_3_1
		cleanupPolicy = CleanupPolicyFromGRPC(specTopicConfig.TopicConfig_3_1.GetCleanupPolicy())
		topicConfigPath = "topic_config_3_1."
	case *kfv1.TopicSpec_TopicConfig_3:
		config = specTopicConfig.TopicConfig_3
		cleanupPolicy = CleanupPolicyFromGRPC(specTopicConfig.TopicConfig_3.GetCleanupPolicy())
		topicConfigPath = "topic_config_3."
	}
	if config == nil {
		panic(fmt.Sprintf("unexpected TopicConfig type: %+v", spec.GetTopicConfig()))
	}

	paths := rootPaths.Subtree(topicConfigPath)
	if paths.Empty() {
		return kfmodels.TopicConfigUpdate{}, nil
	}

	update := kfmodels.TopicConfigUpdate{}

	if paths.Remove("cleanup_policy") {
		update.CleanupPolicy.Set(cleanupPolicy)
	}
	if paths.Remove("compression_type") {
		update.CompressionType.Set(CompressionTypeFromGRPC(config.GetCompressionType()))
	}
	if paths.Remove("delete_retention_ms") {
		update.DeleteRetentionMs.Set(api.UnwrapInt64Value(config.GetDeleteRetentionMs()))
	}
	if paths.Remove("file_delete_delay_ms") {
		update.FileDeleteDelayMs.Set(api.UnwrapInt64Value(config.GetFileDeleteDelayMs()))
	}
	if paths.Remove("flush_messages") {
		update.FlushMessages.Set(api.UnwrapInt64Value(config.GetFlushMessages()))
	}
	if paths.Remove("flush_ms") {
		update.FlushMs.Set(api.UnwrapInt64Value(config.GetFlushMs()))
	}
	if paths.Remove("min_compaction_lag_ms") {
		update.MinCompactionLagMs.Set(api.UnwrapInt64Value(config.GetMinCompactionLagMs()))
	}
	if paths.Remove("retention_bytes") {
		update.RetentionBytes.Set(api.UnwrapInt64Value(config.GetRetentionBytes()))
	}
	if paths.Remove("retention_ms") {
		update.RetentionMs.Set(api.UnwrapInt64Value(config.GetRetentionMs()))
	}
	if paths.Remove("max_message_bytes") {
		update.MaxMessageBytes.Set(api.UnwrapInt64Value(config.GetMaxMessageBytes()))
	}
	if paths.Remove("min_insync_replicas") {
		update.MinInsyncReplicas.Set(api.UnwrapInt64Value(config.GetMinInsyncReplicas()))
	}
	if paths.Remove("segment_bytes") {
		update.SegmentBytes.Set(api.UnwrapInt64Value(config.GetSegmentBytes()))
	}
	if paths.Remove("preallocate") {
		update.Preallocate.Set(api.UnwrapBoolValue(config.GetPreallocate()))
	}

	return update, nil
}

func defaultResourcesToGRPC(resources consolemodels.DefaultResources) *kfv1console.KafkaClustersConfig_DefaultResources {
	return &kfv1console.KafkaClustersConfig_DefaultResources{
		ResourcePresetId: resources.ResourcePresetExtID,
		DiskTypeId:       resources.DiskTypeExtID,
		DiskSize:         resources.DiskSize,
		Generation:       resources.Generation,
		GenerationName:   resources.GenerationName,
	}
}

func withEmptyResult(opGrpc *operation.Operation) (*operation.Operation, error) {
	opGrpc.Result = &operation.Operation_Response{}
	return opGrpc, nil
}

func withResult(opGrpc *operation.Operation, message proto.Message) (*operation.Operation, error) {
	r, err := ptypes.MarshalAny(message)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal operation %+v response: %w", opGrpc, err)
	}
	opGrpc.Result = &operation.Operation_Response{Response: r}
	return opGrpc, nil
}

func operationToGRPC(ctx context.Context, op operations.Operation, kf kafka.Kafka, saltEnvMapper grpcapi.SaltEnvMapper, l log.Logger) (*operation.Operation, error) {
	opGrpc, err := grpcapi.OperationToGRPC(ctx, op, l)
	if err != nil {
		return nil, err
	}
	if op.Status != operations.StatusDone {
		return opGrpc, nil
	}

	switch v := op.MetaData.(type) {
	// Must be at first position before interfaces
	case kfmodels.MetadataDeleteCluster, kfmodels.MetadataDeleteTopic, kfmodels.MetadataDeleteUser:
		return withEmptyResult(opGrpc)
	// TODO: remove type list
	case kfmodels.MetadataCreateCluster, kfmodels.MetadataModifyCluster, kfmodels.MetadataStartCluster, kfmodels.MetadataStopCluster, kfmodels.MetadataRescheduleMaintenance:
		cluster, err := kf.MDBCluster(ctx, op.ClusterID)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ClusterToGRPC(cluster, saltEnvMapper))
	case kfmodels.TopicMetadata:
		topicName := v.GetTopicName()
		topic, err := kf.Topic(ctx, op.ClusterID, topicName)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, TopicToGRPC(topic))
	case kfmodels.UserMetadata:
		userName := v.GetUserName()
		user, err := kf.User(ctx, op.ClusterID, userName)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, UserToGRPC(user))
	case kfmodels.ConnectorMetadata:
		connectorName := v.GetConnectorName()
		connector, err := kf.Connector(ctx, op.ClusterID, connectorName)
		if err != nil {
			return opGrpc, nil
		}
		return withResult(opGrpc, ConnectorToGRPC(connector))
	}

	return opGrpc, nil
}

func ConnectorSpecFromGRPC(spec *kfv1.ConnectorSpec) kfmodels.ConnectorSpec {
	modelsSpec := kfmodels.ConnectorSpec{
		Name:       spec.GetName(),
		TasksMax:   spec.GetTasksMax().GetValue(),
		Properties: spec.GetProperties(),
	}

	switch configSpec := spec.GetConnectorConfig().(type) {
	case *kfv1.ConnectorSpec_ConnectorConfigMirrormaker:
		modelsSpec.Type = kfmodels.ConnectorTypeMirrormaker
		modelsSpec.MirrormakerConfig = MirrormakerConfigSpecFromGRPC(configSpec.ConnectorConfigMirrormaker)
	case *kfv1.ConnectorSpec_ConnectorConfigS3Sink:
		modelsSpec.Type = kfmodels.ConnectorTypeS3Sink
		modelsSpec.S3SinkConnectorConfig = S3SinkConnectorConfigSpecFromGRPC(configSpec.ConnectorConfigS3Sink)
	default:
		modelsSpec.Type = kfmodels.ConnectorTypeUnspecified
	}
	return modelsSpec
}

func MirrormakerConfigSpecFromGRPC(spec *kfv1.ConnectorConfigMirrorMakerSpec) kfmodels.MirrormakerConfigSpec {
	return kfmodels.MirrormakerConfigSpec{
		SourceCluster:     ClusterConnectionSpecFromGRPC(spec.GetSourceCluster()),
		TargetCluster:     ClusterConnectionSpecFromGRPC(spec.GetTargetCluster()),
		Topics:            spec.GetTopics(),
		ReplicationFactor: spec.ReplicationFactor.GetValue(),
	}
}

func S3SinkConnectorConfigSpecFromGRPC(spec *kfv1.ConnectorConfigS3SinkSpec) kfmodels.S3SinkConnectorConfigSpec {
	if spec == nil {
		return kfmodels.S3SinkConnectorConfigSpec{}
	}
	fileCompressionType := spec.GetFileCompressionType()
	if fileCompressionType == "" {
		fileCompressionType = "none"
	}
	return kfmodels.S3SinkConnectorConfigSpec{
		S3Connection:        S3ConnectionSpecFromGRPC(spec.GetS3Connection()),
		Topics:              spec.GetTopics(),
		FileCompressionType: fileCompressionType,
		FileMaxRecords:      spec.GetFileMaxRecords().GetValue(),
	}
}

func ClusterConnectionSpecFromGRPC(spec *kfv1.ClusterConnectionSpec) kfmodels.ClusterConnectionSpec {
	if spec == nil {
		return kfmodels.ClusterConnectionSpec{}
	}
	modelSpec := kfmodels.ClusterConnectionSpec{
		Alias: spec.GetAlias(),
	}
	switch connectSpec := spec.ClusterConnection.(type) {
	case *kfv1.ClusterConnectionSpec_ThisCluster:
		modelSpec.Type = kfmodels.ClusterConnectionTypeThisCluster
	case *kfv1.ClusterConnectionSpec_ExternalCluster:
		extCluster := connectSpec.ExternalCluster
		modelSpec.Type = kfmodels.ClusterConnectionTypeExternal
		modelSpec.BootstrapServers = extCluster.BootstrapServers
		modelSpec.SaslMechanism = strings.ToUpper(extCluster.SaslMechanism)
		modelSpec.SaslUsername = extCluster.SaslUsername
		modelSpec.SaslPassword = secret.NewString(extCluster.SaslPassword)
		modelSpec.SecurityProtocol = strings.ToUpper(extCluster.SecurityProtocol)
		modelSpec.SslTruststoreCertificates = extCluster.SslTruststoreCertificates
	default:
		modelSpec.Type = kfmodels.ClusterConnectionTypeUnspecified
	}
	return modelSpec
}

func S3ConnectionSpecFromGRPC(spec *kfv1.S3ConnectionSpec) kfmodels.S3ConnectionSpec {
	if spec == nil {
		return kfmodels.S3ConnectionSpec{}
	}
	result := kfmodels.S3ConnectionSpec{
		BucketName: spec.BucketName,
	}
	switch s3ConnectSpec := spec.Storage.(type) {
	case *kfv1.S3ConnectionSpec_ExternalS3:
		externalS3Storage := s3ConnectSpec.ExternalS3
		result.Type = kfmodels.S3ConnectionTypeExternal
		result.AccessKeyID = externalS3Storage.AccessKeyId
		result.SecretAccessKey = secret.NewString(externalS3Storage.SecretAccessKey)
		result.Endpoint = externalS3Storage.Endpoint
		result.Region = externalS3Storage.Region
	default:
		result.Type = kfmodels.S3ConnectionTypeUnspecified
	}
	return result
}

func ConnectorsToGRPC(connectors []kfmodels.Connector) []*kfv1.Connector {
	v := make([]*kfv1.Connector, 0, len(connectors))
	for _, connector := range connectors {
		v = append(v, ConnectorToGRPC(connector))
	}
	return v
}

func ConnectorToGRPC(cn kfmodels.Connector) *kfv1.Connector {
	connector := &kfv1.Connector{
		Name:       cn.Name,
		TasksMax:   api.WrapInt64(cn.TasksMax),
		Properties: cn.Properties,
		Status:     ConnectorStatusToGRPC(cn.Status),
		Health:     ConnectorHealthToGRPC(cn.Health),
	}
	if cn.Type == kfmodels.ConnectorTypeMirrormaker {
		connector.ConnectorConfig = &kfv1.Connector_ConnectorConfigMirrormaker{
			ConnectorConfigMirrormaker: MirrormakerConfigToGRPC(cn.MirrormakerConfig),
		}
	} else if cn.Type == kfmodels.ConnectorTypeS3Sink {
		connector.ConnectorConfig = &kfv1.Connector_ConnectorConfigS3Sink{
			ConnectorConfigS3Sink: S3SinkConnectorConfigToGRPC(cn.S3SinkConnectorConfig),
		}
	}

	return connector
}

func MirrormakerConfigToGRPC(mc kfmodels.MirrormakerConfig) *kfv1.ConnectorConfigMirrorMaker {
	return &kfv1.ConnectorConfigMirrorMaker{
		SourceCluster:     ClusterConnectionToGRPC(mc.SourceCluster),
		TargetCluster:     ClusterConnectionToGRPC(mc.TargetCluster),
		Topics:            mc.Topics,
		ReplicationFactor: api.WrapInt64(mc.ReplicationFactor),
	}
}

func S3SinkConnectorConfigToGRPC(s3SinkConnectorConfig kfmodels.S3SinkConnectorConfig) *kfv1.ConnectorConfigS3Sink {
	return &kfv1.ConnectorConfigS3Sink{
		S3Connection:        S3ConnectionToGRPC(s3SinkConnectorConfig.S3Connection),
		Topics:              s3SinkConnectorConfig.Topics,
		FileCompressionType: s3SinkConnectorConfig.FileCompressionType,
		FileMaxRecords:      api.WrapInt64(s3SinkConnectorConfig.FileMaxRecords),
	}
}

func ClusterConnectionToGRPC(cc kfmodels.ClusterConnection) *kfv1.ClusterConnection {
	if cc.Type == kfmodels.ClusterConnectionTypeThisCluster {
		return &kfv1.ClusterConnection{
			Alias:             cc.Alias,
			ClusterConnection: &kfv1.ClusterConnection_ThisCluster{},
		}
	}
	if cc.Type == kfmodels.ClusterConnectionTypeExternal {
		return &kfv1.ClusterConnection{
			Alias: cc.Alias,
			ClusterConnection: &kfv1.ClusterConnection_ExternalCluster{
				ExternalCluster: &kfv1.ExternalClusterConnection{
					BootstrapServers: cc.BootstrapServers,
					SaslUsername:     cc.SaslUsername,
					SaslMechanism:    cc.SaslMechanism,
					SecurityProtocol: cc.SecurityProtocol,
				},
			},
		}
	}
	return nil
}

func S3ConnectionToGRPC(s3 kfmodels.S3Connection) *kfv1.S3Connection {
	if s3.Type == kfmodels.S3ConnectionTypeExternal {
		return &kfv1.S3Connection{
			BucketName: s3.BucketName,
			Storage: &kfv1.S3Connection_ExternalS3{
				ExternalS3: &kfv1.ExternalS3Storage{
					AccessKeyId: s3.AccessKeyID,
					Endpoint:    s3.Endpoint,
					Region:      s3.Region,
				},
			},
		}
	}
	return nil
}

func UpdateConnectorArgsFromGRPC(request *kfv1.UpdateConnectorRequest) (kafka.UpdateConnectorArgs, error) {
	if request == nil {
		return kafka.UpdateConnectorArgs{},
			semerr.InvalidInput("nil UpdateConnectorRequest")
	}
	if request.GetClusterId() == "" {
		return kafka.UpdateConnectorArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}
	if request.GetConnectorName() == "" {
		return kafka.UpdateConnectorArgs{}, semerr.InvalidInput("missing required argument: connector_name")
	}
	args := kafka.UpdateConnectorArgs{
		ClusterID: request.GetClusterId(),
		Name:      request.GetConnectorName(),
	}
	paths := grpcapi.NewFieldPaths(request.UpdateMask.GetPaths())
	paths.Remove("cluster_id")     // just ignore cluster_id
	paths.Remove("name")           // just ignore name
	paths.Remove("connector_name") // just ignore name
	connectorSpecPaths := paths.Subtree("connector_spec.")
	connectorSpec, err := UpdateConnectorSpecFromGRPC(request.ConnectorSpec, connectorSpecPaths)
	if err != nil {
		return kafka.UpdateConnectorArgs{}, err
	}
	args.ConnectorSpec = connectorSpec
	err = paths.MustBeEmpty()
	if err != nil {
		return kafka.UpdateConnectorArgs{}, err
	}
	return args, nil
}

func UpdateConnectorSpecFromGRPC(spec *kfv1.UpdateConnectorSpec, paths *grpcapi.FieldPaths) (kfmodels.UpdateConnectorSpec, error) {
	if spec == nil {
		return kfmodels.UpdateConnectorSpec{},
			semerr.InvalidInput("nil UpdateConnectorSpec")
	}
	update := kfmodels.UpdateConnectorSpec{}
	hasChanges := false
	if paths.Remove("tasks_max") {
		update.TasksMax.Set(spec.GetTasksMax().GetValue())
		hasChanges = true
	}

	if paths.Remove("properties") {
		update.Properties = spec.GetProperties()
		update.PropertiesOverwrite = true
		hasChanges = true
		if !paths.Subtree("properties.").Empty() {
			return kfmodels.UpdateConnectorSpec{}, semerr.InvalidInput("can not use properties and properties.some_field update mask in one request")
		}
	}

	propertiesSubPath := paths.Subtree("properties.")
	if !propertiesSubPath.Empty() {
		update.Properties = spec.GetProperties()
		update.PropertiesContains = map[string]bool{}
		hasChanges = true
		for _, prop := range propertiesSubPath.SubPaths() {
			if _, ok := update.Properties[prop]; !ok {
				update.PropertiesContains[prop] = false
			} else {
				update.PropertiesContains[prop] = true
			}
			propertiesSubPath.Remove(prop)
		}
	}
	switch configSpec := spec.GetConnectorConfig().(type) {
	case *kfv1.UpdateConnectorSpec_ConnectorConfigMirrormaker:
		mmSubPath := paths.Subtree("connector_config_mirrormaker.")
		update.Type.Set(kfmodels.ConnectorTypeMirrormaker)
		mmConfig, ok := UpdateMirrormakerConfigSpecFromGRPC(configSpec.ConnectorConfigMirrormaker, mmSubPath)
		update.MirrormakerConfig = mmConfig
		if ok {
			hasChanges = true
		}
	case *kfv1.UpdateConnectorSpec_ConnectorConfigS3Sink:
		s3SinkConnectorSubPath := paths.Subtree("connector_config_s3_sink.")
		update.Type.Set(kfmodels.ConnectorTypeS3Sink)
		s3SinkConnectorConfig, ok := UpdateS3SinkConnectorConfigSpecFromGRPC(configSpec.ConnectorConfigS3Sink, s3SinkConnectorSubPath)
		update.S3SinkConnectorConfig = s3SinkConnectorConfig
		if ok {
			hasChanges = true
		}
	default:
	}
	if !hasChanges {
		return kfmodels.UpdateConnectorSpec{}, semerr.InvalidInput("no fields to change in update connector request")
	}
	return update, nil
}

func UpdateMirrormakerConfigSpecFromGRPC(spec *kfv1.ConnectorConfigMirrorMakerSpec, paths *grpcapi.FieldPaths) (kfmodels.UpdateMirrormakerConfigSpec, bool) {
	update := kfmodels.UpdateMirrormakerConfigSpec{}
	hasChanges := false
	if spec == nil {
		return kfmodels.UpdateMirrormakerConfigSpec{}, false
	}
	if paths.Remove("topics") {
		hasChanges = true
		update.Topics.Set(spec.Topics)
	}
	if paths.Remove("replication_factor") {
		hasChanges = true
		update.ReplicationFactor.Set(spec.GetReplicationFactor().GetValue())
	}
	sourceClusterSubpath := paths.Subtree("source_cluster.")
	cc, ok := UpdateClusterConnectionSpecFromGRPC(spec.GetSourceCluster(), sourceClusterSubpath)
	if ok {
		hasChanges = true
		update.SourceCluster = cc
	}
	targetClusterSubpath := paths.Subtree("target_cluster.")
	cc, ok = UpdateClusterConnectionSpecFromGRPC(spec.GetTargetCluster(), targetClusterSubpath)
	if ok {
		hasChanges = true
		update.TargetCluster = cc
	}
	return update, hasChanges
}

func UpdateS3SinkConnectorConfigSpecFromGRPC(spec *kfv1.UpdateConnectorConfigS3SinkSpec, paths *grpcapi.FieldPaths) (kfmodels.UpdateS3SinkConnectorConfigSpec, bool) {
	update := kfmodels.UpdateS3SinkConnectorConfigSpec{}
	hasChanges := false
	if spec == nil {
		return kfmodels.UpdateS3SinkConnectorConfigSpec{}, false
	}
	if paths.Remove("topics") {
		hasChanges = true
		update.Topics.Set(spec.GetTopics())
	}
	if paths.Remove("file_max_records") {
		hasChanges = true
		update.FileMaxRecords.Set(spec.GetFileMaxRecords().GetValue())
	}
	s3ConnectionSubpath := paths.Subtree("s3_connection.")
	s3, ok := UpdateS3ConnectionSpecFromGRPC(spec.GetS3Connection(), s3ConnectionSubpath)
	if ok {
		hasChanges = true
		update.S3Connection = s3
	}
	return update, hasChanges
}

func UpdateClusterConnectionSpecFromGRPC(spec *kfv1.ClusterConnectionSpec, paths *grpcapi.FieldPaths) (kfmodels.UpdateClusterConnectionSpec, bool) {
	update := kfmodels.UpdateClusterConnectionSpec{}
	hasChanges := false
	if spec == nil {
		return kfmodels.UpdateClusterConnectionSpec{}, false
	}
	if paths.Remove("alias") {
		update.Alias.Set(spec.GetAlias())
		hasChanges = true
	}
	if spec.GetClusterConnection() != nil {
		switch cc := spec.ClusterConnection.(type) {
		case *kfv1.ClusterConnectionSpec_ThisCluster:
			update.Type.Set(kfmodels.ClusterConnectionTypeThisCluster)
			paths.Remove("this_cluster")
			hasChanges = true
		case *kfv1.ClusterConnectionSpec_ExternalCluster:
			update.Type.Set(kfmodels.ClusterConnectionTypeExternal)
			hasChanges = true
			extPaths := paths.Subtree("external_cluster.")
			extCluster := cc.ExternalCluster
			if extPaths.Remove("bootstrap_servers") {
				update.BootstrapServers.Set(extCluster.BootstrapServers)
			}
			if extPaths.Remove("security_protocol") {
				update.SecurityProtocol.Set(strings.ToUpper(extCluster.SecurityProtocol))
			}
			if extPaths.Remove("sasl_mechanism") {
				update.SaslMechanism.Set(strings.ToUpper(extCluster.SaslMechanism))
			}
			if extPaths.Remove("sasl_username") {
				update.SaslUsername.Set(extCluster.SaslUsername)
			}
			if extPaths.Remove("sasl_password") {
				update.SaslPassword = secret.NewString(extCluster.GetSaslPassword())
				update.SaslPasswordUpdated = true
			} else {
				update.SaslPasswordUpdated = false
			}
			if extPaths.Remove("ssl_truststore_certificates") {
				update.SslTruststoreCertificates.Set(extCluster.GetSslTruststoreCertificates())
			}
		default:
		}
	}
	return update, hasChanges
}

func UpdateS3ConnectionSpecFromGRPC(spec *kfv1.S3ConnectionSpec, paths *grpcapi.FieldPaths) (kfmodels.UpdateS3ConnectionSpec, bool) {
	update := kfmodels.UpdateS3ConnectionSpec{}
	hasChanges := false
	if spec == nil {
		return kfmodels.UpdateS3ConnectionSpec{}, false
	}
	if paths.Remove("bucket_name") {
		update.BucketName.Set(spec.GetBucketName())
		hasChanges = true
	}
	if spec.GetStorage() != nil {
		switch storageSpec := spec.Storage.(type) {
		case *kfv1.S3ConnectionSpec_ExternalS3:
			update.Type.Set(kfmodels.S3ConnectionTypeExternal)
			hasChanges = true
			externalS3Paths := paths.Subtree("external_s3.")
			externalS3 := storageSpec.ExternalS3
			if externalS3Paths.Remove("access_key_id") {
				update.AccessKeyID.Set(externalS3.AccessKeyId)
			}
			if externalS3Paths.Remove("secret_access_key") {
				update.SecretAccessKey = secret.NewString(externalS3.GetSecretAccessKey())
				update.SecretAccessKeyUpdated = true
			} else {
				update.SecretAccessKeyUpdated = false
			}
			if externalS3Paths.Remove("endpoint") {
				update.Endpoint.Set(externalS3.Endpoint)
			}
			if externalS3Paths.Remove("region") {
				update.Region.Set(externalS3.Region)
			}
		default:
		}
	}
	return update, hasChanges
}

var (
	mapMaintenanceWindowDayStringToGRPC = map[string]kfv1.WeeklyMaintenanceWindow_WeekDay{
		"MON": kfv1.WeeklyMaintenanceWindow_MON,
		"TUE": kfv1.WeeklyMaintenanceWindow_TUE,
		"WED": kfv1.WeeklyMaintenanceWindow_WED,
		"THU": kfv1.WeeklyMaintenanceWindow_THU,
		"FRI": kfv1.WeeklyMaintenanceWindow_FRI,
		"SAT": kfv1.WeeklyMaintenanceWindow_SAT,
		"SUN": kfv1.WeeklyMaintenanceWindow_SUN,
	}
	mapMaintenanceWindowDayStringFromGRPC = reflectutil.ReverseMap(mapMaintenanceWindowDayStringToGRPC).(map[kfv1.WeeklyMaintenanceWindow_WeekDay]string)
)

func MaintenanceWindowToGRPC(window clusters.MaintenanceWindow) *kfv1.MaintenanceWindow {
	if window.Anytime() {
		return &kfv1.MaintenanceWindow{
			Policy: &kfv1.MaintenanceWindow_Anytime{
				Anytime: &kfv1.AnytimeMaintenanceWindow{},
			},
		}
	}

	day, found := mapMaintenanceWindowDayStringToGRPC[window.Day]
	if !found {
		day = kfv1.WeeklyMaintenanceWindow_WEEK_DAY_UNSPECIFIED
	}

	return &kfv1.MaintenanceWindow{
		Policy: &kfv1.MaintenanceWindow_WeeklyMaintenanceWindow{
			WeeklyMaintenanceWindow: &kfv1.WeeklyMaintenanceWindow{
				Day:  day,
				Hour: int64(window.Hour),
			},
		},
	}
}

func MaintenanceWindowFromGRPC(window *kfv1.MaintenanceWindow) (clusters.MaintenanceWindow, error) {
	if window == nil {
		return clusters.NewAnytimeMaintenanceWindow(), nil
	}

	switch p := window.GetPolicy().(type) {
	case *kfv1.MaintenanceWindow_Anytime:
		return clusters.NewAnytimeMaintenanceWindow(), nil
	case *kfv1.MaintenanceWindow_WeeklyMaintenanceWindow:
		day, ok := mapMaintenanceWindowDayStringFromGRPC[p.WeeklyMaintenanceWindow.GetDay()]
		if !ok {
			return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid maintenance window data: %q", p.WeeklyMaintenanceWindow.GetDay().String())
		}

		return clusters.NewWeeklyMaintenanceWindow(day, int(p.WeeklyMaintenanceWindow.GetHour())), nil
	default:
		return clusters.MaintenanceWindow{}, semerr.InvalidInputf("invalid maintenance window type")
	}
}

func MaintenanceOperationToGRPC(op clusters.MaintenanceOperation) *kfv1.MaintenanceOperation {
	if !op.Valid() {
		return nil
	}

	return &kfv1.MaintenanceOperation{
		Info:                      op.Info,
		DelayedUntil:              grpcapi.TimeToGRPC(op.DelayedUntil),
		LatestMaintenanceTime:     grpcapi.TimeToGRPC(op.LatestMaintenanceTime),
		NextMaintenanceWindowTime: grpcapi.TimePtrToGRPC(op.NearestMaintenanceWindow),
	}
}

var (
	mapRescheduleTypeToGRPC = map[clusters.RescheduleType]kfv1.RescheduleMaintenanceRequest_RescheduleType{
		clusters.RescheduleTypeUnspecified:         kfv1.RescheduleMaintenanceRequest_RESCHEDULE_TYPE_UNSPECIFIED,
		clusters.RescheduleTypeImmediate:           kfv1.RescheduleMaintenanceRequest_IMMEDIATE,
		clusters.RescheduleTypeNextAvailableWindow: kfv1.RescheduleMaintenanceRequest_NEXT_AVAILABLE_WINDOW,
		clusters.RescheduleTypeSpecificTime:        kfv1.RescheduleMaintenanceRequest_SPECIFIC_TIME,
	}
	mapRescheduleTypeFromGRPC = reflectutil.ReverseMap(mapRescheduleTypeToGRPC).(map[kfv1.RescheduleMaintenanceRequest_RescheduleType]clusters.RescheduleType)
)

func RescheduleTypeFromGRPC(ar kfv1.RescheduleMaintenanceRequest_RescheduleType) (clusters.RescheduleType, error) {
	v, ok := mapRescheduleTypeFromGRPC[ar]
	if !ok {
		return clusters.RescheduleTypeUnspecified, semerr.InvalidInput("unknown reschedule type")
	}

	return v, nil
}
