package kafka

import (
	"testing"

	"github.com/stretchr/testify/require"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/helpers"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
)

func TestConfigSpecFromGRPC(t *testing.T) {
	t.Run("When kafka 3x common config with not yet supported kafka cluster version 3.7 then error.", func(t *testing.T) {
		configSpec := &kfv1.ConfigSpec{
			Version: "3.7",
			Kafka: &kfv1.ConfigSpec_Kafka{
				KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3{
					KafkaConfig_3: &kfv1.KafkaConfig3{},
				},
			},
		}
		_, err := configSpecFromGRPC(configSpec)
		require.Error(t, err)
		require.Equal(t, "the config version \"3\" does not match the cluster version \"3.7\". Supported versions: [3.0 3.1 3.2].", err.Error())
	})

	t.Run("When kafka 3x common config with supported kafka cluster version 3.1 then correct.", func(t *testing.T) {
		configSpec := &kfv1.ConfigSpec{
			Version: "3.1",
			Kafka: &kfv1.ConfigSpec_Kafka{
				KafkaConfig: &kfv1.ConfigSpec_Kafka_KafkaConfig_3{
					KafkaConfig_3: &kfv1.KafkaConfig3{
						CompressionType:             kfv1.CompressionType_COMPRESSION_TYPE_GZIP,
						LogFlushIntervalMessages:    api.WrapInt64(int64(1)),
						LogFlushIntervalMs:          api.WrapInt64(int64(2)),
						LogFlushSchedulerIntervalMs: api.WrapInt64(int64(3)),
						LogRetentionBytes:           api.WrapInt64(int64(4)),
						LogRetentionHours:           api.WrapInt64(int64(5)),
						LogRetentionMinutes:         api.WrapInt64(int64(6)),
						LogRetentionMs:              api.WrapInt64(int64(7)),
						LogSegmentBytes:             api.WrapInt64(int64(8)),
						SocketSendBufferBytes:       api.WrapInt64(int64(9)),
						SocketReceiveBufferBytes:    api.WrapInt64(int64(10)),
						AutoCreateTopicsEnable:      api.WrapBool(true),
						NumPartitions:               api.WrapInt64(int64(11)),
						DefaultReplicationFactor:    api.WrapInt64(int64(12)),
						MessageMaxBytes:             api.WrapInt64(int64(13)),
						ReplicaFetchMaxBytes:        api.WrapInt64(int64(14)),
						SslCipherSuites: []string{
							"TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256",
							"TLS_RSA_WITH_AES_256_CBC_SHA256",
						},
						OffsetsRetentionMinutes: api.WrapInt64(int64(15)),
					},
				},
			},
		}
		mdbClusterSpecResult, err := configSpecFromGRPC(configSpec)
		mdbClusterSpecExpected := kfmodels.MDBClusterSpec{
			Version: "3.1",
			Kafka: kfmodels.KafkaConfigSpec{
				Config: kfmodels.KafkaConfig{
					CompressionType:             kfmodels.CompressionTypeGzip,
					LogFlushIntervalMessages:    helpers.Pointer[int64](int64(1)),
					LogFlushIntervalMs:          helpers.Pointer[int64](int64(2)),
					LogFlushSchedulerIntervalMs: helpers.Pointer[int64](int64(3)),
					LogRetentionBytes:           helpers.Pointer[int64](int64(4)),
					LogRetentionHours:           helpers.Pointer[int64](int64(5)),
					LogRetentionMinutes:         helpers.Pointer[int64](int64(6)),
					LogRetentionMs:              helpers.Pointer[int64](int64(7)),
					LogSegmentBytes:             helpers.Pointer[int64](int64(8)),
					SocketSendBufferBytes:       helpers.Pointer[int64](int64(9)),
					SocketReceiveBufferBytes:    helpers.Pointer[int64](int64(10)),
					AutoCreateTopicsEnable:      helpers.Pointer[bool](true),
					NumPartitions:               helpers.Pointer[int64](int64(11)),
					DefaultReplicationFactor:    helpers.Pointer[int64](int64(12)),
					MessageMaxBytes:             helpers.Pointer[int64](int64(13)),
					ReplicaFetchMaxBytes:        helpers.Pointer[int64](int64(14)),
					SslCipherSuites: []string{
						"TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256",
						"TLS_RSA_WITH_AES_256_CBC_SHA256",
					},
					OffsetsRetentionMinutes: helpers.Pointer[int64](int64(15)),
				},
			},
		}
		require.NoError(t, err)
		require.Equal(t, mdbClusterSpecExpected.Kafka.Config, mdbClusterSpecResult.Kafka.Config)
		require.Equal(t, mdbClusterSpecExpected.Version, mdbClusterSpecResult.Version)
	})
}
