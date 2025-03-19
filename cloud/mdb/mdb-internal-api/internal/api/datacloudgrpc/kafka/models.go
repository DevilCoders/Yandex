package kafka

import (
	"github.com/golang/protobuf/ptypes/wrappers"

	kfconsolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/console/v1"
	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	datacloudv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/datacloudgrpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func CreateClusterArgsFromGRPC(req *kfv1.CreateClusterRequest) (kafka.CreateDataCloudClusterArgs, error) {
	cloudType, err := environment.ParseCloudType(req.GetCloudType())
	if err != nil {
		return kafka.CreateDataCloudClusterArgs{}, err
	}

	version := req.GetVersion()
	if version != "" {
		modelVersion, err := kfmodels.FindVersion(version)
		if err != nil {
			return kafka.CreateDataCloudClusterArgs{}, err
		}
		if modelVersion.Deprecated {
			return kafka.CreateDataCloudClusterArgs{}, semerr.InvalidInputf("version %s is deprecated", version)
		}
	}

	if req.GetResources() == nil {
		return kafka.CreateDataCloudClusterArgs{}, semerr.InvalidInput("kafka resources must be set")
	}

	return kafka.CreateDataCloudClusterArgs{
		ProjectID:   req.GetProjectId(),
		Name:        req.GetName(),
		Description: req.GetDescription(),
		RegionID:    req.GetRegionId(),
		CloudType:   cloudType,
		ClusterSpec: kfmodels.DataCloudClusterSpec{
			Version:      version,
			BrokersCount: req.GetResources().GetKafka().GetBrokerCount().GetValue(),
			ZoneCount:    req.GetResources().GetKafka().GetZoneCount().GetValue(),
			Kafka: kfmodels.KafkaConfigSpec{
				Resources: models.ClusterResources{
					DiskSize:            req.GetResources().GetKafka().GetDiskSize().GetValue(),
					ResourcePresetExtID: req.GetResources().GetKafka().GetResourcePresetId(),
				},
			},
			Access:     dataCloudAccessFromGRPC(req.Access),
			Encryption: dataCloudEncryptionFromGRPC(req.Encryption),
		},
		NetworkID: grpc.OptionalStringFromGRPC(req.GetNetworkId()),
	}, nil
}

func ClusterToGRPC(cluster kfmodels.DataCloudCluster) *kfv1.Cluster {
	v := &kfv1.Cluster{
		Id:                    cluster.ClusterID,
		ProjectId:             cluster.FolderExtID,
		CloudType:             string(cluster.CloudType),
		CreateTime:            grpc.TimeToGRPC(cluster.CreatedAt),
		Name:                  cluster.Name,
		Description:           cluster.Description,
		Status:                datacloudgrpc.StatusToGRPC(cluster.Status, cluster.Health),
		Version:               cluster.Version,
		ConnectionInfo:        ConnectionInfoToGRPC(cluster.ConnectionInfo),
		PrivateConnectionInfo: PrivateConnectionInfoToGRPC(cluster.PrivateConnectionInfo),
		RegionId:              cluster.RegionID,
		Resources: &kfv1.ClusterResources{
			Kafka: &kfv1.ClusterResources_Kafka{
				ResourcePresetId: cluster.Resources.ResourcePresetID.String,
				DiskSize:         grpc.OptionalInt64ToGRPC(cluster.Resources.DiskSize),
				BrokerCount:      grpc.OptionalInt64ToGRPC(cluster.Resources.BrokerCount),
				ZoneCount:        grpc.OptionalInt64ToGRPC(cluster.Resources.ZoneCount),
			},
		},
		Access:     dataCloudAccessToGRPC(cluster.Access),
		Encryption: dataCloudEncryptionToGRPC(cluster.Encryption),
		NetworkId:  cluster.NetworkID,
	}

	return v
}

func ClustersToGRPC(clusters []kfmodels.DataCloudCluster) []*kfv1.Cluster {
	var v []*kfv1.Cluster
	for _, cluster := range clusters {
		v = append(v, ClusterToGRPC(cluster))
	}
	return v
}

func ConnectionInfoToGRPC(info kfmodels.ConnectionInfo) *kfv1.ConnectionInfo {
	return &kfv1.ConnectionInfo{
		ConnectionString: info.ConnectionString,
		User:             info.User,
		Password:         info.Password.Unmask(),
	}
}

func PrivateConnectionInfoToGRPC(info kfmodels.PrivateConnectionInfo) *kfv1.PrivateConnectionInfo {
	return &kfv1.PrivateConnectionInfo{
		ConnectionString: info.ConnectionString,
		User:             info.User,
		Password:         info.Password.Unmask(),
	}
}

func CloudsToGRPC(clouds []consolemodels.Cloud) []*kfconsolev1.Cloud {
	res := make([]*kfconsolev1.Cloud, 0, len(clouds))
	for _, cloud := range clouds {
		res = append(res, &kfconsolev1.Cloud{
			CloudType: string(cloud.Cloud),
			RegionIds: cloud.Regions,
		})
	}

	return res
}

var (
	mapCleanupPolicy28FromGRPC = map[kfv1.TopicConfig28_CleanupPolicy]string{
		kfv1.TopicConfig28_CLEANUP_POLICY_INVALID:            kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig28_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy28ToGRPC   = reflectutil.ReverseMap(mapCleanupPolicy28FromGRPC).(map[string]kfv1.TopicConfig28_CleanupPolicy)
	mapCleanupPolicy30FromGRPC = map[kfv1.TopicConfig30_CleanupPolicy]string{
		kfv1.TopicConfig30_CLEANUP_POLICY_INVALID:            kfmodels.CleanupPolicyUnspecified,
		kfv1.TopicConfig30_CLEANUP_POLICY_DELETE:             kfmodels.CleanupPolicyDelete,
		kfv1.TopicConfig30_CLEANUP_POLICY_COMPACT:            kfmodels.CleanupPolicyCompact,
		kfv1.TopicConfig30_CLEANUP_POLICY_COMPACT_AND_DELETE: kfmodels.CleanupPolicyCompactAndDelete,
	}
	mapCleanupPolicy30ToGRPC = reflectutil.ReverseMap(mapCleanupPolicy30FromGRPC).(map[string]kfv1.TopicConfig30_CleanupPolicy)
)

type grpcTopicConfig interface {
	GetRetentionBytes() *wrappers.Int64Value
	GetRetentionMs() *wrappers.Int64Value
}

func topicConfigFromGRPC(spec *kfv1.TopicSpec) (kfmodels.TopicConfig, error) {
	if spec.GetTopicConfig() == nil {
		return kfmodels.TopicConfig{}, nil
	}
	var config grpcTopicConfig
	cleanupPolicy := ""
	compressionType := ""
	version := ""

	switch topicConfig := spec.GetTopicConfig().(type) {
	case *kfv1.TopicSpec_TopicConfig_2_8:
		version = kfmodels.Version2_8
		config = topicConfig.TopicConfig_2_8
		cleanupPolicy = cleanupPolicyFromGRPC(topicConfig.TopicConfig_2_8.GetCleanupPolicy())
		compressionType = compressionTypeFromGRPC(topicConfig.TopicConfig_2_8.GetCompressionType())
	case *kfv1.TopicSpec_TopicConfig_3_0:
		version = kfmodels.Version3_0
		config = topicConfig.TopicConfig_3_0
		cleanupPolicy = cleanupPolicyFromGRPC(topicConfig.TopicConfig_3_0.GetCleanupPolicy())
		compressionType = compressionTypeFromGRPC(topicConfig.TopicConfig_3_0.GetCompressionType())
	}
	if config == nil {
		return kfmodels.TopicConfig{}, semerr.InvalidInputf("unexpected TopicConfig type: %+v", spec.GetTopicConfig())
	}
	return kfmodels.TopicConfig{
		Version:         version,
		CleanupPolicy:   cleanupPolicy,
		CompressionType: compressionType,
		RetentionBytes:  api.UnwrapInt64Value(config.GetRetentionBytes()),
		RetentionMs:     api.UnwrapInt64Value(config.GetRetentionMs()),
	}, nil
}

func cleanupPolicyFromGRPC(cp interface{}) string {
	result := ""
	switch v := cp.(type) {
	case kfv1.TopicConfig28_CleanupPolicy:
		ret, ok := mapCleanupPolicy28FromGRPC[v]
		if ok {
			result = ret
		}
	case kfv1.TopicConfig30_CleanupPolicy:
		ret, ok := mapCleanupPolicy30FromGRPC[v]
		if ok {
			result = ret
		}
	}
	return result
}

func cleanupPolicy28ToGRPC(cp string) kfv1.TopicConfig28_CleanupPolicy {
	v, ok := mapCleanupPolicy28ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig28_CLEANUP_POLICY_INVALID
	}
	return v
}

func cleanupPolicy30ToGRPC(cp string) kfv1.TopicConfig30_CleanupPolicy {
	v, ok := mapCleanupPolicy30ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig30_CLEANUP_POLICY_INVALID
	}
	return v
}

