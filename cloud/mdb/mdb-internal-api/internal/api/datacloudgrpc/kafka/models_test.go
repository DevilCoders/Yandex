package kafka

import (
	"fmt"
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes/wrappers"
	"github.com/stretchr/testify/require"
	"google.golang.org/protobuf/types/known/wrapperspb"

	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	datacloudv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

func Test_createClusterArgsFromGRPC(t *testing.T) {
	t.Run("When wrong cloud type should return error", func(t *testing.T) {
		req := kfv1.CreateClusterRequest{CloudType: "wrongCloudType"}

		_, err := CreateClusterArgsFromGRPC(&req)

		require.EqualError(t, err, "invalid cloud type: \"wrongCloudType\"")
	})

	t.Run("When wrong kafka version should return error", func(t *testing.T) {
		req := kfv1.CreateClusterRequest{
			CloudType: "aws",
			Version:   "wrongVersion",
		}

		_, err := CreateClusterArgsFromGRPC(&req)

		require.EqualError(t, err, "unknown Apache Kafka version")
	})

	t.Run("When deprecated kafka version should return error", func(t *testing.T) {
		req := kfv1.CreateClusterRequest{
			CloudType: "aws",
			Version:   "2.6",
		}

		_, err := CreateClusterArgsFromGRPC(&req)

		require.EqualError(t, err, "version 2.6 is deprecated")
	})

	t.Run("When resources are empty return error", func(t *testing.T) {
		req := kfv1.CreateClusterRequest{
			CloudType: "aws",
			Version:   "2.8",
			Resources: nil,
		}

		_, err := CreateClusterArgsFromGRPC(&req)

		require.EqualError(t, err, "kafka resources must be set")
	})

	t.Run("When resources are empty return error", func(t *testing.T) {
		req := kfv1.CreateClusterRequest{
			CloudType:   "aws",
			Version:     "2.8",
			Name:        "name",
			Description: "description",
			RegionId:    "regionId",
			ProjectId:   "projectId",
			Resources: &kfv1.ClusterResources{
				Kafka: &kfv1.ClusterResources_Kafka{
					ZoneCount:        &wrapperspb.Int64Value{Value: 3},
					BrokerCount:      &wrapperspb.Int64Value{Value: 1},
					DiskSize:         &wrapperspb.Int64Value{Value: 1234},
					ResourcePresetId: "resourcePresetId",
				},
			},
			Access: validDataCloudAccess(),
		}

		result, err := CreateClusterArgsFromGRPC(&req)

		require.NoError(t, err)
		require.Equal(t, kafka.CreateDataCloudClusterArgs{
			ProjectID:   "projectId",
			Name:        "name",
			Description: "description",
			RegionID:    "regionId",
			CloudType:   environment.CloudTypeAWS,
			ClusterSpec: kfmodels.DataCloudClusterSpec{
				Version:      "2.8",
				BrokersCount: 1,
				ZoneCount:    3,
				Kafka: kfmodels.KafkaConfigSpec{
					Resources: models.ClusterResources{
						DiskSize:            1234,
						ResourcePresetExtID: "resourcePresetId",
					},
				},
				Access: validClusterAccess(),
			},
		}, result)
	})

	t.Run("Specified NetworkID", func(t *testing.T) {
		req := kfv1.CreateClusterRequest{
			CloudType: "aws",
			Resources: &kfv1.ClusterResources{},
			NetworkId: "networkID",
		}

		result, err := CreateClusterArgsFromGRPC(&req)

		require.NoError(t, err)
		require.Equal(t, kafka.CreateDataCloudClusterArgs{
			CloudType: environment.CloudTypeAWS,
			NetworkID: optional.NewString("networkID"),
		}, result)
	})
}

func Test_cleanupPolicyFromGRPC(t *testing.T) {
	t.Run("Check convert values", func(t *testing.T) {
		require.Equal(t, kfmodels.CleanupPolicyUnspecified, cleanupPolicyFromGRPC(kfv1.TopicConfig28_CLEANUP_POLICY_INVALID))
		require.Equal(t, kfmodels.CleanupPolicyDelete, cleanupPolicyFromGRPC(kfv1.TopicConfig28_CLEANUP_POLICY_DELETE))
		require.Equal(t, kfmodels.CleanupPolicyCompact, cleanupPolicyFromGRPC(kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT))
		require.Equal(t, kfmodels.CleanupPolicyCompactAndDelete, cleanupPolicyFromGRPC(kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT_AND_DELETE))
	})

	t.Run("When get unknown cleanup policy should return empty", func(t *testing.T) {
		require.Equal(t, "", cleanupPolicyFromGRPC(1234))
	})
}

func Test_cleanupPolicy2_8ToGRPC(t *testing.T) {
	t.Run("Check convert values", func(t *testing.T) {
		require.Equal(t, kfv1.TopicConfig28_CLEANUP_POLICY_INVALID, cleanupPolicy28ToGRPC(kfmodels.CleanupPolicyUnspecified))
		require.Equal(t, kfv1.TopicConfig28_CLEANUP_POLICY_DELETE, cleanupPolicy28ToGRPC(kfmodels.CleanupPolicyDelete))
		require.Equal(t, kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT, cleanupPolicy28ToGRPC(kfmodels.CleanupPolicyCompact))
		require.Equal(t, kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT_AND_DELETE, cleanupPolicy28ToGRPC(kfmodels.CleanupPolicyCompactAndDelete))
	})

	t.Run("When get unknown cleanup policy should return Invalid", func(t *testing.T) {
		require.Equal(t, kfv1.TopicConfig28_CLEANUP_POLICY_INVALID, cleanupPolicy28ToGRPC("UnknownFiled"))
	})
}

func Test_topicConfigUpdateFromGRPC(t *testing.T) {
	t.Run("When topic spec is empty should return empty", func(t *testing.T) {
		result, err := topicConfigUpdateFromGRPC(&kfv1.TopicSpec{})
		require.NoError(t, err)
		require.Equal(t, kfmodels.TopicConfigUpdate{}, result)
	})

	t.Run("When topic spec config is empty should return empty", func(t *testing.T) {
		result, err := topicConfigUpdateFromGRPC(&kfv1.TopicSpec{
			TopicConfig: &kfv1.TopicSpec_TopicConfig_2_8{},
		})
		require.NoError(t, err)
		require.Equal(t, kfmodels.TopicConfigUpdate{}, result)
	})

	t.Run("When topic spec config is not empty should return not empty", func(t *testing.T) {
		result, err := topicConfigUpdateFromGRPC(&kfv1.TopicSpec{
			TopicConfig: &kfv1.TopicSpec_TopicConfig_2_8{
				TopicConfig_2_8: &kfv1.TopicConfig28{
					CleanupPolicy:   kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT,
					CompressionType: kfv1.TopicConfig28_COMPRESSION_TYPE_ZSTD,
					RetentionBytes:  api.WrapInt64(1),
					RetentionMs:     api.WrapInt64(2),
				},
			},
		})
		require.NoError(t, err)
		require.Equal(t, kfmodels.TopicConfigUpdate{
			CleanupPolicy:   optional.NewString(kfmodels.CleanupPolicyCompact),
			CompressionType: optional.NewString(kfmodels.CompressionTypeZstd),
			RetentionBytes:  optional.NewInt64Pointer(api.WrapInt64ToInt64Pointer(1)),
			RetentionMs:     optional.NewInt64Pointer(api.WrapInt64ToInt64Pointer(2)),
		}, result)
	})
}

func Test_compressionTypeFromGRPC(t *testing.T) {
	t.Run("Check convert values", func(t *testing.T) {
		require.Equal(t, kfmodels.CompressionTypeUnspecified, compressionTypeFromGRPC(kfv1.TopicConfig28_COMPRESSION_TYPE_INVALID))
		require.Equal(t, kfmodels.CompressionTypeUncompressed, compressionTypeFromGRPC(kfv1.TopicConfig28_COMPRESSION_TYPE_UNCOMPRESSED))
		require.Equal(t, kfmodels.CompressionTypeZstd, compressionTypeFromGRPC(kfv1.TopicConfig28_COMPRESSION_TYPE_ZSTD))
		require.Equal(t, kfmodels.CompressionTypeLz4, compressionTypeFromGRPC(kfv1.TopicConfig28_COMPRESSION_TYPE_LZ4))
		require.Equal(t, kfmodels.CompressionTypeSnappy, compressionTypeFromGRPC(kfv1.TopicConfig28_COMPRESSION_TYPE_SNAPPY))
		require.Equal(t, kfmodels.CompressionTypeGzip, compressionTypeFromGRPC(kfv1.TopicConfig28_COMPRESSION_TYPE_GZIP))
		require.Equal(t, kfmodels.CompressionTypeProducer, compressionTypeFromGRPC(kfv1.TopicConfig28_COMPRESSION_TYPE_PRODUCER))
	})

	t.Run("When get unknown cleanup policy should return Invalid", func(t *testing.T) {
		require.Equal(t, "", compressionTypeFromGRPC(2134))
	})
}

func Test_compressionTypeToGRPC(t *testing.T) {
	t.Run("Check convert values", func(t *testing.T) {
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_INVALID, compressionType28ToGRPC(kfmodels.CompressionTypeUnspecified))
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_UNCOMPRESSED, compressionType28ToGRPC(kfmodels.CompressionTypeUncompressed))
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_ZSTD, compressionType28ToGRPC(kfmodels.CompressionTypeZstd))
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_LZ4, compressionType28ToGRPC(kfmodels.CompressionTypeLz4))
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_SNAPPY, compressionType28ToGRPC(kfmodels.CompressionTypeSnappy))
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_GZIP, compressionType28ToGRPC(kfmodels.CompressionTypeGzip))
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_PRODUCER, compressionType28ToGRPC(kfmodels.CompressionTypeProducer))
	})

	t.Run("When get unknown compression type should return Invalid", func(t *testing.T) {
		require.Equal(t, kfv1.TopicConfig28_COMPRESSION_TYPE_INVALID, compressionType28ToGRPC("unknown"))
	})
}

