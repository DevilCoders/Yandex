package kafka

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"
	"google.golang.org/genproto/protobuf/field_mask"
	"google.golang.org/protobuf/types/known/fieldmaskpb"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/defaults"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
)

func TestDoNotUpdateFieldsThatAreNotIncludedInUpdateMask(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"name",
			},
		},
		Description: "new-description",
		Name:        "new-name",
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.True(t, args.Name.Valid)
	require.False(t, args.Description.Valid)
}

func TestSetFieldToDefaultValue(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"description",
				"labels",
				"config_spec.kafka.kafka_config_2_6.log_flush_interval_messages",
			},
		},
		Description: "",
		Labels:      map[string]string{},
		ConfigSpec: &kfv1.ConfigSpec{
			Kafka: &kfv1.ConfigSpec_Kafka{
				KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_2_6{
					KafkaConfig_2_6: &kfv1.KafkaConfig2_6{
						LogFlushIntervalMessages: nil,
					}},
			},
		},
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.True(t, args.Description.Valid)
	require.Equal(t, "", args.Description.String)
	require.True(t, args.Labels.Valid)
	require.Equal(t, optional.LabelsType{}, args.Labels.Labels)
	require.True(t, args.ConfigSpec.Kafka.Config.LogFlushIntervalMessages.Valid)
	require.Nil(t, args.ConfigSpec.Kafka.Config.LogFlushIntervalMessages.Int64)
}

func TestReturnErrorIfFieldPathIsUnmappable(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"name",
				"absent_field",
				"config_spec.unknown_field",
			},
		},
		Name: "new-name",
	}

	_, err := modifyClusterArgsFromGRPC(request)
	require.Error(t, err)
	semErr := semerr.AsSemanticError(err)
	require.NotNil(t, semErr)
	require.Equal(t, semerr.SemanticInvalidInput, semErr.Semantic)
	require.Equal(t, "unknown field paths: absent_field, config_spec.unknown_field", err.Error())
}

func TestUpdateMetaInfo(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"description",
				"labels",
				"name",
			},
		},
		Description: "new-description",
		Labels: map[string]string{
			"key1": "val1",
			"key2": "val2",
		},
		Name: "new-name",
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.Equal(t, "new-description", args.Description.String)
	require.Equal(t, optional.LabelsType{
		"key1": "val1",
		"key2": "val2",
	}, args.Labels.Labels)
	require.Equal(t, "new-name", args.Name.String)
}

func TestKafkaConfigToGRPC(t *testing.T) {
	t.Run("When empty config", func(t *testing.T) {
		mdbClusterSpec := kfmodels.MDBClusterSpec{}
		kafkaSpec := &kfv1.ConfigSpec_Kafka{}
		KafkaConfigToGRPC(mdbClusterSpec, kafkaSpec)
		require.Equal(t, &kfv1.ConfigSpec_Kafka{}, kafkaSpec)
	})

	t.Run("When contains sslCipherSuites", func(t *testing.T) {
		mdbClusterSpec := kfmodels.MDBClusterSpec{
			Version: "3.1",
			Kafka: kfmodels.KafkaConfigSpec{
				Config: kfmodels.KafkaConfig{
					SslCipherSuites: []string{
						"TLS_RSA_WITH_AES_128_CBC_SHA",
						"TLS_RSA_WITH_AES_256_GCM_SHA384",
					},
				},
			},
		}
		kafkaSpec := &kfv1.ConfigSpec_Kafka{}
		KafkaConfigToGRPC(mdbClusterSpec, kafkaSpec)
		expected := &kfv1.ConfigSpec_Kafka{
			KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3_1{
				KafkaConfig_3_1: &kfv1.KafkaConfig3_1{
					SslCipherSuites: []string{
						"TLS_RSA_WITH_AES_128_CBC_SHA",
						"TLS_RSA_WITH_AES_256_GCM_SHA384",
					},
				},
			},
		}
		require.Equal(t, expected, kafkaSpec)
	})

	t.Run("When not contains sslCipherSuites", func(t *testing.T) {
		mdbClusterSpec := kfmodels.MDBClusterSpec{
			Version: "3.1",
			Kafka: kfmodels.KafkaConfigSpec{
				Config: kfmodels.KafkaConfig{
					LogSegmentBytes:   helpers.Pointer[int64](int64(100)),
					LogRetentionBytes: helpers.Pointer[int64](int64(100)),
				},
			},
		}
		kafkaSpec := &kfv1.ConfigSpec_Kafka{}
		KafkaConfigToGRPC(mdbClusterSpec, kafkaSpec)
		expected := &kfv1.ConfigSpec_Kafka{
			KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3_1{
				KafkaConfig_3_1: &kfv1.KafkaConfig3_1{
					LogSegmentBytes:   api.WrapInt64Pointer(helpers.Pointer[int64](int64(100))),
					LogRetentionBytes: api.WrapInt64Pointer(helpers.Pointer[int64](int64(100))),
				},
			},
		}
		require.Equal(t, expected, kafkaSpec)
	})

	t.Run("When use common kafka_3x config then correct.", func(t *testing.T) {
		mdbClusterSpec := kfmodels.MDBClusterSpec{
			Version: kfmodels.Version3_2,
			Kafka: kfmodels.KafkaConfigSpec{
				Config: kfmodels.KafkaConfig{
					LogSegmentBytes:   helpers.Pointer[int64](int64(100)),
					LogRetentionBytes: helpers.Pointer[int64](int64(100)),
				},
			},
		}
		kafkaSpec := &kfv1.ConfigSpec_Kafka{}
		KafkaConfigToGRPC(mdbClusterSpec, kafkaSpec)
		expected := &kfv1.ConfigSpec_Kafka{
			KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3{
				KafkaConfig_3: &kfv1.KafkaConfig3{
					LogSegmentBytes:   api.WrapInt64Pointer(helpers.Pointer[int64](int64(100))),
					LogRetentionBytes: api.WrapInt64Pointer(helpers.Pointer[int64](int64(100))),
				},
			},
		}
		require.Equal(t, expected, kafkaSpec)
	})
}

func TestUpdateConfigSpec(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"config_spec.version",
				"config_spec.zone_id",
				"config_spec.brokers_count",
				"config_spec.assign_public_ip",
			},
		},
		ConfigSpec: &kfv1.ConfigSpec{
			Version:        "2.1",
			ZoneId:         []string{"ru-central1-a", "ru-central1-b"},
			BrokersCount:   api.WrapInt64(2),
			AssignPublicIp: false,
		},
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.Equal(t, "2.1", args.ConfigSpec.Version.String)
	require.Equal(t, []string{"ru-central1-a", "ru-central1-b"}, args.ConfigSpec.ZoneID.Strings)
	require.Equal(t, int64(2), args.ConfigSpec.BrokersCount.Int64)
	require.True(t, args.ConfigSpec.AssignPublicIP.Valid)
	require.False(t, args.ConfigSpec.AssignPublicIP.Bool)
}

func TestUpdateKafkaResources(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"config_spec.kafka.resources.resource_preset_id",
				"config_spec.kafka.resources.disk_size",
				"config_spec.kafka.resources.disk_type_id",
			},
		},
		ConfigSpec: &kfv1.ConfigSpec{
			Kafka: &kfv1.ConfigSpec_Kafka{
				Resources: &kfv1.Resources{
					ResourcePresetId: "s2.medium",
					DiskSize:         1000,
					DiskTypeId:       "network-ssd",
				},
			},
		},
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.Equal(t, "s2.medium", args.ConfigSpec.Kafka.Resources.ResourcePresetExtID.String)
	require.Equal(t, int64(1000), args.ConfigSpec.Kafka.Resources.DiskSize.Int64)
	require.Equal(t, "network-ssd", args.ConfigSpec.Kafka.Resources.DiskTypeExtID.String)
}

