package kafka

import (
	"context"
	"encoding/json"
	"sort"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	kfv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1/console"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

// ClusterService implements DB-specific gRPC methods
type ConsoleClusterService struct {
	kfv1console.UnimplementedClusterServiceServer

	cnsl          console.Console
	kf            kafka.Kafka
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ kfv1console.ClusterServiceServer = &ConsoleClusterService{}

func NewConsoleClusterService(cnsl console.Console, kf kafka.Kafka, saltEnvsCfg logic.SaltEnvsConfig) *ConsoleClusterService {
	return &ConsoleClusterService{
		cnsl: cnsl,
		kf:   kf,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(kfv1.Cluster_PRODUCTION),
			int64(kfv1.Cluster_PRESTABLE),
			int64(kfv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func resourcePresetsToGRPC(aggregatedPresets map[string]map[string][]consolemodels.ResourcePreset) ([]*kfv1console.KafkaClustersConfig_ResourcePreset, error) {
	var lastRecord *consolemodels.ResourcePreset
	resourcesGRPC := make([]*kfv1console.KafkaClustersConfig_ResourcePreset, 0, len(aggregatedPresets))
	for _, presetAgg := range aggregatedPresets {
		zonesGRPC := make([]*kfv1console.KafkaClustersConfig_ResourcePreset_Zone, 0, len(presetAgg))
		for _, zoneAgg := range presetAgg {
			diskTypesGRPC := make([]*kfv1console.KafkaClustersConfig_ResourcePreset_Zone_DiskType, 0, len(zoneAgg))
			for _, record := range zoneAgg {
				diskTypeGRPC := &kfv1console.KafkaClustersConfig_ResourcePreset_Zone_DiskType{
					DiskTypeId: record.DiskTypeExtID,
					MaxHosts:   record.MaxHosts,
					MinHosts:   record.MinHosts,
				}
				// Providing non empty disk size field
				if record.DiskSizes == nil || len(record.DiskSizes) == 0 {
					diskTypeGRPC.DiskSize = &kfv1console.KafkaClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange_{
						DiskSizeRange: &kfv1console.KafkaClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange{
							Min: record.DiskSizeRange.Lower,
							Max: record.DiskSizeRange.Upper,
						},
					}
				} else {
					diskTypeGRPC.DiskSize = &kfv1console.KafkaClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes_{
						DiskSizes: &kfv1console.KafkaClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes{
							Sizes: record.DiskSizes,
						},
					}
				}
				diskTypesGRPC = append(diskTypesGRPC, diskTypeGRPC)
				lastRecord = &record
			}
			// Using last recod to fill zone info
			zoneGRPC := kfv1console.KafkaClustersConfig_ResourcePreset_Zone{
				ZoneId:    lastRecord.Zone,
				DiskTypes: diskTypesGRPC,
			}
			zonesGRPC = append(zonesGRPC, &zoneGRPC)
		}
		sort.Slice(zonesGRPC, func(i, j int) bool {
			zoneA := zonesGRPC[i]
			zoneB := zonesGRPC[j]
			return zoneA.ZoneId < zoneB.ZoneId
		})
		// Using last recod to fill preset info
		presetGRPC := &kfv1console.KafkaClustersConfig_ResourcePreset{
			PresetId:       lastRecord.ExtID,
			Type:           lastRecord.FlavorType,
			CpuLimit:       lastRecord.CPULimit,
			CpuFraction:    lastRecord.CPUFraction,
			MemoryLimit:    lastRecord.MemoryLimit,
			Zones:          zonesGRPC,
			Generation:     lastRecord.Generation,
			GenerationName: lastRecord.GenerationName,
			Decomissioning: lastRecord.Decommissioned,
		}
		resourcesGRPC = append(resourcesGRPC, presetGRPC)
	}
	return resourcesGRPC, nil
}

func (ccs *ConsoleClusterService) Get(ctx context.Context, cfgReq *kfv1console.GetKafkaClustersConfigRequest) (*kfv1console.KafkaClustersConfig, error) {
	resourcePresets, err := ccs.cnsl.GetResourcePresetsByClusterType(ctx, clusters.TypeKafka, cfgReq.FolderId, true)
	if err != nil {
		return nil, err
	}
	kakfaDefaultResources, err := ccs.cnsl.GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleKafka)
	if err != nil {
		return nil, err
	}
	kafkaHostTypeGRPC, err := HostRoleToGRPC(hosts.RoleKafka)
	if err != nil {
		return nil, err
	}

	zkDefaultResources, err := ccs.cnsl.GetDefaultResourcesByClusterType(clusters.TypeKafka, hosts.RoleZooKeeper)
	if err != nil {
		return nil, err
	}
	zkHostTypeGRPC, err := HostRoleToGRPC(hosts.RoleZooKeeper)
	if err != nil {
		return nil, err
	}

	availableVersions := make([]*kfv1console.KafkaClustersConfig_VersionInfo, 0, len(kfmodels.VersionsVisibleInConsoleMDB))
	for _, ver := range kfmodels.VersionsVisibleInConsoleMDB {
		availableVersions = append(availableVersions, &kfv1console.KafkaClustersConfig_VersionInfo{
			Id:          ver.Name,
			Name:        ver.Name,
			Deprecated:  ver.Deprecated,
			UpdatableTo: ver.UpdatableTo,
		})
	}

	var kafkaPresetsGRPC []*kfv1console.KafkaClustersConfig_ResourcePreset
	var zkPresetsGRPC []*kfv1console.KafkaClustersConfig_ResourcePreset
	roleAggregated := common.AggregateResourcePresets(resourcePresets)
	for role, rolePresets := range roleAggregated {
		if role == hosts.RoleKafka {
			kafkaPresetsGRPC, err = resourcePresetsToGRPC(rolePresets)
			if err != nil {
				return nil, err
			}
		}
		if role == hosts.RoleZooKeeper {
			zkPresetsGRPC, err = resourcePresetsToGRPC(rolePresets)
			if err != nil {
				return nil, err
			}
		}
	}

	return &kfv1console.KafkaClustersConfig{
		ClusterName: &kfv1console.NameValidator{
			Regexp: models.DefaultClusterNamePattern,
			Min:    models.DefaultClusterNameMinLen,
			Max:    models.DefaultClusterNameMaxLen,
		},
		TopicName: &kfv1console.NameValidator{
			Regexp: models.DefaultTopicNamePattern,
			Min:    models.DefaultTopicNameMinLen,
			Max:    models.DefaultTopicNameMaxLen,
		},
		UserName: &kfv1console.NameValidator{
			// TODO: validate actually, move pattern to variable
			Regexp: "(?!mdb_)[a-zA-Z0-9_-]+",
			Min:    1,
			Max:    256,
		},
		Password: &kfv1console.NameValidator{
			Regexp: models.DefaultUserPasswordPattern,
			Min:    models.DefaultUserPasswordMinLen,
			Max:    models.DefaultUserPasswordMaxLen,
		},
		BrokerCountLimits: &kfv1console.KafkaClustersConfig_BrokerCountLimits{
			MinBrokerCount: 1,
			MaxBrokerCount: 32,
		},
		ResourcePresets:   kafkaPresetsGRPC,
		DefaultResources:  defaultResourcesToGRPC(kakfaDefaultResources),
		Versions:          kfmodels.NamesOfVersionsVisibleInConsoleMDB,
		AvailableVersions: availableVersions,
		DefaultVersion:    kfmodels.DefaultVersion,
		HostTypes: []*kfv1console.KafkaClustersConfig_HostType{
			{
				Type:             kafkaHostTypeGRPC,
				ResourcePresets:  kafkaPresetsGRPC,
				DefaultResources: defaultResourcesToGRPC(kakfaDefaultResources),
			},
			{
				Type:             zkHostTypeGRPC,
				ResourcePresets:  zkPresetsGRPC,
				DefaultResources: defaultResourcesToGRPC(zkDefaultResources),
			},
		},
	}, nil
}

func (ccs *ConsoleClusterService) EstimateCreate(ctx context.Context, req *kfv1.CreateClusterRequest) (*kfv1console.BillingEstimateResponse, error) {
	env, err := ccs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	configSpec, err := configSpecFromGRPC(req.GetConfigSpec())
	if err != nil {
		return nil, err
	}
	estimate, err := ccs.kf.EstimateCreateCluster(
		ctx,
		kafka.CreateMDBClusterArgs{
			FolderExtID:      req.GetFolderId(),
			Name:             req.GetName(),
			Description:      req.GetDescription(),
			Labels:           req.GetLabels(),
			Environment:      env,
			UserSpecs:        UserSpecsFromGRPC(req.GetUserSpecs()),
			TopicSpecs:       TopicSpecsFromGRPC(req.GetTopicSpecs()),
			NetworkID:        req.GetNetworkId(),
			SubnetID:         req.GetSubnetId(),
			SecurityGroupIDs: req.GetSecurityGroupIds(),
			HostGroupIDs:     req.GetHostGroupIds(),
			ConfigSpec:       configSpec,
		})
	if err != nil {
		return nil, err
	}

	return billingEstimateToGRPC(estimate), nil
}

func billingEstimateToGRPC(estimate consolemodels.BillingEstimate) *kfv1console.BillingEstimateResponse {
	metrics := make([]*kfv1console.BillingMetric, 0, len(estimate.Metrics))
	for _, metric := range estimate.Metrics {
		tags := &kfv1console.BillingMetric_BillingTags{
			PublicIp:                        metric.Tags.PublicIP,
			DiskTypeId:                      metric.Tags.DiskTypeID,
			ClusterType:                     metric.Tags.ClusterType,
			DiskSize:                        metric.Tags.DiskSize,
			ResourcePresetId:                metric.Tags.ResourcePresetID,
			PlatformId:                      metric.Tags.PlatformID,
			Cores:                           metric.Tags.Cores,
			CoreFraction:                    metric.Tags.CoreFraction,
			Memory:                          metric.Tags.Memory,
			SoftwareAcceleratedNetworkCores: metric.Tags.SoftwareAcceleratedNetworkCores,
			Roles:                           metric.Tags.Roles,
			Online:                          metric.Tags.Online,
			OnDedicatedHost:                 metric.Tags.OnDedicatedHost,
		}

		metrics = append(metrics, &kfv1console.BillingMetric{
			FolderId: metric.FolderID,
			Schema:   metric.Schema,
			Tags:     tags,
		})
	}
	return &kfv1console.BillingEstimateResponse{
		Metrics: metrics,
	}
}

func (ccs *ConsoleClusterService) GetCreateConfig(_ context.Context, _ *kfv1console.CreateClusterConfigRequest) (*kfv1console.JSONSchema, error) {
	jsonschema := `{
	"ClusterCreate": {
		"type": "object",
		"properties": {
			"folderId": {
				"type": "string",
				"description": "Folder ID."
			},
			"description": {
				"type": "string",
				"minLength": 0,
				"maxLength": 256,
				"description": "Description."
			},
			"name": {
				"type": "string"
			},
			"labels": {
				"type": "object",
				"minLength": 0,
				"maxLength": 64,
				"description": "Labels."
			},
			"environment": {
				"type": "string",
				"enum": [
					"PRESTABLE",
					"PRODUCTION"
				]
			},
			"configSpec": {
				"$ref": "#/KafkaClusterConfigSpecSchemaV1"
			},
			"networkId": {
				"type": "string",
				"default": ""
			},
			"subnetId": {
				"type": "array",
				"items": {
					"type": "string"
				}
			},
			"securityGroupIds": {
				"type": "array",
				"items": {
					"type": "string"
				}
			}
		},
		"required": [
			"folderId",
			"name",
			"environment",
			"configSpec",
			"networkId"
		]
	},
	"KafkaClusterConfigSpecSchemaV1": {
		"type": "object",
		"properties": {
			"version": {
				"type": "string",
				"enum": [
					"2.1",
					"2.6",
					"2.8",
					"3.0",
				    "3.1"
				]
			},
			"kafka": {
				"$ref": "#/KafkaSchemaV1"
			},
			"zookeeper": {
				"$ref": "#/ZookeeperSchemaV1"
			},
			"zoneId": {
				"type": "array",
				"items": {
					"type": "string"
				}
			},
			"brokersCount": {
				"type": "integer",
				"format": "int16",
				"minimum": 1
			},
			"assignPublicIp": {
				"type": "boolean"
			}
		},
		"required": [
			"brokersCount"
		]
	},
	"KafkaSchemaV1": {
		"type": "object",
		"allOf": [
			{
				"oneOf": [
					{
						"properties": {
							"kafkaConfig_2_1": {"$ref": "#/Kafka21ConfigSchemaV1"}
						}
					},
					{
						"properties": {
							"kafkaConfig_2_6": {"$ref": "#/Kafka26ConfigSchemaV1"}
						}
					},
					{
						"properties": {
							"kafkaConfig_2_8": {"$ref": "#/Kafka28ConfigSchemaV1"}
						}
					},
					{
						"properties": {
							"kafkaConfig_3_0": {"$ref": "#/Kafka30ConfigSchemaV1"}
						}
					},
					{
						"properties": {
							"kafkaConfig_3_1": {"$ref": "#/Kafka31ConfigSchemaV1"}
						}
					},
					{
						"properties": {
							"kafkaConfig_3": {"$ref": "#/Kafka3ConfigSchemaV1"}
						}
					}
				]
			},
			{
				"properties": {
					"resources": {
						"$ref": "#/ResourcesSchemaV1"
					}
				}
			}
		],
		"required": [
			"resources"
		]
	},
	"Kafka21ConfigSchemaV1": {
		"type": "object",
		"properties": {
			"compressionType": {"$ref": "#/KafkaCompressionTypeSchemaV1"},
			"logFlushIntervalMessages": {
				"type": "integer",
				"format": "int16",
				"description": "The number of messages accumulated on a log partition before messages are flushed to disk."
			},
			"logFlushIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum time in ms that a message in any topic is kept in memory before flushed to disk."
			},
			"logFlushSchedulerIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The frequency in ms that the log flusher checks whether any log needs to be flushed to disk."
			},
			"logRetentionBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of the log before deleting it."
			},
			"logRetentionHours": {
				"type": "integer",
				"format": "int16",
				"description": "The number of hours to keep a log file before deleting it."
			},
			"logRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of minutes to keep a log file before deleting it."
			},
			"logRetentionMs": {
				"type": "integer",
				"format": "int16",
				"description": "The number of milliseconds to keep a log file before deleting it."
			},
			"logSegmentBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of a single log file."
			},
			"logPreallocate": {
				"type": "boolean",
				"description": "Should pre allocate file when create new segment?"
			},
			"socketSendBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_SNDBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"socketReceiveBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_RCVBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"autoCreateTopicsEnable": {
				"type": "boolean",
				"description": "Enable auto creation of topic on the server."
			},
			"numPartitions": {
				"type": "integer",
				"format": "int16",
				"description": "The default number of partitions per topic on the cluster."
			},
			"defaultReplicationFactor": {
				"type": "integer",
				"format": "int16",
				"description": "The default replication factor for topic on the cluster."
			},
			"messageMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The largest record batch size allowed by Kafka. Default value: 1048588. Must be <= then replica.fetch.max.bytes."
			},
			"replicaFetchMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of bytes of messages to attempt to fetch for each partition. Default value: 1048576. Must be >= then message.max.bytes."
			},
			"sslCipherSuites": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"description": "A list of ssl cipher suites."
			},
			"offsetsRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "Offset storage time after a consumer group loses all its consumers. Default: 10080."
			}
		}
	},
	"Kafka26ConfigSchemaV1": {
		"type": "object",
		"properties": {
			"compressionType": {"$ref": "#/KafkaCompressionTypeSchemaV1"},
			"logFlushIntervalMessages": {
				"type": "integer",
				"format": "int16",
				"description": "The number of messages accumulated on a log partition before messages are flushed to disk."
			},
			"logFlushIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum time in ms that a message in any topic is kept in memory before flushed to disk."
			},
			"logFlushSchedulerIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The frequency in ms that the log flusher checks whether any log needs to be flushed to disk."
			},
			"logRetentionBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of the log before deleting it."
			},
			"logRetentionHours": {
				"type": "integer",
				"format": "int16",
				"description": "The number of hours to keep a log file before deleting it."
			},
			"logRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of minutes to keep a log file before deleting it."
			},
			"logRetentionMs": {
				"type": "integer",
				"format": "int16",
				"description": "The number of milliseconds to keep a log file before deleting it."
			},
			"logSegmentBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of a single log file."
			},
			"logPreallocate": {
				"type": "boolean",
				"description": "Should pre allocate file when create new segment?"
			},
			"socketSendBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_SNDBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"socketReceiveBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_RCVBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"autoCreateTopicsEnable": {
				"type": "boolean",
				"description": "Enable auto creation of topic on the server"
			},
			"numPartitions": {
				"type": "integer",
				"format": "int16",
				"description": "The default number of partitions per topic on the cluster."
			},
			"defaultReplicationFactor": {
				"type": "integer",
				"format": "int16",
				"description": "The default replication factor for topic on the cluster."
			},
			"messageMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The largest record batch size allowed by Kafka. Default value: 1048588. Must be <= then replica.fetch.max.bytes."
			},
			"replicaFetchMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of bytes of messages to attempt to fetch for each partition. Default value: 1048576. Must be >= then message.max.bytes."
			},
			"sslCipherSuites": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"description": "A list of ssl cipher suites."
			},
			"offsetsRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "Offset storage time after a consumer group loses all its consumers. Default: 10080."
			}
		}
	},
	"Kafka28ConfigSchemaV1": {
		"type": "object",
		"properties": {
			"compressionType": {"$ref": "#/KafkaCompressionTypeSchemaV1"},
			"logFlushIntervalMessages": {
				"type": "integer",
				"format": "int16",
				"description": "The number of messages accumulated on a log partition before messages are flushed to disk."
			},
			"logFlushIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum time in ms that a message in any topic is kept in memory before flushed to disk."
			},
			"logFlushSchedulerIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The frequency in ms that the log flusher checks whether any log needs to be flushed to disk."
			},
			"logRetentionBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of the log before deleting it."
			},
			"logRetentionHours": {
				"type": "integer",
				"format": "int16",
				"description": "The number of hours to keep a log file before deleting it."
			},
			"logRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of minutes to keep a log file before deleting it."
			},
			"logRetentionMs": {
				"type": "integer",
				"format": "int16",
				"description": "The number of milliseconds to keep a log file before deleting it."
			},
			"logSegmentBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of a single log file."
			},
			"logPreallocate": {
				"type": "boolean",
				"description": "Should pre allocate file when create new segment?"
			},
			"socketSendBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_SNDBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"socketReceiveBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_RCVBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"autoCreateTopicsEnable": {
				"type": "boolean",
				"description": "Enable auto creation of topic on the server"
			},
			"numPartitions": {
				"type": "integer",
				"format": "int16",
				"description": "The default number of partitions per topic on the cluster."
			},
			"defaultReplicationFactor": {
				"type": "integer",
				"format": "int16",
				"description": "The default replication factor for topic on the cluster."
			},
			"messageMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The largest record batch size allowed by Kafka. Default value: 1048588. Must be <= then replica.fetch.max.bytes."
			},
			"replicaFetchMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of bytes of messages to attempt to fetch for each partition. Default value: 1048576. Must be >= then message.max.bytes."
			},
			"sslCipherSuites": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"description": "A list of ssl cipher suites."
			},
			"offsetsRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "Offset storage time after a consumer group loses all its consumers. Default: 10080."
			}
		}
	},
	"Kafka30ConfigSchemaV1": {
		"type": "object",
		"properties": {
			"compressionType": {"$ref": "#/KafkaCompressionTypeSchemaV1"},
			"logFlushIntervalMessages": {
				"type": "integer",
				"format": "int16",
				"description": "The number of messages accumulated on a log partition before messages are flushed to disk."
			},
			"logFlushIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum time in ms that a message in any topic is kept in memory before flushed to disk."
			},
			"logFlushSchedulerIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The frequency in ms that the log flusher checks whether any log needs to be flushed to disk."
			},
			"logRetentionBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of the log before deleting it."
			},
			"logRetentionHours": {
				"type": "integer",
				"format": "int16",
				"description": "The number of hours to keep a log file before deleting it."
			},
			"logRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of minutes to keep a log file before deleting it."
			},
			"logRetentionMs": {
				"type": "integer",
				"format": "int16",
				"description": "The number of milliseconds to keep a log file before deleting it."
			},
			"logSegmentBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of a single log file."
			},
			"logPreallocate": {
				"type": "boolean",
				"description": "Should pre allocate file when create new segment?"
			},
			"socketSendBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_SNDBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"socketReceiveBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_RCVBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"autoCreateTopicsEnable": {
				"type": "boolean",
				"description": "Enable auto creation of topic on the server"
			},
			"numPartitions": {
				"type": "integer",
				"format": "int16",
				"description": "The default number of partitions per topic on the cluster."
			},
			"defaultReplicationFactor": {
				"type": "integer",
				"format": "int16",
				"description": "The default replication factor for topic on the cluster."
			},
			"messageMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The largest record batch size allowed by Kafka. Default value: 1048588. Must be <= then replica.fetch.max.bytes."
			},
			"replicaFetchMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of bytes of messages to attempt to fetch for each partition. Default value: 1048576. Must be >= then message.max.bytes."
			},
			"sslCipherSuites": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"description": "A list of ssl cipher suites."
			},
			"offsetsRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "Offset storage time after a consumer group loses all its consumers. Default: 10080."
			}
		}
	},
	"Kafka31ConfigSchemaV1": {
		"type": "object",
		"properties": {
			"compressionType": {"$ref": "#/KafkaCompressionTypeSchemaV1"},
			"logFlushIntervalMessages": {
				"type": "integer",
				"format": "int16",
				"description": "The number of messages accumulated on a log partition before messages are flushed to disk."
			},
			"logFlushIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum time in ms that a message in any topic is kept in memory before flushed to disk."
			},
			"logFlushSchedulerIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The frequency in ms that the log flusher checks whether any log needs to be flushed to disk."
			},
			"logRetentionBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of the log before deleting it."
			},
			"logRetentionHours": {
				"type": "integer",
				"format": "int16",
				"description": "The number of hours to keep a log file before deleting it."
			},
			"logRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of minutes to keep a log file before deleting it."
			},
			"logRetentionMs": {
				"type": "integer",
				"format": "int16",
				"description": "The number of milliseconds to keep a log file before deleting it."
			},
			"logSegmentBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of a single log file."
			},
			"logPreallocate": {
				"type": "boolean",
				"description": "Should pre allocate file when create new segment?"
			},
			"socketSendBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_SNDBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"socketReceiveBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_RCVBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"autoCreateTopicsEnable": {
				"type": "boolean",
				"description": "Enable auto creation of topic on the server"
			},
			"numPartitions": {
				"type": "integer",
				"format": "int16",
				"description": "The default number of partitions per topic on the cluster."
			},
			"defaultReplicationFactor": {
				"type": "integer",
				"format": "int16",
				"description": "The default replication factor for topic on the cluster."
			},
			"messageMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The largest record batch size allowed by Kafka. Default value: 1048588. Must be <= then replica.fetch.max.bytes."
			},
			"replicaFetchMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of bytes of messages to attempt to fetch for each partition. Default value: 1048576. Must be >= then message.max.bytes."
			},
			"sslCipherSuites": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"description": "A list of ssl cipher suites."
			},
			"offsetsRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "Offset storage time after a consumer group loses all its consumers. Default: 10080."
			}
		}
	},
	"Kafka3ConfigSchemaV1": {
		"type": "object",
		"properties": {
			"compressionType": {"$ref": "#/KafkaCompressionTypeSchemaV1"},
			"logFlushIntervalMessages": {
				"type": "integer",
				"format": "int16",
				"description": "The number of messages accumulated on a log partition before messages are flushed to disk."
			},
			"logFlushIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum time in ms that a message in any topic is kept in memory before flushed to disk."
			},
			"logFlushSchedulerIntervalMs": {
				"type": "integer",
				"format": "int16",
				"description": "The frequency in ms that the log flusher checks whether any log needs to be flushed to disk."
			},
			"logRetentionBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of the log before deleting it."
			},
			"logRetentionHours": {
				"type": "integer",
				"format": "int16",
				"description": "The number of hours to keep a log file before deleting it."
			},
			"logRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of minutes to keep a log file before deleting it."
			},
			"logRetentionMs": {
				"type": "integer",
				"format": "int16",
				"description": "The number of milliseconds to keep a log file before deleting it."
			},
			"logSegmentBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The maximum size of a single log file."
			},
			"logPreallocate": {
				"type": "boolean",
				"description": "Should pre allocate file when create new segment?"
			},
			"socketSendBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_SNDBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"socketReceiveBufferBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The SO_RCVBUF buffer of the socket server sockets. If the value is -1, the OS default will be used."
			},
			"autoCreateTopicsEnable": {
				"type": "boolean",
				"description": "Enable auto creation of topic on the server"
			},
			"numPartitions": {
				"type": "integer",
				"format": "int16",
				"description": "The default number of partitions per topic on the cluster."
			},
			"defaultReplicationFactor": {
				"type": "integer",
				"format": "int16",
				"description": "The default replication factor for topic on the cluster."
			},
			"messageMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The largest record batch size allowed by Kafka. Default value: 1048588. Must be <= then replica.fetch.max.bytes."
			},
			"replicaFetchMaxBytes": {
				"type": "integer",
				"format": "int16",
				"description": "The number of bytes of messages to attempt to fetch for each partition. Default value: 1048576. Must be >= then message.max.bytes."
			},
			"sslCipherSuites": {
				"type": "array",
				"items": {
					"type": "string"
				},
				"description": "A list of ssl cipher suites."
			},
			"offsetsRetentionMinutes": {
				"type": "integer",
				"format": "int16",
				"description": "Offset storage time after a consumer group loses all its consumers. Default: 10080."
			}
		}
	},
	"KafkaCompressionTypeSchemaV1": {
		"type": "string",
		"enum": [
			"COMPRESSION_TYPE_UNCOMPRESSED",
			"COMPRESSION_TYPE_ZSTD",
			"COMPRESSION_TYPE_LZ4",
			"COMPRESSION_TYPE_SNAPPY",
			"COMPRESSION_TYPE_GZIP",
			"COMPRESSION_TYPE_PRODUCER"
		]
	},
	"ZookeeperSchemaV1": {
		"type": "object",
		"properties": {
			"resources": {
				"$ref": "#/ResourcesSchemaV1"
			}
		},
		"required": [
			"resources"
		]
	},
	"ResourcesSchemaV1": {
		"type": "object",
		"properties": {
			"diskSize": {
				"type": "integer",
				"format": "int32"
			},
			"resourcePresetId": {
				"type": "string"
			},
			"diskTypeId": {
				"type": "string"
			}
		},
		"required": [
			"diskSize",
			"diskTypeId",
			"resourcePresetId"
		]
	}
 }`
	m := make(map[string]interface{})
	err := json.Unmarshal([]byte(jsonschema), &m)
	if err != nil {
		return nil, err
	}
	b, err := json.Marshal(m)
	if err != nil {
		return nil, err
	}
	return &kfv1console.JSONSchema{Schema: string(b)}, nil
}