func Test_TopicToGRPC(t *testing.T) {
	t.Run("When topics are empty should return error with invalid kafka version", func(t *testing.T) {
		_, err := TopicToGRPC(kfmodels.Topic{})
		require.EqualError(t, err, "do not know how to convert TopicConfig of kafka version \"\" to grpc format")
	})
	t.Run("When kafka version is wrong should return error", func(t *testing.T) {
		_, err := TopicToGRPC(kfmodels.Topic{Config: kfmodels.TopicConfig{
			Version: "wrongVersion",
		}})
		require.EqualError(t, err, "do not know how to convert TopicConfig of kafka version \"wrongVersion\" to grpc format")
	})
	t.Run("When all fields are empty except kafka version should return empty structure", func(t *testing.T) {
		result, err := TopicToGRPC(kfmodels.Topic{Config: kfmodels.TopicConfig{
			Version: kfmodels.Version2_8,
		}})
		require.NoError(t, err)
		require.Equal(t, &kfv1.Topic{
			ClusterId:         "",
			Name:              "",
			Partitions:        api.WrapInt64(0),
			ReplicationFactor: api.WrapInt64(0),
			TopicConfig: &kfv1.Topic_TopicConfig_2_8{
				TopicConfig_2_8: &kfv1.TopicConfig28{
					CleanupPolicy:   kfv1.TopicConfig28_CLEANUP_POLICY_INVALID,
					CompressionType: kfv1.TopicConfig28_COMPRESSION_TYPE_INVALID,
					RetentionBytes:  api.WrapInt64Pointer(nil),
					RetentionMs:     api.WrapInt64Pointer(nil),
				},
			},
		}, result)
	})

	t.Run("Check convert values", func(t *testing.T) {
		topic := kfmodels.Topic{
			ClusterID:         "cid",
			Name:              "name",
			Partitions:        1,
			ReplicationFactor: 2,
			Config: kfmodels.TopicConfig{
				Version:         kfmodels.Version2_8,
				CleanupPolicy:   kfmodels.CleanupPolicyCompact,
				CompressionType: kfmodels.CompressionTypeSnappy,
				RetentionBytes:  api.WrapInt64ToInt64Pointer(100),
				RetentionMs:     api.WrapInt64ToInt64Pointer(200),
			},
		}
		result, err := TopicToGRPC(topic)
		require.NoError(t, err)
		require.Equal(t, &kfv1.Topic{
			ClusterId:         "cid",
			Name:              "name",
			Partitions:        api.WrapInt64(1),
			ReplicationFactor: api.WrapInt64(2),
			TopicConfig: &kfv1.Topic_TopicConfig_2_8{
				TopicConfig_2_8: &kfv1.TopicConfig28{
					CleanupPolicy:   kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT,
					CompressionType: kfv1.TopicConfig28_COMPRESSION_TYPE_SNAPPY,
					RetentionBytes:  api.WrapInt64(100),
					RetentionMs:     api.WrapInt64(200),
				},
			},
		}, result)
	})
}