func TestUpdateKafkaConfig_FullCorrect(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"config_spec.kafka.kafka_config_2_1.compression_type",
				"config_spec.kafka.kafka_config_2_1.log_flush_interval_messages",
				"config_spec.kafka.kafka_config_2_1.log_flush_interval_ms",
				"config_spec.kafka.kafka_config_2_1.log_flush_scheduler_interval_ms",
				"config_spec.kafka.kafka_config_2_1.log_retention_bytes",
				"config_spec.kafka.kafka_config_2_1.log_retention_hours",
				"config_spec.kafka.kafka_config_2_1.log_retention_minutes",
				"config_spec.kafka.kafka_config_2_1.log_retention_ms",
				"config_spec.kafka.kafka_config_2_1.message_max_bytes",
				"config_spec.kafka.kafka_config_2_1.replica_fetch_max_bytes",
				"config_spec.kafka.kafka_config_2_1.ssl_cipher_suites",
				"config_spec.kafka.kafka_config_2_1.offsets_retention_minutes",
			},
		},
		ConfigSpec: &kfv1.ConfigSpec{
			Kafka: &kfv1.ConfigSpec_Kafka{
				KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_2_1{
					KafkaConfig_2_1: &kfv1.KafkaConfig2_1{
						CompressionType:             kfv1.CompressionType_COMPRESSION_TYPE_SNAPPY,
						LogFlushIntervalMessages:    api.WrapInt64(1),
						LogFlushIntervalMs:          api.WrapInt64(2),
						LogFlushSchedulerIntervalMs: api.WrapInt64(3),
						LogRetentionBytes:           api.WrapInt64(4),
						LogRetentionHours:           api.WrapInt64(5),
						LogRetentionMinutes:         api.WrapInt64(6),
						LogRetentionMs:              api.WrapInt64(7),
						MessageMaxBytes:             api.WrapInt64(8),
						ReplicaFetchMaxBytes:        api.WrapInt64(2000000),
						SslCipherSuites:             []string{"TLS_AKE_WITH_AES_256_GCM_SHA384", "TLS_RSA_WITH_AES_256_GCM_SHA384"},
						OffsetsRetentionMinutes:     api.WrapInt64(30000),
					}},
			},
		},
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.Equal(t, "snappy", args.ConfigSpec.Kafka.Config.CompressionType.String)
	require.Equal(t, int64(1), *args.ConfigSpec.Kafka.Config.LogFlushIntervalMessages.Int64)
	require.Equal(t, int64(2), *args.ConfigSpec.Kafka.Config.LogFlushIntervalMs.Int64)
	require.Equal(t, int64(3), *args.ConfigSpec.Kafka.Config.LogFlushSchedulerIntervalMs.Int64)
	require.Equal(t, int64(4), *args.ConfigSpec.Kafka.Config.LogRetentionBytes.Int64)
	require.Equal(t, int64(5), *args.ConfigSpec.Kafka.Config.LogRetentionHours.Int64)
	require.Equal(t, int64(6), *args.ConfigSpec.Kafka.Config.LogRetentionMinutes.Int64)
	require.Equal(t, int64(7), *args.ConfigSpec.Kafka.Config.LogRetentionMs.Int64)
	require.Equal(t, int64(8), *args.ConfigSpec.Kafka.Config.MessageMaxBytes.Int64)
	require.Equal(t, int64(2000000), *args.ConfigSpec.Kafka.Config.ReplicaFetchMaxBytes.Int64)
	require.Equal(t, []string{"TLS_AKE_WITH_AES_256_GCM_SHA384", "TLS_RSA_WITH_AES_256_GCM_SHA384"}, args.ConfigSpec.Kafka.Config.SslCipherSuites.Strings)
	require.Equal(t, int64(30000), *args.ConfigSpec.Kafka.Config.OffsetsRetentionMinutes.Int64)
}

func TestUpdateKafkaConfigVersion3_FullCorrect(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"config_spec.kafka.kafka_config_3.compression_type",
				"config_spec.kafka.kafka_config_3.log_flush_interval_messages",
				"config_spec.kafka.kafka_config_3.log_flush_interval_ms",
				"config_spec.kafka.kafka_config_3.log_flush_scheduler_interval_ms",
				"config_spec.kafka.kafka_config_3.log_retention_bytes",
				"config_spec.kafka.kafka_config_3.log_retention_hours",
				"config_spec.kafka.kafka_config_3.log_retention_minutes",
				"config_spec.kafka.kafka_config_3.log_retention_ms",
				"config_spec.kafka.kafka_config_3.message_max_bytes",
				"config_spec.kafka.kafka_config_3.replica_fetch_max_bytes",
				"config_spec.kafka.kafka_config_3.ssl_cipher_suites",
				"config_spec.kafka.kafka_config_3.offsets_retention_minutes",
			},
		},
		ConfigSpec: &kfv1.ConfigSpec{
			Kafka: &kfv1.ConfigSpec_Kafka{
				KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3{
					KafkaConfig_3: &kfv1.KafkaConfig3{
						CompressionType:             kfv1.CompressionType_COMPRESSION_TYPE_SNAPPY,
						LogFlushIntervalMessages:    api.WrapInt64(1),
						LogFlushIntervalMs:          api.WrapInt64(2),
						LogFlushSchedulerIntervalMs: api.WrapInt64(3),
						LogRetentionBytes:           api.WrapInt64(4),
						LogRetentionHours:           api.WrapInt64(5),
						LogRetentionMinutes:         api.WrapInt64(6),
						LogRetentionMs:              api.WrapInt64(7),
						MessageMaxBytes:             api.WrapInt64(8),
						ReplicaFetchMaxBytes:        api.WrapInt64(2000000),
						SslCipherSuites:             []string{"TLS_AKE_WITH_AES_256_GCM_SHA384", "TLS_RSA_WITH_AES_256_GCM_SHA384"},
						OffsetsRetentionMinutes:     api.WrapInt64(30000),
					}},
			},
		},
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.Equal(t, "snappy", args.ConfigSpec.Kafka.Config.CompressionType.String)
	require.Equal(t, int64(1), *args.ConfigSpec.Kafka.Config.LogFlushIntervalMessages.Int64)
	require.Equal(t, int64(2), *args.ConfigSpec.Kafka.Config.LogFlushIntervalMs.Int64)
	require.Equal(t, int64(3), *args.ConfigSpec.Kafka.Config.LogFlushSchedulerIntervalMs.Int64)
	require.Equal(t, int64(4), *args.ConfigSpec.Kafka.Config.LogRetentionBytes.Int64)
	require.Equal(t, int64(5), *args.ConfigSpec.Kafka.Config.LogRetentionHours.Int64)
	require.Equal(t, int64(6), *args.ConfigSpec.Kafka.Config.LogRetentionMinutes.Int64)
	require.Equal(t, int64(7), *args.ConfigSpec.Kafka.Config.LogRetentionMs.Int64)
	require.Equal(t, int64(8), *args.ConfigSpec.Kafka.Config.MessageMaxBytes.Int64)
	require.Equal(t, int64(2000000), *args.ConfigSpec.Kafka.Config.ReplicaFetchMaxBytes.Int64)
	require.Equal(t, []string{"TLS_AKE_WITH_AES_256_GCM_SHA384", "TLS_RSA_WITH_AES_256_GCM_SHA384"}, args.ConfigSpec.Kafka.Config.SslCipherSuites.Strings)
	require.Equal(t, int64(30000), *args.ConfigSpec.Kafka.Config.OffsetsRetentionMinutes.Int64)
}