var (
	mapCompressionType28FromGRPC = map[kfv1.TopicConfig28_CompressionType]string{
		kfv1.TopicConfig28_COMPRESSION_TYPE_INVALID:      kfmodels.CompressionTypeUnspecified,
		kfv1.TopicConfig28_COMPRESSION_TYPE_UNCOMPRESSED: kfmodels.CompressionTypeUncompressed,
		kfv1.TopicConfig28_COMPRESSION_TYPE_ZSTD:         kfmodels.CompressionTypeZstd,
		kfv1.TopicConfig28_COMPRESSION_TYPE_LZ4:          kfmodels.CompressionTypeLz4,
		kfv1.TopicConfig28_COMPRESSION_TYPE_SNAPPY:       kfmodels.CompressionTypeSnappy,
		kfv1.TopicConfig28_COMPRESSION_TYPE_GZIP:         kfmodels.CompressionTypeGzip,
		kfv1.TopicConfig28_COMPRESSION_TYPE_PRODUCER:     kfmodels.CompressionTypeProducer,
	}
	mapCompressionType28ToGRPC   = reflectutil.ReverseMap(mapCompressionType28FromGRPC).(map[string]kfv1.TopicConfig28_CompressionType)
	mapCompressionType30FromGRPC = map[kfv1.TopicConfig30_CompressionType]string{
		kfv1.TopicConfig30_COMPRESSION_TYPE_INVALID:      kfmodels.CompressionTypeUnspecified,
		kfv1.TopicConfig30_COMPRESSION_TYPE_UNCOMPRESSED: kfmodels.CompressionTypeUncompressed,
		kfv1.TopicConfig30_COMPRESSION_TYPE_ZSTD:         kfmodels.CompressionTypeZstd,
		kfv1.TopicConfig30_COMPRESSION_TYPE_LZ4:          kfmodels.CompressionTypeLz4,
		kfv1.TopicConfig30_COMPRESSION_TYPE_SNAPPY:       kfmodels.CompressionTypeSnappy,
		kfv1.TopicConfig30_COMPRESSION_TYPE_GZIP:         kfmodels.CompressionTypeGzip,
		kfv1.TopicConfig30_COMPRESSION_TYPE_PRODUCER:     kfmodels.CompressionTypeProducer,
	}
	mapCompressionType30ToGRPC = reflectutil.ReverseMap(mapCompressionType30FromGRPC).(map[string]kfv1.TopicConfig30_CompressionType)
)