func Test_TopicsToGRPC(t *testing.T) {
	t.Run("When topics are empty should return error with invalid kafka version", func(t *testing.T) {
		_, err := TopicsToGRPC([]kfmodels.Topic{{}})
		require.EqualError(t, err, "do not know how to convert TopicConfig of kafka version \"\" to grpc format")
	})
	t.Run("When kafka version is wrong should return error", func(t *testing.T) {
		_, err := TopicsToGRPC([]kfmodels.Topic{{Config: kfmodels.TopicConfig{
			Version: "wrongVersion",
		}}})
		require.EqualError(t, err, "do not know how to convert TopicConfig of kafka version \"wrongVersion\" to grpc format")
	})

	t.Run("Check convert values", func(t *testing.T) {
		topics := []kfmodels.Topic{
			{
				Config: kfmodels.TopicConfig{
					Version: kfmodels.Version2_8,
				},
			},
			{
				ClusterID:         "cid",
				Name:              "name",
				Partitions:        1,
				ReplicationFactor: 2,
				Config: kfmodels.TopicConfig{
					Version:         kfmodels.Version2_8,
					CleanupPolicy:   kfmodels.CleanupPolicyCompact,
					CompressionType: kfmodels.CompressionTypeSnappy,
					RetentionBytes:  api.WrapInt64ToInt64Pointer(100),
					RetentionMs:     api.WrapInt64ToInt64Pointer(200),
				},
			},
		}
		result, err := TopicsToGRPC(topics)
		require.NoError(t, err)
		require.Equal(t, []*kfv1.Topic{
			{
				ClusterId:         "",
				Name:              "",
				Partitions:        api.WrapInt64(0),
				ReplicationFactor: api.WrapInt64(0),
				TopicConfig: &kfv1.Topic_TopicConfig_2_8{
					TopicConfig_2_8: &kfv1.TopicConfig28{
						CleanupPolicy:   kfv1.TopicConfig28_CLEANUP_POLICY_INVALID,
						CompressionType: kfv1.TopicConfig28_COMPRESSION_TYPE_INVALID,
						RetentionBytes:  api.WrapInt64Pointer(nil),
						RetentionMs:     api.WrapInt64Pointer(nil),
					},
				},
			},
			{
				ClusterId:         "cid",
				Name:              "name",
				Partitions:        api.WrapInt64(1),
				ReplicationFactor: api.WrapInt64(2),
				TopicConfig: &kfv1.Topic_TopicConfig_2_8{
					TopicConfig_2_8: &kfv1.TopicConfig28{
						CleanupPolicy:   kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT,
						CompressionType: kfv1.TopicConfig28_COMPRESSION_TYPE_SNAPPY,
						RetentionBytes:  api.WrapInt64(100),
						RetentionMs:     api.WrapInt64(200),
					},
				},
			},
		}, result)
	})
}