func TestUpdateKafkaConfig_InvalidSslCipherSuites(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"config_spec.kafka.kafka_config_2_1.compression_type",
				"config_spec.kafka.kafka_config_2_1.log_flush_interval_messages",
				"config_spec.kafka.kafka_config_2_1.log_flush_interval_ms",
				"config_spec.kafka.kafka_config_2_1.log_flush_scheduler_interval_ms",
				"config_spec.kafka.kafka_config_2_1.log_retention_bytes",
				"config_spec.kafka.kafka_config_2_1.log_retention_hours",
				"config_spec.kafka.kafka_config_2_1.log_retention_minutes",
				"config_spec.kafka.kafka_config_2_1.log_retention_ms",
				"config_spec.kafka.kafka_config_2_1.message_max_bytes",
				"config_spec.kafka.kafka_config_2_1.replica_fetch_max_bytes",
				"config_spec.kafka.kafka_config_2_1.ssl_cipher_suites",
			},
		},
		ConfigSpec: &kfv1.ConfigSpec{
			Kafka: &kfv1.ConfigSpec_Kafka{
				KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_2_1{
					KafkaConfig_2_1: &kfv1.KafkaConfig2_1{
						CompressionType:             kfv1.CompressionType_COMPRESSION_TYPE_SNAPPY,
						LogFlushIntervalMessages:    api.WrapInt64(1),
						LogFlushIntervalMs:          api.WrapInt64(2),
						LogFlushSchedulerIntervalMs: api.WrapInt64(3),
						LogRetentionBytes:           api.WrapInt64(4),
						LogRetentionHours:           api.WrapInt64(5),
						LogRetentionMinutes:         api.WrapInt64(6),
						LogRetentionMs:              api.WrapInt64(7),
						MessageMaxBytes:             api.WrapInt64(8),
						ReplicaFetchMaxBytes:        api.WrapInt64(2000000),
						SslCipherSuites:             []string{"TLS_AKE_WITH_AES_256_GCM_SHA3841"},
					}},
			},
		},
	}

	_, err := modifyClusterArgsFromGRPC(request)
	require.Error(t, err)
	require.Equal(t, fmt.Sprintf("these suites are invalid: [TLS_AKE_WITH_AES_256_GCM_SHA3841]. List of valid suites: [%s].", defaults.AllValidSslCipherSuitesSortedString), err.Error())
}

func TestKafkaConfigFromGRPC(t *testing.T) {
	configSpecKafka := &kfv1.ConfigSpec_Kafka{
		Resources: &kfv1.Resources{
			ResourcePresetId: "s2.medium",
			DiskSize:         1000,
			DiskTypeId:       "network-ssd",
		},
		KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3_1{
			KafkaConfig_3_1: &kfv1.KafkaConfig3_1{
				CompressionType:             kfv1.CompressionType_COMPRESSION_TYPE_SNAPPY,
				LogFlushIntervalMessages:    api.WrapInt64(1),
				LogFlushIntervalMs:          api.WrapInt64(2),
				LogFlushSchedulerIntervalMs: api.WrapInt64(3),
				LogRetentionBytes:           api.WrapInt64(4),
				LogRetentionHours:           api.WrapInt64(5),
				LogRetentionMinutes:         api.WrapInt64(6),
				LogRetentionMs:              api.WrapInt64(7),
				LogSegmentBytes:             api.WrapInt64(8),
				LogPreallocate:              api.WrapBool(true),
				SocketSendBufferBytes:       api.WrapInt64(9),
				SocketReceiveBufferBytes:    api.WrapInt64(10),
				AutoCreateTopicsEnable:      api.WrapBool(true),
				NumPartitions:               api.WrapInt64(11),
				DefaultReplicationFactor:    api.WrapInt64(12),
				MessageMaxBytes:             api.WrapInt64(13),
				ReplicaFetchMaxBytes:        api.WrapInt64(2000000),
				SslCipherSuites:             []string{"blank", "abc"},
				OffsetsRetentionMinutes:     api.WrapInt64(30000),
			}},
	}
	actual, version := KafkaConfigFromGRPC(configSpecKafka)
	expected := kfmodels.KafkaConfig{
		CompressionType:             "snappy",
		LogFlushIntervalMessages:    helpers.Pointer[int64](int64(1)),
		LogFlushIntervalMs:          helpers.Pointer[int64](int64(2)),
		LogFlushSchedulerIntervalMs: helpers.Pointer[int64](int64(3)),
		LogRetentionBytes:           helpers.Pointer[int64](int64(4)),
		LogRetentionHours:           helpers.Pointer[int64](int64(5)),
		LogRetentionMinutes:         helpers.Pointer[int64](int64(6)),
		LogRetentionMs:              helpers.Pointer[int64](int64(7)),
		LogSegmentBytes:             helpers.Pointer[int64](int64(8)),
		LogPreallocate:              helpers.Pointer[bool](true),
		SocketSendBufferBytes:       helpers.Pointer[int64](int64(9)),
		SocketReceiveBufferBytes:    helpers.Pointer[int64](int64(10)),
		AutoCreateTopicsEnable:      helpers.Pointer[bool](true),
		NumPartitions:               helpers.Pointer[int64](int64(11)),
		DefaultReplicationFactor:    helpers.Pointer[int64](int64(12)),
		MessageMaxBytes:             helpers.Pointer[int64](int64(13)),
		ReplicaFetchMaxBytes:        helpers.Pointer[int64](int64(2000000)),
		SslCipherSuites:             []string{"abc", "blank"},
		OffsetsRetentionMinutes:     helpers.Pointer[int64](int64(30000)),
	}
	require.Equal(t, expected, actual)
	require.Equal(t, "3.1", version)
}

func TestKafkaConfigFromGRPCWhenKafkaVersion3(t *testing.T) {
	configSpecKafka := &kfv1.ConfigSpec_Kafka{
		Resources: &kfv1.Resources{
			ResourcePresetId: "s2.medium",
			DiskSize:         1000,
			DiskTypeId:       "network-ssd",
		},
		KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3{
			KafkaConfig_3: &kfv1.KafkaConfig3{
				CompressionType:             kfv1.CompressionType_COMPRESSION_TYPE_SNAPPY,
				LogFlushIntervalMessages:    api.WrapInt64(1),
				LogFlushIntervalMs:          api.WrapInt64(2),
				LogFlushSchedulerIntervalMs: api.WrapInt64(3),
				LogRetentionBytes:           api.WrapInt64(4),
				LogRetentionHours:           api.WrapInt64(5),
				LogRetentionMinutes:         api.WrapInt64(6),
				LogRetentionMs:              api.WrapInt64(7),
				LogSegmentBytes:             api.WrapInt64(8),
				LogPreallocate:              api.WrapBool(true),
				SocketSendBufferBytes:       api.WrapInt64(9),
				SocketReceiveBufferBytes:    api.WrapInt64(10),
				AutoCreateTopicsEnable:      api.WrapBool(true),
				NumPartitions:               api.WrapInt64(11),
				DefaultReplicationFactor:    api.WrapInt64(12),
				MessageMaxBytes:             api.WrapInt64(13),
				ReplicaFetchMaxBytes:        api.WrapInt64(2000000),
				SslCipherSuites:             []string{"blank", "abc"},
				OffsetsRetentionMinutes:     api.WrapInt64(30000),
			}},
	}
	actual, version := KafkaConfigFromGRPC(configSpecKafka)
	expected := kfmodels.KafkaConfig{
		CompressionType:             "snappy",
		LogFlushIntervalMessages:    helpers.Pointer[int64](int64(1)),
		LogFlushIntervalMs:          helpers.Pointer[int64](int64(2)),
		LogFlushSchedulerIntervalMs: helpers.Pointer[int64](int64(3)),
		LogRetentionBytes:           helpers.Pointer[int64](int64(4)),
		LogRetentionHours:           helpers.Pointer[int64](int64(5)),
		LogRetentionMinutes:         helpers.Pointer[int64](int64(6)),
		LogRetentionMs:              helpers.Pointer[int64](int64(7)),
		LogSegmentBytes:             helpers.Pointer[int64](int64(8)),
		LogPreallocate:              helpers.Pointer[bool](true),
		SocketSendBufferBytes:       helpers.Pointer[int64](int64(9)),
		SocketReceiveBufferBytes:    helpers.Pointer[int64](int64(10)),
		AutoCreateTopicsEnable:      helpers.Pointer[bool](true),
		NumPartitions:               helpers.Pointer[int64](int64(11)),
		DefaultReplicationFactor:    helpers.Pointer[int64](int64(12)),
		MessageMaxBytes:             helpers.Pointer[int64](int64(13)),
		ReplicaFetchMaxBytes:        helpers.Pointer[int64](int64(2000000)),
		SslCipherSuites:             []string{"abc", "blank"},
		OffsetsRetentionMinutes:     helpers.Pointer[int64](int64(30000)),
	}
	require.Equal(t, expected, actual)
	require.Equal(t, "3", version)
}