func topicSpecUpdateFromGRPC(spec *kfv1.TopicSpec) (kfmodels.TopicSpecUpdate, error) {
	topicConfigUpdate, err := topicConfigUpdateFromGRPC(spec)
	if err != nil {
		return kfmodels.TopicSpecUpdate{}, err
	}

	return kfmodels.TopicSpecUpdate{
		Partitions:        grpc.OptionalInt64FromGRPC(spec.GetPartitions()),
		ReplicationFactor: grpc.OptionalInt64FromGRPC(spec.GetReplicationFactor()),
		Config:            topicConfigUpdate,
	}, nil
}

func topicConfigUpdateFromGRPC(spec *kfv1.TopicSpec) (kfmodels.TopicConfigUpdate, error) {
	if spec.GetTopicConfig() == nil {
		return kfmodels.TopicConfigUpdate{}, nil
	}

	var config grpcTopicConfig
	cleanupPolicy := ""
	compressionType := ""

	switch specTopicConfig := spec.GetTopicConfig().(type) {
	case *kfv1.TopicSpec_TopicConfig_2_8:
		config = specTopicConfig.TopicConfig_2_8
		cleanupPolicy = cleanupPolicyFromGRPC(specTopicConfig.TopicConfig_2_8.GetCleanupPolicy())
		compressionType = compressionTypeFromGRPC(specTopicConfig.TopicConfig_2_8.GetCompressionType())
	case *kfv1.TopicSpec_TopicConfig_3_0:
		config = specTopicConfig.TopicConfig_3_0
		cleanupPolicy = cleanupPolicyFromGRPC(specTopicConfig.TopicConfig_3_0.GetCleanupPolicy())
		compressionType = compressionTypeFromGRPC(specTopicConfig.TopicConfig_3_0.GetCompressionType())
	}
	if config == nil {
		return kfmodels.TopicConfigUpdate{}, semerr.InvalidInputf("unexpected TopicConfig type: %+v", spec.GetTopicConfig())
	}

	return kfmodels.TopicConfigUpdate{
		CleanupPolicy:   grpc.OptionalStringFromGRPC(cleanupPolicy),
		CompressionType: grpc.OptionalStringFromGRPC(compressionType),
		RetentionBytes:  api.UnwrapInt64PointerToOptional(config.GetRetentionBytes()),
		RetentionMs:     api.UnwrapInt64PointerToOptional(config.GetRetentionMs()),
	}, nil
}