func Test_TopicSpecFromGRPC(t *testing.T) {
	t.Run("When topic spec is empty should return empty response", func(t *testing.T) {
		result, err := TopicSpecFromGRPC(&kfv1.TopicSpec{})

		require.NoError(t, err)
		require.Equal(t, kfmodels.TopicSpec{}, result)
	})

	t.Run("When topic spec config is empty should return topic config with version only", func(t *testing.T) {
		req := kfv1.TopicSpec{
			TopicConfig: &kfv1.TopicSpec_TopicConfig_2_8{
				TopicConfig_2_8: &kfv1.TopicConfig28{},
			},
		}

		result, err := TopicSpecFromGRPC(&req)

		require.NoError(t, err)
		require.Equal(t, kfmodels.TopicSpec{
			Config: kfmodels.TopicConfig{Version: kfmodels.Version2_8},
		}, result)
	})

	t.Run("When topic spec config has all fields should fill all fields in response", func(t *testing.T) {
		req := kfv1.TopicSpec{
			Name:              "name",
			Partitions:        api.WrapInt64(1),
			ReplicationFactor: api.WrapInt64(2),
			TopicConfig: &kfv1.TopicSpec_TopicConfig_2_8{
				TopicConfig_2_8: &kfv1.TopicConfig28{
					CleanupPolicy:   kfv1.TopicConfig28_CLEANUP_POLICY_DELETE,
					CompressionType: kfv1.TopicConfig28_COMPRESSION_TYPE_ZSTD,
					RetentionBytes:  api.WrapInt64(3),
					RetentionMs:     api.WrapInt64(4),
				},
			},
		}

		result, err := TopicSpecFromGRPC(&req)

		require.NoError(t, err)
		require.Equal(t, kfmodels.TopicSpec{
			Name:              "name",
			Partitions:        1,
			ReplicationFactor: 2,
			Config: kfmodels.TopicConfig{
				Version:         kfmodels.Version2_8,
				CleanupPolicy:   kfmodels.CleanupPolicyDelete,
				CompressionType: kfmodels.CompressionTypeZstd,
				RetentionBytes:  api.WrapInt64ToInt64Pointer(3),
				RetentionMs:     api.WrapInt64ToInt64Pointer(4),
			},
		}, result)
	})
}