func TestUpdateZookeeperResources(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"config_spec.zookeeper.resources.resource_preset_id",
				"config_spec.zookeeper.resources.disk_size",
				"config_spec.zookeeper.resources.disk_type_id",
			},
		},
		ConfigSpec: &kfv1.ConfigSpec{
			Zookeeper: &kfv1.ConfigSpec_Zookeeper{
				Resources: &kfv1.Resources{
					ResourcePresetId: "s2.medium",
					DiskSize:         1000,
					DiskTypeId:       "network-ssd",
				},
			},
		},
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.Equal(t, "s2.medium", args.ConfigSpec.ZooKeeper.Resources.ResourcePresetExtID.String)
	require.Equal(t, int64(1000), args.ConfigSpec.ZooKeeper.Resources.DiskSize.Int64)
	require.Equal(t, "network-ssd", args.ConfigSpec.ZooKeeper.Resources.DiskTypeExtID.String)
}

func TestUpdateSecurityGroupIDs(t *testing.T) {
	request := &kfv1.UpdateClusterRequest{
		ClusterId: "cluster-id",
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"security_group_ids",
			},
		},
		SecurityGroupIds: []string{"s1", "s2"},
	}

	args, err := modifyClusterArgsFromGRPC(request)
	require.NoError(t, err)
	require.True(t, args.SecurityGroupIDs.Valid)
	require.Equal(t, args.SecurityGroupIDs.Strings, []string{"s1", "s2"})
}

func TestUpdateClusterConnectionSpecFromGRPC_Correct_Alias(t *testing.T) {
	spec := &kfv1.ClusterConnectionSpec{
		Alias: "source",
	}
	paths := grpcapi.NewFieldPaths([]string{"alias"})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	expectedUpdateSpec.Alias.Set("source")
	require.True(t, hasChanges)
	require.Equal(t, updateSpec, expectedUpdateSpec)
}

func TestUpdateClusterConnectionSpecFromGRPC_Correct_ThisCluster(t *testing.T) {
	spec := &kfv1.ClusterConnectionSpec{
		ClusterConnection: &kfv1.ClusterConnectionSpec_ThisCluster{},
	}
	paths := grpcapi.NewFieldPaths([]string{
		"this_cluster",
	})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	expectedUpdateSpec.Type.Set(kfmodels.ClusterConnectionTypeThisCluster)
	require.True(t, hasChanges)
	require.Equal(t, updateSpec, expectedUpdateSpec)
}