func compressionTypeFromGRPC(cp interface{}) string {
	result := ""
	switch v := cp.(type) {
	case kfv1.TopicConfig28_CompressionType:
		ret, ok := mapCompressionType28FromGRPC[v]
		if ok {
			result = ret
		}
	case kfv1.TopicConfig30_CompressionType:
		ret, ok := mapCompressionType30FromGRPC[v]
		if ok {
			result = ret
		}
	}
	return result
}

func compressionType28ToGRPC(cp string) kfv1.TopicConfig28_CompressionType {
	v, ok := mapCompressionType28ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig28_COMPRESSION_TYPE_INVALID
	}
	return v
}

func compressionType30ToGRPC(cp string) kfv1.TopicConfig30_CompressionType {
	v, ok := mapCompressionType30ToGRPC[cp]
	if !ok {
		return kfv1.TopicConfig30_COMPRESSION_TYPE_INVALID
	}
	return v
}

func topicConfigToGRPC(tp kfmodels.Topic, topic *kfv1.Topic) error {
	config := tp.Config
	switch config.Version {
	case kfmodels.Version2_8:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_2_8{
			TopicConfig_2_8: &kfv1.TopicConfig28{
				CleanupPolicy:   cleanupPolicy28ToGRPC(config.CleanupPolicy),
				CompressionType: compressionType28ToGRPC(config.CompressionType),
				RetentionBytes:  api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:     api.WrapInt64Pointer(config.RetentionMs),
			},
		}
	case kfmodels.Version3_0:
		topic.TopicConfig = &kfv1.Topic_TopicConfig_3_0{
			TopicConfig_3_0: &kfv1.TopicConfig30{
				CleanupPolicy:   cleanupPolicy30ToGRPC(config.CleanupPolicy),
				CompressionType: compressionType30ToGRPC(config.CompressionType),
				RetentionBytes:  api.WrapInt64Pointer(config.RetentionBytes),
				RetentionMs:     api.WrapInt64Pointer(config.RetentionMs),
			},
		}
	default:
		return semerr.InvalidInputf("do not know how to convert TopicConfig of kafka version \"%s\" to grpc format", config.Version)
	}

	return nil
}

func TopicSpecFromGRPC(spec *kfv1.TopicSpec) (kfmodels.TopicSpec, error) {
	topicConfig, err := topicConfigFromGRPC(spec)
	if err != nil {
		return kfmodels.TopicSpec{}, err
	}

	return kfmodels.TopicSpec{
		Name:              spec.GetName(),
		Partitions:        spec.GetPartitions().GetValue(),
		ReplicationFactor: spec.GetReplicationFactor().GetValue(),
		Config:            topicConfig,
	}, nil
}

func TopicToGRPC(tp kfmodels.Topic) (*kfv1.Topic, error) {
	topic := &kfv1.Topic{
		Name:              tp.Name,
		ClusterId:         tp.ClusterID,
		Partitions:        api.WrapInt64(tp.Partitions),
		ReplicationFactor: api.WrapInt64(tp.ReplicationFactor),
	}
	err := topicConfigToGRPC(tp, topic)
	if err != nil {
		return nil, err
	}

	return topic, nil
}