func Test_UpdateTopicArgsFromGRPC(t *testing.T) {
	t.Run("When clusterId is empty should return error", func(t *testing.T) {
		_, err := UpdateTopicArgsFromGRPC(&kfv1.UpdateTopicRequest{})
		require.EqualError(t, err, "missing required argument: cluster_id")
	})

	t.Run("When topicName is empty should return error", func(t *testing.T) {
		_, err := UpdateTopicArgsFromGRPC(&kfv1.UpdateTopicRequest{ClusterId: "cid"})
		require.EqualError(t, err, "missing required argument: name")
	})

	t.Run("When topic spec is empty should return empty topic spec", func(t *testing.T) {
		result, err := UpdateTopicArgsFromGRPC(&kfv1.UpdateTopicRequest{
			ClusterId: "cid",
			TopicName: "topicName",
			TopicSpec: &kfv1.TopicSpec{},
		})

		require.NoError(t, err)
		require.Equal(t, kafka.UpdateTopicArgs{
			ClusterID: "cid",
			Name:      "topicName",
			TopicSpec: kfmodels.TopicSpecUpdate{
				Partitions:        optional.Int64{},
				ReplicationFactor: optional.Int64{},
			},
		}, result)
	})

	t.Run("When topic spec is empty should return empty topic spec", func(t *testing.T) {
		result, err := UpdateTopicArgsFromGRPC(&kfv1.UpdateTopicRequest{
			ClusterId: "cid",
			TopicName: "topicName",
			TopicSpec: &kfv1.TopicSpec{
				Partitions:        api.WrapInt64(5),
				ReplicationFactor: api.WrapInt64(3),
				TopicConfig: &kfv1.TopicSpec_TopicConfig_2_8{
					TopicConfig_2_8: &kfv1.TopicConfig28{
						CleanupPolicy:   kfv1.TopicConfig28_CLEANUP_POLICY_COMPACT,
						CompressionType: kfv1.TopicConfig28_COMPRESSION_TYPE_ZSTD,
						RetentionBytes:  api.WrapInt64(1),
						RetentionMs:     api.WrapInt64(2),
					},
				},
			},
		})

		require.NoError(t, err)
		require.Equal(t, kafka.UpdateTopicArgs{
			ClusterID: "cid",
			Name:      "topicName",
			TopicSpec: kfmodels.TopicSpecUpdate{
				Partitions:        optional.NewInt64(5),
				ReplicationFactor: optional.NewInt64(3),
				Config: kfmodels.TopicConfigUpdate{
					CleanupPolicy:   optional.NewString(kfmodels.CleanupPolicyCompact),
					CompressionType: optional.NewString(kfmodels.CompressionTypeZstd),
					RetentionBytes:  optional.NewInt64Pointer(api.WrapInt64ToInt64Pointer(1)),
					RetentionMs:     optional.NewInt64Pointer(api.WrapInt64ToInt64Pointer(2)),
				},
			},
		}, result)
	})
}