func TestUpdateClusterConnectionSpecFromGRPC_Correct_ExternalCluster(t *testing.T) {
	spec := &kfv1.ClusterConnectionSpec{
		ClusterConnection: &kfv1.ClusterConnectionSpec_ExternalCluster{
			ExternalCluster: &kfv1.ExternalClusterConnectionSpec{
				BootstrapServers:          "bootstrap_servers",
				SecurityProtocol:          "SASL_SSL",
				SaslMechanism:             "SCRAM-SHA-512",
				SaslUsername:              "egor",
				SaslPassword:              "123",
				SslTruststoreCertificates: "123",
			},
		},
	}
	paths := grpcapi.NewFieldPaths([]string{
		"external_cluster.bootstrap_servers",
		"external_cluster.security_protocol",
		"external_cluster.sasl_mechanism",
		"external_cluster.sasl_username",
		"external_cluster.sasl_password",
		"external_cluster.ssl_truststore_certificates",
	})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	expectedUpdateSpec.Type.Set(kfmodels.ClusterConnectionTypeExternal)
	expectedUpdateSpec.BootstrapServers.Set("bootstrap_servers")
	expectedUpdateSpec.SecurityProtocol.Set("SASL_SSL")
	expectedUpdateSpec.SaslMechanism.Set("SCRAM-SHA-512")
	expectedUpdateSpec.SaslUsername.Set("egor")
	expectedUpdateSpec.SaslPasswordUpdated = true
	expectedUpdateSpec.SaslPassword = secret.NewString("123")
	expectedUpdateSpec.SslTruststoreCertificates.Set("123")
	require.True(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateClusterConnectionSpecFromGRPC_Correct_ExternalClusterWithoutPassword(t *testing.T) {
	spec := &kfv1.ClusterConnectionSpec{
		ClusterConnection: &kfv1.ClusterConnectionSpec_ExternalCluster{
			ExternalCluster: &kfv1.ExternalClusterConnectionSpec{
				BootstrapServers: "bootstrap_servers",
				SecurityProtocol: "SASL_SSL",
				SaslMechanism:    "SCRAM-SHA-512",
				SaslUsername:     "egor",
			},
		},
	}
	paths := grpcapi.NewFieldPaths([]string{
		"external_cluster.bootstrap_servers",
		"external_cluster.security_protocol",
		"external_cluster.sasl_mechanism",
		"external_cluster.sasl_username",
	})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	expectedUpdateSpec.Type.Set(kfmodels.ClusterConnectionTypeExternal)
	expectedUpdateSpec.BootstrapServers.Set("bootstrap_servers")
	expectedUpdateSpec.SecurityProtocol.Set("SASL_SSL")
	expectedUpdateSpec.SaslMechanism.Set("SCRAM-SHA-512")
	expectedUpdateSpec.SaslUsername.Set("egor")
	expectedUpdateSpec.SaslPasswordUpdated = false
	require.True(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateClusterConnectionSpecFromGRPC_Correct_ExternalClusterFullSpec(t *testing.T) {
	spec := &kfv1.ClusterConnectionSpec{
		Alias: "alias",
		ClusterConnection: &kfv1.ClusterConnectionSpec_ExternalCluster{
			ExternalCluster: &kfv1.ExternalClusterConnectionSpec{
				BootstrapServers:          "bootstrap_servers",
				SecurityProtocol:          "SASL_SSL",
				SaslMechanism:             "SCRAM-SHA-512",
				SaslUsername:              "egor",
				SaslPassword:              "123",
				SslTruststoreCertificates: "123",
			},
		},
	}
	paths := grpcapi.NewFieldPaths([]string{
		"alias",
		"external_cluster.bootstrap_servers",
		"external_cluster.security_protocol",
		"external_cluster.sasl_mechanism",
		"external_cluster.sasl_username",
		"external_cluster.sasl_password",
		"external_cluster.ssl_truststore_certificates",
	})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	expectedUpdateSpec.Alias.Set("alias")
	expectedUpdateSpec.Type.Set(kfmodels.ClusterConnectionTypeExternal)
	expectedUpdateSpec.BootstrapServers.Set("bootstrap_servers")
	expectedUpdateSpec.SecurityProtocol.Set("SASL_SSL")
	expectedUpdateSpec.SaslMechanism.Set("SCRAM-SHA-512")
	expectedUpdateSpec.SaslUsername.Set("egor")
	expectedUpdateSpec.SaslPasswordUpdated = true
	expectedUpdateSpec.SaslPassword = secret.NewString("123")
	expectedUpdateSpec.SslTruststoreCertificates.Set("123")
	require.True(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateClusterConnectionSpecFromGRPC_Correct_ThisClusterFullSpec(t *testing.T) {
	spec := &kfv1.ClusterConnectionSpec{
		Alias:             "alias",
		ClusterConnection: &kfv1.ClusterConnectionSpec_ThisCluster{},
	}
	paths := grpcapi.NewFieldPaths([]string{
		"alias",
		"this_cluster",
	})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	expectedUpdateSpec.Alias.Set("alias")
	expectedUpdateSpec.Type.Set(kfmodels.ClusterConnectionTypeThisCluster)
	require.True(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateClusterConnectionSpecFromGRPC_NilSpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(nil, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	require.False(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateS3ConnectionSpecFromGRPC_NilSpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	updateSpec, hasChanges := UpdateS3ConnectionSpecFromGRPC(nil, paths)
	expectedUpdateSpec := kfmodels.UpdateS3ConnectionSpec{}
	require.False(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateS3ConnectionSpecFromGRPC_EmptySpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	updateSpec, hasChanges := UpdateS3ConnectionSpecFromGRPC(&kfv1.S3ConnectionSpec{}, paths)
	expectedUpdateSpec := kfmodels.UpdateS3ConnectionSpec{}
	require.False(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateS3ConnectionSpecFromGRPC_FullSpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{
		"bucket_name",
		"external_s3.access_key_id",
		"external_s3.secret_access_key",
		"external_s3.endpoint",
		"external_s3.region",
	})
	spec := &kfv1.S3ConnectionSpec{
		BucketName: "test",
		Storage: &kfv1.S3ConnectionSpec_ExternalS3{
			ExternalS3: &kfv1.ExternalS3StorageSpec{
				AccessKeyId:     "test",
				SecretAccessKey: "test",
				Endpoint:        "test",
				Region:          "test",
			},
		},
	}
	actualUpdateSpec, hasChanges := UpdateS3ConnectionSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateS3ConnectionSpec{
		SecretAccessKeyUpdated: true,
		SecretAccessKey:        secret.NewString("test"),
	}
	expectedUpdateSpec.Type.Set(kfmodels.S3ConnectionTypeExternal)
	expectedUpdateSpec.AccessKeyID.Set("test")
	expectedUpdateSpec.BucketName.Set("test")
	expectedUpdateSpec.Endpoint.Set("test")
	expectedUpdateSpec.Region.Set("test")
	require.True(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, actualUpdateSpec)
}

func TestUpdateClusterConnectionSpecFromGRPC_EmptySpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	updateSpec, hasChanges := UpdateClusterConnectionSpecFromGRPC(&kfv1.ClusterConnectionSpec{}, paths)
	expectedUpdateSpec := kfmodels.UpdateClusterConnectionSpec{}
	require.False(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateMirrormakerConfigSpecFromGRPC_NilSpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	updateSpec, hasChanges := UpdateMirrormakerConfigSpecFromGRPC(nil, paths)
	expectedUpdateSpec := kfmodels.UpdateMirrormakerConfigSpec{}
	require.False(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateMirrormakerConfigSpecFromGRPC_EmptySpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	spec := &kfv1.ConnectorConfigMirrorMakerSpec{}
	updateSpec, hasChanges := UpdateMirrormakerConfigSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateMirrormakerConfigSpec{}
	require.False(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateMirrormakerConfigSpecFromGRPC_Correct_FullSpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{
		"topics",
		"replication_factor",
		"source_cluster.alias",
		"source_cluster.external_cluster.bootstrap_servers",
		"source_cluster.external_cluster.security_protocol",
		"source_cluster.external_cluster.sasl_mechanism",
		"source_cluster.external_cluster.sasl_username",
		"source_cluster.external_cluster.sasl_password",
		"source_cluster.external_cluster.ssl_truststore_certificates",
		"target_cluster.alias",
		"target_cluster.this_cluster",
	})
	spec := &kfv1.ConnectorConfigMirrorMakerSpec{
		Topics:            "topics",
		ReplicationFactor: api.WrapInt64(1),
		SourceCluster: &kfv1.ClusterConnectionSpec{
			Alias: "source",
			ClusterConnection: &kfv1.ClusterConnectionSpec_ExternalCluster{
				ExternalCluster: &kfv1.ExternalClusterConnectionSpec{
					BootstrapServers:          "bootstrap_servers",
					SecurityProtocol:          "SASL_SSL",
					SaslMechanism:             "SCRAM-SHA-512",
					SaslUsername:              "egor",
					SaslPassword:              "123",
					SslTruststoreCertificates: "123",
				},
			},
		},
		TargetCluster: &kfv1.ClusterConnectionSpec{
			Alias:             "target",
			ClusterConnection: &kfv1.ClusterConnectionSpec_ThisCluster{},
		},
	}
	updateSpec, hasChanges := UpdateMirrormakerConfigSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateMirrormakerConfigSpec{
		SourceCluster: kfmodels.UpdateClusterConnectionSpec{},
		TargetCluster: kfmodels.UpdateClusterConnectionSpec{},
	}
	expectedUpdateSpec.Topics.Set("topics")
	expectedUpdateSpec.ReplicationFactor.Set(1)
	expectedUpdateSpec.SourceCluster.Alias.Set("source")
	expectedUpdateSpec.SourceCluster.Type.Set(kfmodels.ClusterConnectionTypeExternal)
	expectedUpdateSpec.SourceCluster.BootstrapServers.Set("bootstrap_servers")
	expectedUpdateSpec.SourceCluster.SecurityProtocol.Set("SASL_SSL")
	expectedUpdateSpec.SourceCluster.SaslMechanism.Set("SCRAM-SHA-512")
	expectedUpdateSpec.SourceCluster.SaslUsername.Set("egor")
	expectedUpdateSpec.SourceCluster.SaslPasswordUpdated = true
	expectedUpdateSpec.SourceCluster.SaslPassword = secret.NewString("123")
	expectedUpdateSpec.SourceCluster.SslTruststoreCertificates.Set("123")
	expectedUpdateSpec.TargetCluster.Alias.Set("target")
	expectedUpdateSpec.TargetCluster.Type.Set(kfmodels.ClusterConnectionTypeThisCluster)
	require.True(t, hasChanges)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateConnectorSpecFromGRPC_NilSpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	updateSpec, err := UpdateConnectorSpecFromGRPC(nil, paths)
	expectedUpdateSpec := kfmodels.UpdateConnectorSpec{}
	require.Error(t, err)
	require.True(t, semerr.IsInvalidInput(err))
	require.Equal(t, "nil UpdateConnectorSpec", err.Error())
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateConnectorSpecFromGRPC_EmptySpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{})
	spec := &kfv1.UpdateConnectorSpec{}
	updateSpec, err := UpdateConnectorSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateConnectorSpec{}
	require.Error(t, err)
	require.True(t, semerr.IsInvalidInput(err))
	require.Equal(t, "no fields to change in update connector request", err.Error())
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateConnectorSpecFromGRPC_FullSpec(t *testing.T) {
	paths := grpcapi.NewFieldPaths([]string{
		"tasks_max",
		"properties.key_to_add",
		"properties.key_to_change",
		"properties.key_to_delete",
		"connector_config_mirrormaker.topics",
		"connector_config_mirrormaker.replication_factor",
		"connector_config_mirrormaker.source_cluster.alias",
		"connector_config_mirrormaker.source_cluster.external_cluster.bootstrap_servers",
		"connector_config_mirrormaker.source_cluster.external_cluster.security_protocol",
		"connector_config_mirrormaker.source_cluster.external_cluster.sasl_mechanism",
		"connector_config_mirrormaker.source_cluster.external_cluster.sasl_username",
		"connector_config_mirrormaker.source_cluster.external_cluster.sasl_password",
		"connector_config_mirrormaker.source_cluster.external_cluster.ssl_truststore_certificates",
		"connector_config_mirrormaker.target_cluster.alias",
		"connector_config_mirrormaker.target_cluster.this_cluster",
	})
	spec := &kfv1.UpdateConnectorSpec{
		TasksMax: api.WrapInt64(1),
		Properties: map[string]string{
			"key_to_add":    "value",
			"key_to_change": "value",
		},
		ConnectorConfig: &kfv1.UpdateConnectorSpec_ConnectorConfigMirrormaker{
			ConnectorConfigMirrormaker: &kfv1.ConnectorConfigMirrorMakerSpec{
				Topics:            "topics",
				ReplicationFactor: api.WrapInt64(1),
				SourceCluster: &kfv1.ClusterConnectionSpec{
					Alias: "source",
					ClusterConnection: &kfv1.ClusterConnectionSpec_ExternalCluster{
						ExternalCluster: &kfv1.ExternalClusterConnectionSpec{
							BootstrapServers:          "bootstrap_servers",
							SecurityProtocol:          "SASL_SSL",
							SaslMechanism:             "SCRAM-SHA-512",
							SaslUsername:              "egor",
							SaslPassword:              "123",
							SslTruststoreCertificates: "123",
						},
					},
				},
				TargetCluster: &kfv1.ClusterConnectionSpec{
					Alias:             "target",
					ClusterConnection: &kfv1.ClusterConnectionSpec_ThisCluster{},
				},
			},
		},
	}
	updateSpec, err := UpdateConnectorSpecFromGRPC(spec, paths)
	expectedUpdateSpec := kfmodels.UpdateConnectorSpec{
		Properties: map[string]string{
			"key_to_add":    "value",
			"key_to_change": "value",
		},
		PropertiesContains: map[string]bool{
			"key_to_add":    true,
			"key_to_change": true,
			"key_to_delete": false,
		},
		MirrormakerConfig: kfmodels.UpdateMirrormakerConfigSpec{
			SourceCluster: kfmodels.UpdateClusterConnectionSpec{},
			TargetCluster: kfmodels.UpdateClusterConnectionSpec{},
		},
	}
	expectedUpdateSpec.Type.Set(kfmodels.ConnectorTypeMirrormaker)
	expectedUpdateSpec.TasksMax.Set(1)
	expectedUpdateSpec.MirrormakerConfig.Topics.Set("topics")
	expectedUpdateSpec.MirrormakerConfig.ReplicationFactor.Set(1)
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.Alias.Set("source")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.Type.Set(kfmodels.ClusterConnectionTypeExternal)
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.BootstrapServers.Set("bootstrap_servers")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SecurityProtocol.Set("SASL_SSL")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslMechanism.Set("SCRAM-SHA-512")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslUsername.Set("egor")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslPasswordUpdated = true
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslPassword = secret.NewString("123")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SslTruststoreCertificates.Set("123")
	expectedUpdateSpec.MirrormakerConfig.TargetCluster.Alias.Set("target")
	expectedUpdateSpec.MirrormakerConfig.TargetCluster.Type.Set(kfmodels.ClusterConnectionTypeThisCluster)
	require.NoError(t, err)
	require.Equal(t, expectedUpdateSpec, updateSpec)
}

func TestUpdateConnectorArgsFromGRPC_NilRequest(t *testing.T) {
	updateArgs, err := UpdateConnectorArgsFromGRPC(nil)
	expectedUpdateArgs := kafka.UpdateConnectorArgs{}
	require.Error(t, err)
	require.True(t, semerr.IsInvalidInput(err))
	require.Equal(t, "nil UpdateConnectorRequest", err.Error())
	require.Equal(t, expectedUpdateArgs, updateArgs)
}

func TestUpdateConnectorArgsFromGRPC_FullUpdateRequest(t *testing.T) {
	request := &kfv1.UpdateConnectorRequest{
		ClusterId:     "cluster_id",
		ConnectorName: "connector_name",
		UpdateMask: &fieldmaskpb.FieldMask{
			Paths: []string{
				"cluster_id",
				"connector_name",
				"connector_spec.tasks_max",
				"connector_spec.properties.key_to_add",
				"connector_spec.properties.key_to_change",
				"connector_spec.properties.key_to_delete",
				"connector_spec.connector_config_mirrormaker.topics",
				"connector_spec.connector_config_mirrormaker.replication_factor",
				"connector_spec.connector_config_mirrormaker.source_cluster.alias",
				"connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.bootstrap_servers",
				"connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.security_protocol",
				"connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.sasl_mechanism",
				"connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.sasl_username",
				"connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.sasl_password",
				"connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.ssl_truststore_certificates",
				"connector_spec.connector_config_mirrormaker.target_cluster.alias",
				"connector_spec.connector_config_mirrormaker.target_cluster.this_cluster",
			},
		},
		ConnectorSpec: &kfv1.UpdateConnectorSpec{
			TasksMax: api.WrapInt64(1),
			Properties: map[string]string{
				"key_to_add":    "value",
				"key_to_change": "value",
			},
			ConnectorConfig: &kfv1.UpdateConnectorSpec_ConnectorConfigMirrormaker{
				ConnectorConfigMirrormaker: &kfv1.ConnectorConfigMirrorMakerSpec{
					Topics:            "topics",
					ReplicationFactor: api.WrapInt64(1),
					SourceCluster: &kfv1.ClusterConnectionSpec{
						Alias: "source",
						ClusterConnection: &kfv1.ClusterConnectionSpec_ExternalCluster{
							ExternalCluster: &kfv1.ExternalClusterConnectionSpec{
								BootstrapServers:          "bootstrap_servers",
								SecurityProtocol:          "SASL_SSL",
								SaslMechanism:             "SCRAM-SHA-512",
								SaslUsername:              "egor",
								SaslPassword:              "123",
								SslTruststoreCertificates: "123",
							},
						},
					},
					TargetCluster: &kfv1.ClusterConnectionSpec{
						Alias:             "target",
						ClusterConnection: &kfv1.ClusterConnectionSpec_ThisCluster{},
					},
				},
			},
		},
	}
	updateArgs, err := UpdateConnectorArgsFromGRPC(request)
	expectedUpdateSpec := kfmodels.UpdateConnectorSpec{
		Properties: map[string]string{
			"key_to_add":    "value",
			"key_to_change": "value",
		},
		PropertiesContains: map[string]bool{
			"key_to_add":    true,
			"key_to_change": true,
			"key_to_delete": false,
		},
		MirrormakerConfig: kfmodels.UpdateMirrormakerConfigSpec{
			SourceCluster: kfmodels.UpdateClusterConnectionSpec{},
			TargetCluster: kfmodels.UpdateClusterConnectionSpec{},
		},
	}
	expectedUpdateSpec.Type.Set(kfmodels.ConnectorTypeMirrormaker)
	expectedUpdateSpec.TasksMax.Set(1)
	expectedUpdateSpec.MirrormakerConfig.Topics.Set("topics")
	expectedUpdateSpec.MirrormakerConfig.ReplicationFactor.Set(1)
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.Alias.Set("source")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.Type.Set(kfmodels.ClusterConnectionTypeExternal)
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.BootstrapServers.Set("bootstrap_servers")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SecurityProtocol.Set("SASL_SSL")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslMechanism.Set("SCRAM-SHA-512")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslUsername.Set("egor")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslPasswordUpdated = true
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SaslPassword = secret.NewString("123")
	expectedUpdateSpec.MirrormakerConfig.SourceCluster.SslTruststoreCertificates.Set("123")
	expectedUpdateSpec.MirrormakerConfig.TargetCluster.Alias.Set("target")
	expectedUpdateSpec.MirrormakerConfig.TargetCluster.Type.Set(kfmodels.ClusterConnectionTypeThisCluster)
	expectedUpdateArgs := kafka.UpdateConnectorArgs{
		ClusterID:     "cluster_id",
		Name:          "connector_name",
		ConnectorSpec: expectedUpdateSpec,
	}
	require.NoError(t, err)
	require.Equal(t, expectedUpdateArgs, updateArgs)
}

func TestUpdateConnectorArgsFromGRPC_PropertiesOverwriteRequest(t *testing.T) {
	request := &kfv1.UpdateConnectorRequest{
		ClusterId:     "cluster_id",
		ConnectorName: "connector_name",
		UpdateMask: &fieldmaskpb.FieldMask{
			Paths: []string{
				"connector_spec.properties",
			},
		},
		ConnectorSpec: &kfv1.UpdateConnectorSpec{
			Properties: map[string]string{
				"key_to_add":    "value",
				"key_to_change": "value",
			},
		},
	}
	updateArgs, err := UpdateConnectorArgsFromGRPC(request)
	expectedUpdateSpec := kfmodels.UpdateConnectorSpec{
		Properties: map[string]string{
			"key_to_add":    "value",
			"key_to_change": "value",
		},
		PropertiesOverwrite: true,
	}
	expectedUpdateArgs := kafka.UpdateConnectorArgs{
		ClusterID:     "cluster_id",
		Name:          "connector_name",
		ConnectorSpec: expectedUpdateSpec,
	}
	require.NoError(t, err)
	require.Equal(t, expectedUpdateArgs, updateArgs)
}

func TestUpdateConnectorArgsFromGRPC_PropertiesOverwriteAndFieldWriteInOneRequestFails(t *testing.T) {
	request := &kfv1.UpdateConnectorRequest{
		ClusterId:     "cluster_id",
		ConnectorName: "connector_name",
		UpdateMask: &fieldmaskpb.FieldMask{
			Paths: []string{
				"connector_spec.properties",
				"connector_spec.properties.key_to_change",
			},
		},
		ConnectorSpec: &kfv1.UpdateConnectorSpec{
			Properties: map[string]string{
				"key_to_add":    "value",
				"key_to_change": "value",
			},
		},
	}
	_, err := UpdateConnectorArgsFromGRPC(request)
	require.Error(t, err)
	require.True(t, semerr.IsInvalidInput(err))
	require.Equal(t, "can not use properties and properties.some_field update mask in one request", err.Error())
}

func TestS3ConnectionSpecFromGRPC_NilSpec(t *testing.T) {
	expected := S3ConnectionSpecFromGRPC(nil)
	actual := kfmodels.S3ConnectionSpec{}
	require.Equal(t, expected, actual)
}

func TestS3ConnectionSpecFromGRPC_FullSpec(t *testing.T) {
	spec := kfv1.S3ConnectionSpec{
		BucketName: "test",
		Storage: &kfv1.S3ConnectionSpec_ExternalS3{
			ExternalS3: &kfv1.ExternalS3StorageSpec{
				AccessKeyId:     "test",
				SecretAccessKey: "test",
				Endpoint:        "test",
				Region:          "test",
			},
		},
	}
	actual := S3ConnectionSpecFromGRPC(&spec)
	expected := kfmodels.S3ConnectionSpec{
		Type:            kfmodels.S3ConnectionTypeExternal,
		AccessKeyID:     "test",
		SecretAccessKey: secret.NewString("test"),
		BucketName:      "test",
		Endpoint:        "test",
		Region:          "test",
	}
	require.Equal(t, expected, actual)
}

func TestS3SinkConnectorConfigSpecFromGRPC_FullSpec(t *testing.T) {
	spec := kfv1.ConnectorConfigS3SinkSpec{
		S3Connection: &kfv1.S3ConnectionSpec{
			BucketName: "test",
			Storage: &kfv1.S3ConnectionSpec_ExternalS3{
				ExternalS3: &kfv1.ExternalS3StorageSpec{
					AccessKeyId:     "test",
					SecretAccessKey: "test",
					Endpoint:        "test",
					Region:          "test",
				},
			},
		},
		Topics:              "test",
		FileCompressionType: "test",
		FileMaxRecords:      api.WrapInt64(1),
	}
	expected := S3SinkConnectorConfigSpecFromGRPC(&spec)
	actual := kfmodels.S3SinkConnectorConfigSpec{
		S3Connection: kfmodels.S3ConnectionSpec{
			Type:            kfmodels.S3ConnectionTypeExternal,
			AccessKeyID:     "test",
			SecretAccessKey: secret.NewString("test"),
			BucketName:      "test",
			Endpoint:        "test",
			Region:          "test",
		},
		Topics:              "test",
		FileCompressionType: "test",
		FileMaxRecords:      1,
	}
	require.Equal(t, expected, actual)
}

func TestS3ConnectionToGRPC_FullSpec(t *testing.T) {
	spec := kfmodels.S3Connection{
		Type:        kfmodels.S3ConnectionTypeExternal,
		AccessKeyID: "test",
		BucketName:  "test",
		Endpoint:    "test",
		Region:      "test",
	}
	expected := S3ConnectionToGRPC(spec)
	actual := kfv1.S3Connection{
		BucketName: "test",
		Storage: &kfv1.S3Connection_ExternalS3{
			ExternalS3: &kfv1.ExternalS3Storage{
				AccessKeyId: "test",
				Endpoint:    "test",
				Region:      "test",
			},
		},
	}
	require.Equal(t, expected, &actual)
}

func TestS3SinkConnectorConfigToGRPC_FullSpec(t *testing.T) {
	spec := kfmodels.S3SinkConnectorConfig{
		S3Connection: kfmodels.S3Connection{
			Type:        kfmodels.S3ConnectionTypeExternal,
			AccessKeyID: "test",
			BucketName:  "test",
			Endpoint:    "test",
			Region:      "test",
		},
		Topics:              "test",
		FileCompressionType: "test",
		FileMaxRecords:      int64(1),
	}
	actual := S3SinkConnectorConfigToGRPC(spec)
	expected := &kfv1.ConnectorConfigS3Sink{
		S3Connection: &kfv1.S3Connection{
			BucketName: "test",
			Storage: &kfv1.S3Connection_ExternalS3{
				ExternalS3: &kfv1.ExternalS3Storage{
					AccessKeyId: "test",
					Endpoint:    "test",
					Region:      "test",
				},
			},
		},
		Topics:              "test",
		FileCompressionType: "test",
		FileMaxRecords:      api.WrapInt64(1),
	}
	require.Equal(t, expected, actual)
}

func TestCleanupPolicyFromGRPC(t *testing.T) {
	t.Run("When version 3.0 cleanup policy delete then return \"delete\".", func(t *testing.T) {
		require.Equal(t, "delete", CleanupPolicyFromGRPC(kfv1.TopicConfig3_0_CLEANUP_POLICY_DELETE))
	})

	t.Run("When version 3.0 cleanup policy not valid then return empty string.", func(t *testing.T) {
		require.Equal(t, "", CleanupPolicyFromGRPC("fake"))
	})

	t.Run("When version 3 cleanup policy compact then return \"compact\".", func(t *testing.T) {
		require.Equal(t, "compact", CleanupPolicyFromGRPC(kfv1.TopicConfig3_CLEANUP_POLICY_COMPACT))
	})
}

func TestCleanupPolicyToGRPC(t *testing.T) {
	t.Run("When version 3 cleanup policy delete then return correct spec constant.", func(t *testing.T) {
		require.Equal(t, kfv1.TopicConfig3_CLEANUP_POLICY_DELETE, CleanupPolicy3ToGRPC("delete"))
	})

	t.Run("When version 3 cleanup policy not valid then return correct spec constant.", func(t *testing.T) {
		require.Equal(t, kfv1.TopicConfig3_CLEANUP_POLICY_UNSPECIFIED, CleanupPolicy3ToGRPC("fake"))
	})
}

func TestTopicConfigFromGRPC(t *testing.T) {
	t.Run("TopicConfigFromGRPC with kafka version 3 works correct", func(t *testing.T) {
		deleteRetentionMs := helpers.Pointer[int64](int64(1))
		fileDeleteDelayMs := helpers.Pointer[int64](int64(2))
		flushMessages := helpers.Pointer[int64](int64(3))
		flushMs := helpers.Pointer[int64](int64(4))
		minCompactionLagMs := helpers.Pointer[int64](int64(5))
		retentionBytes := helpers.Pointer[int64](int64(6))
		retentionMs := helpers.Pointer[int64](int64(7))
		maxMessageBytes := helpers.Pointer[int64](int64(8))
		minInsyncReplicas := helpers.Pointer[int64](int64(9))
		segmentBytes := helpers.Pointer[int64](int64(10))
		preallocate := helpers.Pointer[bool](false)
		spec := &kfv1.TopicSpec{
			Name:              "test_topic",
			Partitions:        api.WrapInt64Pointer(helpers.Pointer[int64](int64(5))),
			ReplicationFactor: api.WrapInt64Pointer(helpers.Pointer[int64](int64(3))),
			TopicConfig: &kfv1.TopicSpec_TopicConfig_3{
				TopicConfig_3: &kfv1.TopicConfig3{
					CleanupPolicy:      kfv1.TopicConfig3_CLEANUP_POLICY_UNSPECIFIED,
					CompressionType:    kfv1.CompressionType_COMPRESSION_TYPE_UNCOMPRESSED,
					DeleteRetentionMs:  api.WrapInt64Pointer(deleteRetentionMs),
					FileDeleteDelayMs:  api.WrapInt64Pointer(fileDeleteDelayMs),
					FlushMessages:      api.WrapInt64Pointer(flushMessages),
					FlushMs:            api.WrapInt64Pointer(flushMs),
					MinCompactionLagMs: api.WrapInt64Pointer(minCompactionLagMs),
					RetentionBytes:     api.WrapInt64Pointer(retentionBytes),
					RetentionMs:        api.WrapInt64Pointer(retentionMs),
					MaxMessageBytes:    api.WrapInt64Pointer(maxMessageBytes),
					MinInsyncReplicas:  api.WrapInt64Pointer(minInsyncReplicas),
					SegmentBytes:       api.WrapInt64Pointer(segmentBytes),
					Preallocate:        api.WrapBoolPointer(preallocate),
				},
			},
		}
		config := TopicConfigFromGRPC(spec)
		expected := kfmodels.TopicConfig{
			Version:            kfmodels.СonfigVersion3,
			CleanupPolicy:      kfmodels.CleanupPolicyUnspecified,
			CompressionType:    kfmodels.CompressionTypeUncompressed,
			DeleteRetentionMs:  deleteRetentionMs,
			FileDeleteDelayMs:  fileDeleteDelayMs,
			FlushMessages:      flushMessages,
			FlushMs:            flushMs,
			MinCompactionLagMs: minCompactionLagMs,
			RetentionBytes:     retentionBytes,
			RetentionMs:        retentionMs,
			MaxMessageBytes:    maxMessageBytes,
			MinInsyncReplicas:  minInsyncReplicas,
			SegmentBytes:       segmentBytes,
			Preallocate:        preallocate,
		}
		require.Equal(t, expected, config)
	})
}

func TestTopicSpecFromGRPC(t *testing.T) {
	t.Run("TopicSpecFromGRPC with kafka version 3 works correct", func(t *testing.T) {
		deleteRetentionMs := helpers.Pointer[int64](int64(1))
		fileDeleteDelayMs := helpers.Pointer[int64](int64(2))
		flushMessages := helpers.Pointer[int64](int64(3))
		flushMs := helpers.Pointer[int64](int64(4))
		minCompactionLagMs := helpers.Pointer[int64](int64(5))
		retentionBytes := helpers.Pointer[int64](int64(6))
		retentionMs := helpers.Pointer[int64](int64(7))
		maxMessageBytes := helpers.Pointer[int64](int64(8))
		minInsyncReplicas := helpers.Pointer[int64](int64(9))
		segmentBytes := helpers.Pointer[int64](int64(10))
		preallocate := helpers.Pointer[bool](false)
		spec := &kfv1.TopicSpec{
			Name:              "test_topic",
			Partitions:        api.WrapInt64Pointer(helpers.Pointer[int64](int64(5))),
			ReplicationFactor: api.WrapInt64Pointer(helpers.Pointer[int64](int64(3))),
			TopicConfig: &kfv1.TopicSpec_TopicConfig_3{
				TopicConfig_3: &kfv1.TopicConfig3{
					CleanupPolicy:      kfv1.TopicConfig3_CLEANUP_POLICY_UNSPECIFIED,
					CompressionType:    kfv1.CompressionType_COMPRESSION_TYPE_UNCOMPRESSED,
					DeleteRetentionMs:  api.WrapInt64Pointer(deleteRetentionMs),
					FileDeleteDelayMs:  api.WrapInt64Pointer(fileDeleteDelayMs),
					FlushMessages:      api.WrapInt64Pointer(flushMessages),
					FlushMs:            api.WrapInt64Pointer(flushMs),
					MinCompactionLagMs: api.WrapInt64Pointer(minCompactionLagMs),
					RetentionBytes:     api.WrapInt64Pointer(retentionBytes),
					RetentionMs:        api.WrapInt64Pointer(retentionMs),
					MaxMessageBytes:    api.WrapInt64Pointer(maxMessageBytes),
					MinInsyncReplicas:  api.WrapInt64Pointer(minInsyncReplicas),
					SegmentBytes:       api.WrapInt64Pointer(segmentBytes),
					Preallocate:        api.WrapBoolPointer(preallocate),
				},
			},
		}
		config := TopicSpecFromGRPC(spec)
		expected := kfmodels.TopicSpec{
			Name:              "test_topic",
			Partitions:        5,
			ReplicationFactor: 3,
			Config: kfmodels.TopicConfig{
				Version:            kfmodels.СonfigVersion3,
				CleanupPolicy:      kfmodels.CleanupPolicyUnspecified,
				CompressionType:    kfmodels.CompressionTypeUncompressed,
				DeleteRetentionMs:  deleteRetentionMs,
				FileDeleteDelayMs:  fileDeleteDelayMs,
				FlushMessages:      flushMessages,
				FlushMs:            flushMs,
				MinCompactionLagMs: minCompactionLagMs,
				RetentionBytes:     retentionBytes,
				RetentionMs:        retentionMs,
				MaxMessageBytes:    maxMessageBytes,
				MinInsyncReplicas:  minInsyncReplicas,
				SegmentBytes:       segmentBytes,
				Preallocate:        preallocate,
			},
		}
		require.Equal(t, expected, config)
	})
}

func TestTopicConfigToGRPC(t *testing.T) {
	t.Run("TopicConfigFromGRPC with kafka version 3 works correct", func(t *testing.T) {
		deleteRetentionMs := helpers.Pointer[int64](int64(1))
		fileDeleteDelayMs := helpers.Pointer[int64](int64(2))
		flushMessages := helpers.Pointer[int64](int64(3))
		flushMs := helpers.Pointer[int64](int64(4))
		minCompactionLagMs := helpers.Pointer[int64](int64(5))
		retentionBytes := helpers.Pointer[int64](int64(6))
		retentionMs := helpers.Pointer[int64](int64(7))
		maxMessageBytes := helpers.Pointer[int64](int64(8))
		minInsyncReplicas := helpers.Pointer[int64](int64(9))
		segmentBytes := helpers.Pointer[int64](int64(10))
		preallocate := helpers.Pointer[bool](false)
		topic := kfmodels.Topic{
			Config: kfmodels.TopicConfig{
				Version:            kfmodels.СonfigVersion3,
				CleanupPolicy:      kfmodels.CleanupPolicyCompact,
				CompressionType:    kfmodels.CompressionTypeGzip,
				DeleteRetentionMs:  deleteRetentionMs,
				FileDeleteDelayMs:  fileDeleteDelayMs,
				FlushMessages:      flushMessages,
				FlushMs:            flushMs,
				MinCompactionLagMs: minCompactionLagMs,
				RetentionBytes:     retentionBytes,
				RetentionMs:        retentionMs,
				MaxMessageBytes:    maxMessageBytes,
				MinInsyncReplicas:  minInsyncReplicas,
				SegmentBytes:       segmentBytes,
				Preallocate:        preallocate,
			},
		}
		result := &kfv1.Topic{}
		TopicConfigToGRPC(topic, result)
		expected := &kfv1.Topic{
			TopicConfig: &kfv1.Topic_TopicConfig_3{
				TopicConfig_3: &kfv1.TopicConfig3{
					CleanupPolicy:      kfv1.TopicConfig3_CLEANUP_POLICY_COMPACT,
					CompressionType:    kfv1.CompressionType_COMPRESSION_TYPE_GZIP,
					DeleteRetentionMs:  api.WrapInt64Pointer(deleteRetentionMs),
					FileDeleteDelayMs:  api.WrapInt64Pointer(fileDeleteDelayMs),
					FlushMessages:      api.WrapInt64Pointer(flushMessages),
					FlushMs:            api.WrapInt64Pointer(flushMs),
					MinCompactionLagMs: api.WrapInt64Pointer(minCompactionLagMs),
					RetentionBytes:     api.WrapInt64Pointer(retentionBytes),
					RetentionMs:        api.WrapInt64Pointer(retentionMs),
					MaxMessageBytes:    api.WrapInt64Pointer(maxMessageBytes),
					MinInsyncReplicas:  api.WrapInt64Pointer(minInsyncReplicas),
					SegmentBytes:       api.WrapInt64Pointer(segmentBytes),
					Preallocate:        api.WrapBoolPointer(preallocate),
				},
			},
		}
		require.Equal(t, expected, result)
	})
}

func TestTopicConfigUpdateFromGRPC(t *testing.T) {
	t.Run("TopicConfigUpdateFromGRPC with kafka version 3 works correct", func(t *testing.T) {
		deleteRetentionMs := helpers.Pointer[int64](int64(1))
		maxMessageBytes := helpers.Pointer[int64](int64(2))
		spec := &kfv1.TopicSpec{
			TopicConfig: &kfv1.TopicSpec_TopicConfig_3{
				TopicConfig_3: &kfv1.TopicConfig3{
					DeleteRetentionMs: api.WrapInt64Pointer(deleteRetentionMs),
					MaxMessageBytes:   api.WrapInt64Pointer(maxMessageBytes),
				},
			},
		}
		paths := grpcapi.NewFieldPaths([]string{
			"topic_config_3.delete_retention_ms",
			"topic_config_3.max_message_bytes",
		})
		config, err := topicConfigUpdateFromGRPC(spec, paths)
		expected := kfmodels.TopicConfigUpdate{}
		expected.DeleteRetentionMs.Set(deleteRetentionMs)
		expected.MaxMessageBytes.Set(maxMessageBytes)
		require.NoError(t, err)
		require.Equal(t, expected, config)
	})
}