func TopicsToGRPC(tps []kfmodels.Topic) ([]*kfv1.Topic, error) {
	var v []*kfv1.Topic
	for _, tp := range tps {
		topic, err := TopicToGRPC(tp)
		if err != nil {
			return nil, err
		}
		v = append(v, topic)
	}
	return v, nil
}

func UpdateTopicArgsFromGRPC(request *kfv1.UpdateTopicRequest) (kafka.UpdateTopicArgs, error) {
	if request.GetClusterId() == "" {
		return kafka.UpdateTopicArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	if request.GetTopicName() == "" {
		return kafka.UpdateTopicArgs{}, semerr.InvalidInput("missing required argument: name")
	}

	topicSpec, err := topicSpecUpdateFromGRPC(request.TopicSpec)
	if err != nil {
		return kafka.UpdateTopicArgs{}, err
	}
	return kafka.UpdateTopicArgs{
		ClusterID: request.GetClusterId(),
		Name:      request.GetTopicName(),
		TopicSpec: topicSpec,
	}, nil
}

func modifyClusterArgsFromGRPC(request *kfv1.UpdateClusterRequest) (kafka.ModifyDataCloudClusterArgs, error) {
	if request.GetClusterId() == "" {
		return kafka.ModifyDataCloudClusterArgs{}, semerr.InvalidInput("missing required argument: cluster_id")
	}

	return kafka.ModifyDataCloudClusterArgs{
		ClusterID:   request.GetClusterId(),
		Name:        grpc.OptionalStringFromGRPC(request.GetName()),
		Description: grpc.OptionalStringFromGRPC(request.GetDescription()),
		ConfigSpec:  clusterKafkaConfigSpecUpdateFromGRPC(request),
	}, nil
}

func clusterKafkaConfigSpecUpdateFromGRPC(request *kfv1.UpdateClusterRequest) kfmodels.ClusterConfigSpecDataCloudUpdate {
	return kfmodels.ClusterConfigSpecDataCloudUpdate{
		Version:      grpc.OptionalStringFromGRPC(request.Version),
		BrokersCount: grpc.OptionalInt64FromGRPC(request.GetResources().GetKafka().GetBrokerCount()),
		ZoneCount:    grpc.OptionalInt64FromGRPC(request.GetResources().GetKafka().GetZoneCount()),
		Kafka: kfmodels.KafkaConfigSpecUpdate{
			Resources: models.ClusterResourcesSpec{
				ResourcePresetExtID: grpc.OptionalStringFromGRPC(request.GetResources().GetKafka().GetResourcePresetId()),
				DiskSize:            grpc.OptionalInt64FromGRPC(request.GetResources().GetKafka().GetDiskSize()),
			},
		},
		Access: dataCloudAccessFromGRPC(request.Access),
	}
}

var (
	mapHostHealthToGRPC = map[hosts.Status]datacloudv1.HostStatus{
		hosts.StatusAlive:    datacloudv1.HostStatus_HOST_STATUS_ALIVE,
		hosts.StatusDegraded: datacloudv1.HostStatus_HOST_STATUS_DEGRADED,
		hosts.StatusDead:     datacloudv1.HostStatus_HOST_STATUS_DEAD,
		hosts.StatusUnknown:  datacloudv1.HostStatus_HOST_STATUS_INVALID,
	}
)

func HostStatusToGRPC(hh hosts.Health) datacloudv1.HostStatus {
	v, ok := mapHostHealthToGRPC[hh.Status]
	if !ok {
		return datacloudv1.HostStatus_HOST_STATUS_INVALID
	}
	return v
}

func HostToGRPC(host hosts.HostExtended) *kfv1.Host {
	h := &kfv1.Host{
		Name:      host.FQDN,
		ClusterId: host.ClusterID,
		Status:    HostStatusToGRPC(host.Health),
	}
	return h
}

func HostsToGRPC(clusterHosts []hosts.HostExtended) ([]*kfv1.Host, error) {
	v := make([]*kfv1.Host, 0, len(clusterHosts))
	for _, host := range clusterHosts {
		if len(host.Roles) < 1 {
			return v, xerrors.Errorf("host role is unspecified")
		}
		if host.Roles[0] == hosts.RoleKafka {
			v = append(v, HostToGRPC(host))
		}
	}
	return v, nil
}

func wrapInt64(val int64) *wrappers.Int64Value {
	if val == 0 {
		return nil
	}
	return &wrappers.Int64Value{Value: val}
}

func dataCloudAccessToGRPC(access clusters.Access) *datacloudv1.Access {
	grpcDataCloudAccess := datacloudv1.Access{}
	settingsExists := false

	if access.Ipv4CidrBlocks != nil {
		var cidrBlocks []*datacloudv1.Access_CidrBlock
		for _, block := range access.Ipv4CidrBlocks {
			cidrBlocks = append(cidrBlocks, &datacloudv1.Access_CidrBlock{Value: block.Value, Description: block.Description})
		}
		grpcDataCloudAccess.Ipv4CidrBlocks = &datacloudv1.Access_CidrBlockList{Values: cidrBlocks}
		settingsExists = true
	}

	if access.Ipv6CidrBlocks != nil {
		var cidrBlocks []*datacloudv1.Access_CidrBlock
		for _, block := range access.Ipv6CidrBlocks {
			cidrBlocks = append(cidrBlocks, &datacloudv1.Access_CidrBlock{Value: block.Value, Description: block.Description})
		}
		grpcDataCloudAccess.Ipv6CidrBlocks = &datacloudv1.Access_CidrBlockList{Values: cidrBlocks}
		settingsExists = true
	}

	var dataServices []datacloudv1.Access_DataService
	if b, err := access.DataTransfer.Get(); err == nil && b {
		dataServices = append(dataServices, datacloudv1.Access_DATA_SERVICE_TRANSFER)
		settingsExists = true
	}
	grpcDataCloudAccess.DataServices = &datacloudv1.Access_DataServiceList{Values: dataServices}

	if !settingsExists {
		return nil
	}
	return &grpcDataCloudAccess
}

func dataCloudAccessFromGRPC(reqAccess *datacloudv1.Access) clusters.Access {
	access := clusters.Access{}
	if reqAccess == nil {
		return access
	}
	if reqAccess.Ipv4CidrBlocks != nil {
		var blocks []clusters.CidrBlock
		if reqAccess.Ipv4CidrBlocks.GetValues() != nil {
			for _, cidrBlock := range reqAccess.Ipv4CidrBlocks.Values {
				blocks = append(blocks, clusters.CidrBlock{Value: cidrBlock.Value, Description: cidrBlock.Description})
			}
		}
		access.Ipv4CidrBlocks = blocks
	}

	if reqAccess.Ipv6CidrBlocks != nil {
		var blocks []clusters.CidrBlock
		if reqAccess.Ipv6CidrBlocks.GetValues() != nil {
			for _, cidrBlock := range reqAccess.Ipv6CidrBlocks.Values {
				blocks = append(blocks, clusters.CidrBlock{Value: cidrBlock.Value, Description: cidrBlock.Description})
			}
		}
		access.Ipv6CidrBlocks = blocks
	}
	if reqAccess.DataServices != nil {
		access.DataTransfer = optional.NewBool(false)
		for _, ds := range reqAccess.DataServices.GetValues() {
			switch ds {
			case datacloudv1.Access_DATA_SERVICE_TRANSFER:
				access.DataTransfer = optional.NewBool(true)
			}
		}
	}
	return access
}

func dataCloudEncryptionFromGRPC(reqEncryption *datacloudv1.DataEncryption) clusters.Encryption {
	encryption := clusters.Encryption{}
	if reqEncryption == nil {
		return encryption
	}

	encryption.Enabled = optional.NewBool(reqEncryption.GetEnabled().GetValue())
	return encryption
}

func dataCloudEncryptionToGRPC(encryption clusters.Encryption) *datacloudv1.DataEncryption {
	grpcDataCloudEncryption := datacloudv1.DataEncryption{
		Enabled: grpc.OptionalBoolToGRPC(encryption.Enabled),
	}

	if encryption.Enabled.Valid {
		return &grpcDataCloudEncryption
	}

	return nil
}

func VersionToGRPC(version *kfmodels.KafkaVersion) *kfv1.Version {
	return &kfv1.Version{
		Id:          version.Name,
		Name:        version.Name,
		Deprecated:  version.Deprecated,
		UpdatableTo: version.UpdatableTo,
	}
}

func VersionsToGRPC() []*kfv1.Version {
	versions := kfmodels.VersionsVisibleInConsoleDC
	v := make([]*kfv1.Version, 0, len(versions))
	for _, version := range versions {
		v = append(v, VersionToGRPC(version))
	}
	return v
}