func Test_ModifyClusterArgsFromGRPC(t *testing.T) {
	t.Run("When no clusterId should return error", func(t *testing.T) {
		_, err := modifyClusterArgsFromGRPC(&kfv1.UpdateClusterRequest{})
		require.EqualError(t, err, "missing required argument: cluster_id")
	})

	t.Run("Check empty fields", func(t *testing.T) {
		request := &kfv1.UpdateClusterRequest{
			ClusterId: "clusterId",
		}

		res, err := modifyClusterArgsFromGRPC(request)
		require.NoError(t, err)
		require.Equal(t, kafka.ModifyDataCloudClusterArgs{
			ClusterID: "clusterId",
		}, res)
	})

	t.Run("Set fields to default value", func(t *testing.T) {
		request := &kfv1.UpdateClusterRequest{
			ClusterId:   "clusterId",
			Description: "description",
			Name:        "name",
			Version:     "3.0",
			Resources: &kfv1.ClusterResources{
				Kafka: &kfv1.ClusterResources_Kafka{
					ResourcePresetId: "resourcePresetId",
					DiskSize:         wrapInt64(10000),
					BrokerCount:      wrapInt64(2),
					ZoneCount:        wrapInt64(1),
				},
			},
			Access: validDataCloudAccess(),
		}

		res, err := modifyClusterArgsFromGRPC(request)
		require.NoError(t, err)
		require.Equal(t, kafka.ModifyDataCloudClusterArgs{
			ClusterID:   "clusterId",
			Description: optional.NewString("description"),
			Name:        optional.NewString("name"),
			ConfigSpec: kfmodels.ClusterConfigSpecDataCloudUpdate{
				Kafka: kfmodels.KafkaConfigSpecUpdate{
					Resources: models.ClusterResourcesSpec{
						ResourcePresetExtID: optional.NewString("resourcePresetId"),
						DiskSize:            optional.NewInt64(10000),
					},
				},
				BrokersCount: optional.NewInt64(2),
				ZoneCount:    optional.NewInt64(1),
				Version:      optional.NewString("3.0"),
				Access:       validClusterAccess(),
			},
		}, res)
	})
}

func Test_DataCloudAccessToGRPC(t *testing.T) {
	t.Run("When access is empty should return nil", func(t *testing.T) {
		res := dataCloudAccessToGRPC(clusters.Access{})
		require.Nil(t, res)
	})

	t.Run("When access is not empty should fill all fields", func(t *testing.T) {
		res := dataCloudAccessToGRPC(validClusterAccess())
		require.Equal(t, validDataCloudAccess(), res)
	})
}

func Test_DataCloudAccessFromGRPC(t *testing.T) {
	t.Run("When access is nil should return empty structure", func(t *testing.T) {
		res := dataCloudAccessFromGRPC(nil)
		require.Equal(t, clusters.Access{}, res)
	})

	t.Run("When access is not empty should fill all fields", func(t *testing.T) {
		res := dataCloudAccessFromGRPC(validDataCloudAccess())
		require.Equal(t, validClusterAccess(), res)
	})
}

func Test_DataCloudEncryptionToGRPC(t *testing.T) {
	t.Run("When encryption is empty should return nil", func(t *testing.T) {
		res := dataCloudEncryptionToGRPC(clusters.Encryption{})
		require.Nil(t, res)
	})

	t.Run("When encryption is not empty should fill all fields", func(t *testing.T) {
		for _, in := range []struct {
			enabled bool
		}{
			{
				enabled: true,
			},
			{
				enabled: false,
			},
		} {
			t.Run(fmt.Sprintf("%v", in), func(t *testing.T) {
				res := dataCloudEncryptionToGRPC(validClusterEncryption(in.enabled))
				require.Equal(t, validDataCloudEncryption(in.enabled), res)
			})
		}
	})
}

func Test_DataCloudEncryptionFromGRPC(t *testing.T) {
	t.Run("When encryption is nil should return empty structure", func(t *testing.T) {
		res := dataCloudEncryptionFromGRPC(nil)
		require.Equal(t, clusters.Encryption{}, res)
	})

	t.Run("When encryption is not empty should fill all fields", func(t *testing.T) {
		for _, in := range []struct {
			enabled bool
		}{
			{
				enabled: true,
			},
			{
				enabled: false,
			},
		} {
			t.Run(fmt.Sprintf("%v", in), func(t *testing.T) {
				res := dataCloudEncryptionFromGRPC(validDataCloudEncryption(in.enabled))
				require.Equal(t, validClusterEncryption(in.enabled), res)
			})
		}
	})
}

func Test_ClusterToGRPC(t *testing.T) {
	t.Run("When cluster is empty should return empty", func(t *testing.T) {
		res := ClusterToGRPC(kfmodels.DataCloudCluster{})
		require.Equal(t, &kfv1.Cluster{
			Status: datacloudv1.ClusterStatus_CLUSTER_STATUS_INVALID,
			Resources: &kfv1.ClusterResources{
				Kafka: &kfv1.ClusterResources_Kafka{},
			},
			ConnectionInfo:        &kfv1.ConnectionInfo{},
			PrivateConnectionInfo: &kfv1.PrivateConnectionInfo{},
		}, res)
	})

	t.Run("When cluster is not empty should fill all fields", func(t *testing.T) {
		createdAt := time.Date(2022, 1, 1, 12, 0, 0, 0, time.UTC)

		cluster := kfmodels.DataCloudCluster{
			CloudType: environment.CloudTypeAWS,
			Version:   "2.8",
			RegionID:  "regionId",
			ConnectionInfo: kfmodels.ConnectionInfo{
				ConnectionString: "node1.yadc.io:9091,node2.yadc.io:9091,node3.yadc.io:9091",
				User:             "user",
				Password:         secret.NewString("password"),
			},
			PrivateConnectionInfo: kfmodels.PrivateConnectionInfo{
				ConnectionString: "node1.private.yadc.io:19091,node2.private.yadc.io:19091,node3.private.yadc.io:19091",
				User:             "user",
				Password:         secret.NewString("password"),
			},
			Resources: kfmodels.DataCloudResources{
				ResourcePresetID: optional.NewString("resourcePresetId"),
				DiskSize:         optional.NewInt64(100),
				BrokerCount:      optional.NewInt64(1),
				ZoneCount:        optional.NewInt64(3),
			},
			Access: validClusterAccess(),
		}
		cluster.ClusterID = "clusterId"
		cluster.FolderExtID = "folderExtId"
		cluster.CreatedAt = createdAt
		cluster.Name = "name"
		cluster.Description = "description"
		cluster.Status = clusters.StatusCreating

		res := ClusterToGRPC(cluster)
		require.Equal(t, &kfv1.Cluster{
			Id:          "clusterId",
			ProjectId:   "folderExtId",
			CloudType:   "aws",
			CreateTime:  grpc.TimeToGRPC(createdAt),
			Name:        "name",
			Description: "description",
			Status:      datacloudv1.ClusterStatus_CLUSTER_STATUS_CREATING,
			Version:     "2.8",
			ConnectionInfo: &kfv1.ConnectionInfo{
				ConnectionString: "node1.yadc.io:9091,node2.yadc.io:9091,node3.yadc.io:9091",
				User:             "user",
				Password:         "password",
			},
			PrivateConnectionInfo: &kfv1.PrivateConnectionInfo{
				ConnectionString: "node1.private.yadc.io:19091,node2.private.yadc.io:19091,node3.private.yadc.io:19091",
				User:             "user",
				Password:         "password",
			},
			RegionId: "regionId",
			Resources: &kfv1.ClusterResources{
				Kafka: &kfv1.ClusterResources_Kafka{
					ResourcePresetId: "resourcePresetId",
					DiskSize:         &wrappers.Int64Value{Value: 100},
					BrokerCount:      &wrappers.Int64Value{Value: 1},
					ZoneCount:        &wrappers.Int64Value{Value: 3},
				},
			},
			Access: validDataCloudAccess(),
		}, res)
	})
}

func validDataCloudAccess() *datacloudv1.Access {
	return &datacloudv1.Access{
		Ipv4CidrBlocks: &datacloudv1.Access_CidrBlockList{
			Values: []*datacloudv1.Access_CidrBlock{
				{
					Value:       "0.0.0.0/0",
					Description: "ipv4 description 1",
				},
				{
					Value:       "192.168.0.0/0",
					Description: "ipv4 description 2",
				},
			},
		},
		Ipv6CidrBlocks: &datacloudv1.Access_CidrBlockList{
			Values: []*datacloudv1.Access_CidrBlock{
				{
					Value:       "::/0",
					Description: "ipv6 description 1",
				},
				{
					Value:       "::/1",
					Description: "ipv6 description 2",
				},
			},
		},
		DataServices: &datacloudv1.Access_DataServiceList{
			Values: []datacloudv1.Access_DataService{datacloudv1.Access_DATA_SERVICE_TRANSFER},
		},
	}
}

func validClusterAccess() clusters.Access {
	return clusters.Access{
		Ipv4CidrBlocks: []clusters.CidrBlock{
			{
				Value:       "0.0.0.0/0",
				Description: "ipv4 description 1",
			},
			{
				Value:       "192.168.0.0/0",
				Description: "ipv4 description 2",
			},
		},
		Ipv6CidrBlocks: []clusters.CidrBlock{
			{
				Value:       "::/0",
				Description: "ipv6 description 1",
			},
			{
				Value:       "::/1",
				Description: "ipv6 description 2",
			},
		},
		DataTransfer: optional.NewBool(true),
	}
}

func validDataCloudEncryption(enabled bool) *datacloudv1.DataEncryption {
	return &datacloudv1.DataEncryption{
		Enabled: grpc.OptionalBoolToGRPC(optional.NewBool(enabled)),
	}
}

func validClusterEncryption(enabled bool) clusters.Encryption {
	return clusters.Encryption{
		Enabled: optional.NewBool(enabled),
	}
}
