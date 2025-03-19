package kfpillars

import (
	"encoding/json"
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/defaults"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/ptr"
)

const GB = 1024 * 1024 * 1024

func parseCluster(raw json.RawMessage) (*Cluster, error) {
	var p Cluster
	if err := p.UnmarshalPillar(raw); err != nil {
		return nil, err
	}

	return &p, nil
}

func TestDeleteUser(t *testing.T) {
	pillar := NewCluster()
	bob := UserData{
		Name:     "bob",
		Password: pillars.CryptoKey{},
		Permissions: []PermissionsData{
			{TopicName: "topic1", Role: "producer", Group: "*", Host: "*"},
		},
	}
	err := pillar.AddUser(bob)
	require.NoError(t, err)
	require.Contains(t, pillar.Data.Kafka.Users, "bob")
	require.NotContains(t, pillar.Data.Kafka.DeletedUsers, "bob")

	err = pillar.DeleteUser("bob")
	require.NoError(t, err)
	require.NotContains(t, pillar.Data.Kafka.Users, "bob")
	require.Contains(t, pillar.Data.Kafka.DeletedUsers, "bob")

	err = pillar.AddUser(bob)
	require.NoError(t, err)
	require.Contains(t, pillar.Data.Kafka.Users, "bob")
	require.NotContains(t, pillar.Data.Kafka.DeletedUsers, "bob")

	pillar.Data.Kafka.DeletedUsers = nil // emulate old pillar format
	msg, err := pillar.MarshalPillar()
	require.NoError(t, err)

	pillar = new(Cluster)
	err = pillar.UnmarshalPillar(msg)
	require.NoError(t, err)

	err = pillar.DeleteUser("bob")
	require.NoError(t, err)
	require.NotContains(t, pillar.Data.Kafka.Users, "bob")
	require.Contains(t, pillar.Data.Kafka.DeletedUsers, "bob")
}

func TestDeleteTopic(t *testing.T) {
	pillar := NewCluster()
	pillar.Data.Kafka.BrokersCount = 3
	pillar.Data.Kafka.ZoneID = []string{"zone1"}
	events := TopicData{
		Name:              "events",
		Partitions:        24,
		ReplicationFactor: 3,
		Config:            TopicConfig{},
	}
	err := pillar.AddTopic(events)
	require.NoError(t, err)
	require.Contains(t, pillar.Data.Kafka.Topics, "events")
	require.NotContains(t, pillar.Data.Kafka.DeletedTopics, "events")

	err = pillar.DeleteTopic("events")
	require.NoError(t, err)
	require.NotContains(t, pillar.Data.Kafka.Topics, "events")
	require.Contains(t, pillar.Data.Kafka.DeletedTopics, "events")

	err = pillar.AddTopic(events)
	require.NoError(t, err)
	require.Contains(t, pillar.Data.Kafka.Topics, "events")
	require.NotContains(t, pillar.Data.Kafka.DeletedTopics, "events")

	pillar.Data.Kafka.DeletedTopics = nil // emulate old pillar format
	msg, err := pillar.MarshalPillar()
	require.NoError(t, err)

	pillar = new(Cluster)
	err = pillar.UnmarshalPillar(msg)
	require.NoError(t, err)

	err = pillar.DeleteTopic("events")
	require.NoError(t, err)
	require.NotContains(t, pillar.Data.Kafka.Topics, "events")
	require.Contains(t, pillar.Data.Kafka.DeletedTopics, "events")
}

func TestRequiredDiskSize(t *testing.T) {
	mb := int64(1024 * 1024)

	t.Run("DefaultSegmentSize", func(t *testing.T) {
		cluster := Cluster{
			Data: ClusterData{
				Kafka: KafkaData{
					Topics: map[string]TopicData{
						"topic1": {
							Partitions:        12,
							ReplicationFactor: 3,
						},
						"topic2": {
							Partitions:        6,
							ReplicationFactor: 2,
						},
					},
					BrokersCount: 2,
					ZoneID:       []string{"zone1", "zone2", "zone3"},
				},
			},
		}
		expected := (1024*mb*12*3 + 1024*mb*6*2) * 2 / 6
		require.Equal(t, expected, cluster.requiredDiskSize())
	})

	t.Run("ClusterWideSegmentSize", func(t *testing.T) {
		cluster := Cluster{
			Data: ClusterData{
				Kafka: KafkaData{
					Topics: map[string]TopicData{
						"topic1": {
							Partitions:        12,
							ReplicationFactor: 3,
						},
						"topic2": {
							Partitions:        6,
							ReplicationFactor: 2,
						},
					},
					BrokersCount: 2,
					ZoneID:       []string{"zone1", "zone2", "zone3"},
					Config: KafkaConfig{
						LogSegmentBytes: create(128 * mb),
					},
				},
			},
		}
		expected := (128*mb*12*3 + 128*mb*6*2) * 2 / 6
		require.Equal(t, expected, cluster.requiredDiskSize())
	})

	t.Run("PerTopicSegmentSize", func(t *testing.T) {
		cluster := Cluster{
			Data: ClusterData{
				Kafka: KafkaData{
					Topics: map[string]TopicData{
						"topic1": {
							Partitions:        12,
							ReplicationFactor: 3,
							Config: TopicConfig{
								SegmentBytes: create(256 * mb),
							},
						},
						"topic2": {
							Partitions:        6,
							ReplicationFactor: 2,
						},
					},
					BrokersCount: 2,
					ZoneID:       []string{"zone1", "zone2", "zone3"},
					Config: KafkaConfig{
						LogSegmentBytes: create(128 * mb),
					},
				},
			},
		}
		expected := (256*mb*12*3 + 128*mb*6*2) * 2 / 6
		require.Equal(t, expected, cluster.requiredDiskSize())
	})
}

func TestCustomAuthorizer(t *testing.T) {
	pillar := NewCluster()
	require.True(t, pillar.Data.Kafka.UseCustomAuthorizerForCluster)
}

func TestKnownTopicConfigProperties(t *testing.T) {
	require.Contains(t, KnownTopicConfigProperties, "retention.bytes")
	require.NotContains(t, KnownTopicConfigProperties, "-")
	require.GreaterOrEqual(t, len(KnownTopicConfigProperties), 13)
}

func TestSetVersionOnCreate(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		pillar := NewCluster()
		err := pillar.SetVersion("2.8")
		require.NoError(t, err)
		require.Equal(t, "2.8", pillar.Data.Kafka.Version)
		require.Equal(t, "2.8.1.1-java11", pillar.Data.Kafka.PackageVersion)
	})

	t.Run("UseDefault", func(t *testing.T) {
		pillar := NewCluster()
		err := pillar.SetVersion("")
		require.NoError(t, err)
		require.Equal(t, "3.0", pillar.Data.Kafka.Version)
		require.Equal(t, "3.0.1-java11", pillar.Data.Kafka.PackageVersion)
	})

	t.Run("UnknownVersion", func(t *testing.T) {
		pillar := NewCluster()
		err := pillar.SetVersion("2.7")
		require.EqualError(t, err, "unknown Apache Kafka version")
	})
}

func TestChangeVersion(t *testing.T) {
	t.Run("When cluster is created and the version of is setting up should be set up version and inter broker protocol version", func(t *testing.T) {
		pillar := NewCluster()
		err := pillar.SetVersion("2.8")
		require.NoError(t, err)
		require.Equal(t, "2.8", pillar.Data.Kafka.Version)
		require.Equal(t, "2.8.1.1-java11", pillar.Data.Kafka.PackageVersion)
		require.Equal(t, "2.8", pillar.Data.Kafka.InterBrokerProtocolVersion)
	})

	t.Run("When the version of cluster is updated should update version but not inter broker protocol version", func(t *testing.T) {
		pillar := NewCluster()
		pillar.Data.Kafka.Version = "2.6"
		err := pillar.SetVersion("2.8")
		require.NoError(t, err)
		require.Equal(t, "2.8", pillar.Data.Kafka.Version)
		require.Equal(t, "2.8.1.1-java11", pillar.Data.Kafka.PackageVersion)
		require.Equal(t, "2.6", pillar.Data.Kafka.InterBrokerProtocolVersion)
	})

	t.Run("UnknownVersion", func(t *testing.T) {
		pillar := NewCluster()
		pillar.Data.Kafka.Version = "2.6"
		err := pillar.SetVersion("2.7")
		require.EqualError(t, err, "unknown Apache Kafka version")
	})
}

func TestClusterValidate(t *testing.T) {
	pillar := NewCluster()
	pillar.SetOneHostMode()
	pillar.Data.Kafka.BrokersCount = 1
	pillar.Data.Kafka.ZoneID = []string{"vla"}
	pillar.Data.Kafka.Resources = models.ClusterResources{
		ResourcePresetExtID: "flavour-id1",
		DiskTypeExtID:       "nbs",
		DiskSize:            5 * GB,
	}
	err := pillar.AddKafkaNode(KafkaNodeData{
		ID:   1,
		FQDN: "kafka-node1-vla",
		Rack: "vla",
	})
	require.NoError(t, err)
	raw, err := pillar.MarshalPillar()
	require.NoError(t, err)

	t.Run("Success case", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Zero brokers count fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.BrokersCount = 0
		err = pillar.Validate()
		require.EqualError(t, err, "brokers count must be at least 1")
	})

	t.Run("Skip validation works", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.SetSkipValidation(true)
		pillar.Data.Kafka.BrokersCount = 0
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Auto create topics without unmanaged topics fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		autoCreate := true
		pillar.Data.Kafka.Config = KafkaConfig{
			AutoCreateTopicsEnable: &autoCreate,
		}
		err = pillar.Validate()
		require.EqualError(t, err, "auto_create_topics_enabled can be set only with unmanaged_topics")
	})

	t.Run("Auto create topics with topics sync works", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.SyncTopics = true
		autoCreate := true
		pillar.Data.Kafka.Config = KafkaConfig{
			AutoCreateTopicsEnable: &autoCreate,
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Default replication factor less than broker number fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			DefaultReplicationFactor: create(2),
		}
		err = pillar.Validate()
		require.EqualError(t, err, "default_replication_factor(2) must be less then quantity of brokers(1)")
	})

	t.Run("Offsets retention minutes less then 1", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			OffsetsRetentionMinutes: create(-1),
		}
		err = pillar.Validate()
		require.EqualError(t, err, "wrong value for offsets.retention.minutes: -1")
	})

	t.Run("Disk size less than calculated from topics fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        6,
			ReplicationFactor: 1,
		})
		require.NoError(t, err)
		err = pillar.Validate()
		require.EqualError(t, err, "disk size must be at least 12884901888 according to topics partitions number and replication factor but size is 5368709120")
	})

	t.Run("Disk size less than calculated from topics not fails when disk size validation disabled", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        6,
			ReplicationFactor: 1,
		})
		require.NoError(t, err)
		pillar.Data.Kafka.DiskSizeValidationDisabled = true
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Setting max.message.bytes of topic more then replica.fetch.max.bytes of cluster failed", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 1,
			Config: TopicConfig{
				MaxMessageBytes: create(1013),
			},
		})
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			ReplicaFetchMaxBytes: create(1000),
			MessageMaxBytes:      create(999),
		}
		pillar.Data.Kafka.BrokersCount = 1
		pillar.Data.Kafka.ZoneID = []string{"zone1", "zone2", "zone3"}
		pillar.Data.Kafka.Nodes = map[string]KafkaNodeData{
			"node1": {},
			"node2": {},
			"node3": {},
		}
		err = pillar.Validate()
		require.EqualError(t, err, "For multi-node kafka cluster, broker setting \"replica.fetch.max.bytes\" value(1000) must be equal or greater then topic (\"topic1\") setting \"max.message.bytes\" value(1013) - record log overhead size(12).")
	})

	t.Run("Setting max.message.bytes of topic default value more then replica.fetch.max.bytes of cluster failed", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 1,
		})
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			ReplicaFetchMaxBytes: create(1000),
			MessageMaxBytes:      create(999),
		}
		pillar.Data.Kafka.BrokersCount = 1
		pillar.Data.Kafka.ZoneID = []string{"zone1", "zone2", "zone3"}
		pillar.Data.Kafka.Nodes = map[string]KafkaNodeData{
			"node1": {},
			"node2": {},
			"node3": {},
		}
		err = pillar.Validate()
		require.EqualError(t, err, "For multi-node kafka cluster, broker setting \"replica.fetch.max.bytes\" value(1000) must be equal or greater then topic (\"topic1\") setting \"max.message.bytes\" value(1048588) - record log overhead size(12).")
	})

	t.Run("Setting max.message.bytes of topic value more then replica.fetch.max.bytes of cluster default value failed", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 1,
			Config: TopicConfig{
				MaxMessageBytes: create(1100001),
			},
		})
		pillar.Data.Kafka.BrokersCount = 1
		pillar.Data.Kafka.ZoneID = []string{"zone1", "zone2", "zone3"}
		pillar.Data.Kafka.Nodes = map[string]KafkaNodeData{
			"node1": {},
			"node2": {},
			"node3": {},
		}
		require.NoError(t, err)
		err = pillar.Validate()
		require.EqualError(t, err, "For multi-node kafka cluster, broker setting \"replica.fetch.max.bytes\" value(1048576) must be equal or greater then topic (\"topic1\") setting \"max.message.bytes\" value(1100001) - record log overhead size(12).")
	})

	t.Run("Setting max.message.bytes of topic equal to replica.fetch.max.bytes of cluster accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 1,
			Config: TopicConfig{
				MaxMessageBytes: create(1000),
			},
		})
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			ReplicaFetchMaxBytes: create(1000),
			MessageMaxBytes:      create(999),
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Setting max.message.bytes of topic less then replica.fetch.max.bytes of cluster accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 1,
			Config: TopicConfig{
				MaxMessageBytes: create(900),
			},
		})
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			ReplicaFetchMaxBytes: create(1000),
			MessageMaxBytes:      create(999),
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Setting max.message.bytes of topic less then default value replica.fetch.max.bytes of cluster accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 1,
			Config: TopicConfig{
				MaxMessageBytes: create(900),
			},
		})
		require.NoError(t, err)
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Setting max.message.bytes of topic default value less then replica.fetch.max.bytes of cluster accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 1,
		})
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			ReplicaFetchMaxBytes: create(1100000),
			MessageMaxBytes:      create(999),
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Single invalid sslCipherSuit in list fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			SslCipherSuites: []string{"blank"},
		}
		err = pillar.Validate()
		require.EqualError(t, err, fmt.Sprintf("these suites are invalid: [blank]. List of valid suites: [%s].", defaults.AllValidSslCipherSuitesSortedString))
	})

	t.Run("List contains only invalid sslCipherSuit's fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			SslCipherSuites: []string{"blank", "abc", "zephyr"},
		}
		err = pillar.Validate()
		require.EqualError(t, err, fmt.Sprintf("these suites are invalid: [abc blank zephyr]. List of valid suites: [%s].", defaults.AllValidSslCipherSuitesSortedString))
	})

	t.Run("List contains only valid sslCipherSuit's accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			SslCipherSuites: []string{
				"TLS_DHE_RSA_WITH_AES_128_CBC_SHA",
				"TLS_DHE_RSA_WITH_AES_256_CBC_SHA",
				"TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256",
				"TLS_AKE_WITH_AES_128_GCM_SHA256",
			},
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("List contains all valid sslCipherSuit's accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			SslCipherSuites: defaults.AllValidSslCipherSuitesSortedSlice,
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Empty sslCipherSuit's list accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			SslCipherSuites: []string{},
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("Nil sslCipherSuit's list accepted", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		pillar.Data.Kafka.Config = KafkaConfig{
			SslCipherSuites: nil,
		}
		err = pillar.Validate()
		require.NoError(t, err)
	})

	t.Run("When current pillar does not pass disk size validation", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        6,
			ReplicationFactor: 1,
		})
		require.NoError(t, err)

		// check that we already exceeded disk size
		err = pillar.Validate()
		require.Error(t, err)
		require.Contains(t, err.Error(), "disk size must be at least")

		// add topic that we will delete later
		err = pillar.AddTopic(TopicData{
			Name:              "topic2",
			Partitions:        6,
			ReplicationFactor: 1,
		})
		require.NoError(t, err)

		newRaw, err := pillar.MarshalPillar()
		require.NoError(t, err)

		t.Run("Then topic delete is always allowed", func(t *testing.T) {
			pillar, err := parseCluster(newRaw)
			require.NoError(t, err)

			err = pillar.DeleteTopic("topic2")
			require.NoError(t, err)
			err = pillar.Validate()
			require.NoError(t, err)
		})

		t.Run("Then any config change is allowed", func(t *testing.T) {
			pillar, err := parseCluster(newRaw)
			require.NoError(t, err)

			pillar.Data.Kafka.Config.LogRetentionMs = create(10000)
			err = pillar.Validate()
			require.NoError(t, err)
		})

		t.Run("Then any change that increases required disk size is not allowed", func(t *testing.T) {
			pillar, err := parseCluster(newRaw)
			require.NoError(t, err)

			err = pillar.AddTopic(TopicData{
				Name:              "topic3",
				Partitions:        6,
				ReplicationFactor: 1,
			})
			require.NoError(t, err)

			err = pillar.Validate()
			require.Error(t, err)
			require.Contains(t, err.Error(), "disk size must be at least")
		})
	})

	t.Run("Topic replication factor less than brokers fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic1",
			Partitions:        1,
			ReplicationFactor: 2,
		})
		require.NoError(t, err)
		err = pillar.Validate()
		require.EqualError(t, err, "topic \"topic1\" has too big replication factor: 2. maximum is brokers quantity: 1")
	})

	t.Run("Topic colliding names fails", func(t *testing.T) {
		pillar, err := parseCluster(raw)
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic.complex-name_example",
			Partitions:        1,
			ReplicationFactor: 2,
		})
		require.NoError(t, err)
		err = pillar.AddTopic(TopicData{
			Name:              "topic_complex-name.example",
			Partitions:        1,
			ReplicationFactor: 2,
		})
		require.EqualError(t, err, "name \"topic_complex-name.example\" colliding with \"topic.complex-name_example\"")
	})

}

func TestTopicDataValidate(t *testing.T) {
	topic := TopicData{
		Name:              "topic",
		Partitions:        3,
		ReplicationFactor: 1,
	}

	t.Run("Success case", func(t *testing.T) {
		err := topic.Validate()
		require.NoError(t, err)
	})

	t.Run("Empty name fails", func(t *testing.T) {
		topic := topic
		topic.Name = ""
		err := topic.Validate()
		require.EqualError(t, err, "topic name must be specified")
	})

	t.Run("Zero topic partitions fails", func(t *testing.T) {
		topic := topic
		topic.Partitions = 0
		err := topic.Validate()
		require.EqualError(t, err, "topic \"topic\" has too small partitions number: 0. minimum is 1")
	})

	t.Run("Negative value of FlushMs fails", func(t *testing.T) {
		topic := topic
		topic.Config.FlushMs = create(-1)
		err := topic.Validate()
		require.EqualError(t, err, "topic \"topic\" has wrong value for flush.ms: -1")
	})

	t.Run("Less than -1 value of RetentionMs fails", func(t *testing.T) {
		topic := topic
		topic.Config.RetentionMs = create(-2)
		err := topic.Validate()
		require.EqualError(t, err, "topic \"topic\" has wrong value for retention.ms: -2")
	})

	t.Run("Small positive value of SegmentBytes fails", func(t *testing.T) {
		topic := topic
		topic.Config.SegmentBytes = create(2)
		err := topic.Validate()
		require.EqualError(t, err, "topic \"topic\" has wrong value for segment.bytes: 2. minimum is 14")
	})

	t.Run("Zero MinISR fails", func(t *testing.T) {
		topic := topic
		topic.Config.MinInsyncReplicas = create(0)
		err := topic.Validate()
		require.EqualError(t, err, "topic \"topic\" has wrong value for min.insync.replicas: 0. minimum is 1")
	})

	t.Run("MinISR greater than replication factor fails", func(t *testing.T) {
		topic := topic
		topic.Config.MinInsyncReplicas = create(2)
		err := topic.Validate()
		require.EqualError(t, err, "topic \"topic\" has too big min.insync.replicas parameter: 2. maximum is topic replication factor: 1")
	})

}

func TestUserDataValidate(t *testing.T) {
	t.Run("Success case", func(t *testing.T) {
		user := UserData{
			Name: "username",
			Permissions: []PermissionsData{
				{
					TopicName: "topic",
					Role:      "producer",
				},
			},
		}
		err := user.Validate()
		require.NoError(t, err)
	})

	t.Run("Admin permissions with all topics works", func(t *testing.T) {
		user := UserData{
			Name: "username",
			Permissions: []PermissionsData{
				{
					TopicName: "*",
					Role:      "admin",
				},
			},
		}
		err := user.Validate()
		require.NoError(t, err)
	})

	t.Run("Admin permissions with specific topic fails", func(t *testing.T) {
		user := UserData{
			Name: "username",
			Permissions: []PermissionsData{
				{
					TopicName: "topic",
					Role:      "admin",
				},
			},
		}
		err := user.Validate()
		require.EqualError(t, err, "admin role can be set only on all topics. user with admin role \"username\"")
	})
}

func TestKafkaConfigValidate(t *testing.T) {
	t.Run("Success case", func(t *testing.T) {
		conf := KafkaConfig{}
		err := conf.Validate()
		require.NoError(t, err)
	})

	t.Run("Negative value of LogFlushIntervalMessages fails", func(t *testing.T) {
		conf := KafkaConfig{}
		conf.LogFlushIntervalMessages = create(-1)
		err := conf.Validate()
		require.EqualError(t, err, "wrong value for log.flush.interval.messages: -1")
	})

	t.Run("LogRetentionBytes less than -1 fails", func(t *testing.T) {
		conf := KafkaConfig{}
		conf.LogRetentionBytes = create(-2)
		err := conf.Validate()
		require.EqualError(t, err, "wrong value for log.retention.bytes: -2")
	})

	t.Run("NumPartitions less than -1 fails", func(t *testing.T) {
		conf := KafkaConfig{}
		conf.NumPartitions = create(-2)
		err := conf.Validate()
		require.EqualError(t, err, "wrong value for num.partitions: -2")
	})

	t.Run("Zero value of SocketSendBufferBytes fails", func(t *testing.T) {
		conf := KafkaConfig{}
		conf.SocketSendBufferBytes = create(0)
		err := conf.Validate()
		require.EqualError(t, err, "wrong value for socket.send.buffer.bytes: 0")
	})

	t.Run("Positive value of SocketSendBufferBytes is accepted", func(t *testing.T) {
		conf := KafkaConfig{}
		conf.SocketSendBufferBytes = create(1)
		err := conf.Validate()
		require.NoError(t, err)
	})

	t.Run("Negative (except for -1) value of SocketReceiveBufferBytes fails", func(t *testing.T) {
		conf := KafkaConfig{}
		conf.SocketReceiveBufferBytes = create(-2)
		err := conf.Validate()
		require.EqualError(t, err, "wrong value for socket.receive.buffer.bytes: -2")
	})

	t.Run("-1 value of SocketReceiveBufferBytes is accepted", func(t *testing.T) {
		conf := KafkaConfig{}
		conf.SocketReceiveBufferBytes = create(-1)
		err := conf.Validate()
		require.NoError(t, err)
	})
}

func TestConnectorValidate(t *testing.T) {
	mmConfig := MirrorMakerConfig{
		ReplicationFactor: 1,
		Topics:            "topic*",
		SourceCluster: ClusterConnection{
			Alias:            "source",
			Type:             kfmodels.ClusterConnectionTypeExternal,
			BootstrapServers: "server1:9091",
		},
		TargetCluster: ClusterConnection{
			Type: kfmodels.ClusterConnectionTypeThisCluster,
		},
	}

	t.Run("Success case", func(t *testing.T) {
		conf := ConnectorData{
			Name:              "cluster-migration",
			TasksMax:          1,
			Type:              kfmodels.ConnectorTypeMirrormaker,
			MirrorMakerConfig: mmConfig,
		}
		err := conf.Validate()
		require.NoError(t, err)
	})

	t.Run("Zero task max value fails", func(t *testing.T) {
		conf := ConnectorData{
			Name:              "cluster-migration",
			TasksMax:          0,
			Type:              kfmodels.ConnectorTypeMirrormaker,
			MirrorMakerConfig: mmConfig,
		}
		err := conf.Validate()
		require.EqualError(t, err, "connector \"cluster-migration\" has too small tasks.max setting's value: 0. minimum is 1")
	})

	t.Run("Name with invalid symbols fails", func(t *testing.T) {
		conf := ConnectorData{
			Name:              "cluster-migration@@@",
			TasksMax:          1,
			Type:              kfmodels.ConnectorTypeMirrormaker,
			MirrorMakerConfig: mmConfig,
		}
		err := conf.Validate()
		require.EqualError(t, err, "connector name \"cluster-migration@@@\" has invalid symbols")
	})

	t.Run("Unknown conector type  fails", func(t *testing.T) {
		conf := ConnectorData{
			Name:              "cluster-migration",
			TasksMax:          1,
			Type:              "Bad connector type",
			MirrorMakerConfig: mmConfig,
		}
		err := conf.Validate()
		require.EqualError(t, err, "unknown connector type - \"Bad connector type\"")
	})
}

func getMirrorMakerConfig() MirrorMakerConfig {
	return MirrorMakerConfig{
		ReplicationFactor: 1,
		Topics:            "topic*",
		SourceCluster: ClusterConnection{
			Alias:            "source",
			Type:             kfmodels.ClusterConnectionTypeExternal,
			BootstrapServers: "server1:9091",
		},
		TargetCluster: ClusterConnection{
			Type: kfmodels.ClusterConnectionTypeThisCluster,
		},
	}
}

func TestMirrorMakerValidate(t *testing.T) {

	t.Run("Success case", func(t *testing.T) {
		mmConfig := getMirrorMakerConfig()
		err := mmConfig.Validate("connector-name")
		require.NoError(t, err)
	})

	t.Run("Just * topic name pattern fails", func(t *testing.T) {
		mmConfig := getMirrorMakerConfig()
		mmConfig.Topics = "*"
		err := mmConfig.Validate("connector-name")
		require.EqualError(t, err, "* is not valid topic name pattern for connector \"connector-name\". For all topics use .* instead")
	})

	t.Run(".* topic name pattern works", func(t *testing.T) {
		mmConfig := getMirrorMakerConfig()
		mmConfig.Topics = ".*"
		err := mmConfig.Validate("connector-name")
		require.NoError(t, err)
	})

	t.Run("Zero replication factor fails", func(t *testing.T) {
		mmConfig := getMirrorMakerConfig()
		mmConfig.ReplicationFactor = 0
		err := mmConfig.Validate("connector-name")
		require.EqualError(t, err, "connector \"connector-name\" has too small replication factor: 0. minimum is 1")
	})

	t.Run("Two external clusters fails", func(t *testing.T) {
		mmConfig := getMirrorMakerConfig()
		mmConfig.TargetCluster = mmConfig.SourceCluster
		err := mmConfig.Validate("connector-name")
		require.EqualError(t, err, "cannot create connector without source or target type - this cluster")
	})

	t.Run("Two this clusters fails", func(t *testing.T) {
		mmConfig := getMirrorMakerConfig()
		mmConfig.SourceCluster = ClusterConnection{
			Type: kfmodels.ClusterConnectionTypeThisCluster,
		}
		mmConfig.TargetCluster = mmConfig.SourceCluster
		err := mmConfig.Validate("connector-name")
		require.EqualError(t, err, "cannot create connector with source and target type - this cluster")
	})
}

func create(x int64) *int64 {
	return &x
}

func TestTopicConfigMarshalJSON(t *testing.T) {
	t.Run("Empty pillar", func(t *testing.T) {
		tc := TopicConfig{}
		raw, err := json.Marshal(tc)
		require.NoError(t, err)
		require.Equal(t, "{}", string(raw))
	})

	t.Run("delete retention is 1", func(t *testing.T) {
		tc := TopicConfig{
			DeleteRetentionMs: create(1),
		}
		raw, err := json.Marshal(tc)
		require.NoError(t, err)
		require.Equal(t, "{\"delete.retention.ms\":1}", string(raw))
	})

	t.Run("delete retention is -1", func(t *testing.T) {
		tc := TopicConfig{
			DeleteRetentionMs: create(-1),
		}
		raw, err := json.Marshal(tc)
		require.NoError(t, err)
		require.Equal(t, "{\"delete.retention.ms\":-1}", string(raw))
	})

	t.Run("delete retention is -1", func(t *testing.T) {
		tc := TopicConfig{
			DeleteRetentionMs: create(0),
		}
		raw, err := json.Marshal(tc)
		require.NoError(t, err)
		require.Equal(t, "{\"delete.retention.ms\":0}", string(raw))
	})
}

func TestAccessSettingsToModel(t *testing.T) {
	t.Run("When structure is null should return empty structure", func(t *testing.T) {
		var accessSettings AccessSettings
		require.Equal(t, clusters.Access{}, accessSettings.ToModel())
	})

	t.Run("When structure is filled should return filled structure", func(t *testing.T) {
		settings := validAccessSettings()
		require.Equal(t, validClustersAccess(), settings.ToModel())
	})
}

func TestClusterSetAccess(t *testing.T) {
	t.Run("When structure is null should return empty structure", func(t *testing.T) {
		pillar := &Cluster{}
		pillar.SetAccess(clusters.Access{})

		require.Equal(t, AccessSettings{}, pillar.Data.Access)
	})

	t.Run("When structure is filled should return filled structure", func(t *testing.T) {
		pillar := &Cluster{}
		pillar.SetAccess(validClustersAccess())
		require.Equal(t, validAccessSettings(), pillar.Data.Access)
	})

	t.Run("Update of access settings should not override user network cidrs", func(t *testing.T) {
		boolTrue := true
		boolFalse := false

		pillar := &Cluster{
			Data: ClusterData{
				Access: AccessSettings{
					DataTransfer:     &boolFalse,
					Ipv4CidrBlocks:   []clusters.CidrBlock{{Value: "127.0.0.1/32"}},
					Ipv6CidrBlocks:   []clusters.CidrBlock{{Value: "::1/32"}},
					UserNetIPV4CIDRs: []string{"userNetIPV4CIDR_1", "userNetIPV4CIDR_2"},
					UserNetIPV6CIDRs: []string{"userNetIPV6CIDR_1", "userNetIPV6CIDR_2"},
				},
			},
		}
		pillar.SetAccess(validClustersAccess())

		require.Equal(t, AccessSettings{
			DataTransfer: &boolTrue,
			Ipv4CidrBlocks: []clusters.CidrBlock{
				{Value: "0.0.0.0/0", Description: "ipv4 description 1"},
				{Value: "192.168.0.0/0", Description: "ipv4 description 2"},
			},
			Ipv6CidrBlocks: []clusters.CidrBlock{
				{Value: "::/0", Description: "ipv6 description 1"},
				{Value: "::/1", Description: "ipv6 description 2"},
			},
			UserNetIPV4CIDRs: []string{"userNetIPV4CIDR_1", "userNetIPV4CIDR_2"},
			UserNetIPV6CIDRs: []string{"userNetIPV6CIDR_1", "userNetIPV6CIDR_2"},
		}, pillar.Data.Access)
	})
}

func validAccessSettings() AccessSettings {
	boolTrue := true
	return AccessSettings{
		DataTransfer: &boolTrue,
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
	}
}

func validClustersAccess() clusters.Access {
	return clusters.Access{
		DataTransfer: optional.NewBool(true),
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
	}
}

func TestEncryptionSettingsToModel(t *testing.T) {
	t.Run("When structure is null should return empty structure", func(t *testing.T) {
		var encryptionSettings EncryptionSettings
		require.Equal(t, clusters.Encryption{}, encryptionSettings.ToModel())
	})

	t.Run("When structure is filled should return filled structure", func(t *testing.T) {
		settings := EncryptionSettings{Enabled: ptr.Bool(false)}
		require.Equal(t, clusters.Encryption{Enabled: optional.NewBool(false)}, settings.ToModel())

		settings = EncryptionSettings{Enabled: ptr.Bool(true)}
		require.Equal(t, clusters.Encryption{Enabled: optional.NewBool(true)}, settings.ToModel())
	})
}

func TestClusterSetEncryption(t *testing.T) {
	t.Run("When structure is null should return empty structure", func(t *testing.T) {
		pillar := &Cluster{}
		pillar.SetEncryption(clusters.Encryption{})

		require.Equal(t, EncryptionSettings{}, pillar.Data.Encryption)
	})

	t.Run("When structure is filled should return filled structure", func(t *testing.T) {
		pillar := &Cluster{}
		pillar.SetEncryption(clusters.Encryption{Enabled: optional.NewBool(false)})
		require.Equal(t, EncryptionSettings{Enabled: ptr.Bool(false)}, pillar.Data.Encryption)

		pillar.SetEncryption(clusters.Encryption{Enabled: optional.NewBool(true)})
		require.Equal(t, EncryptionSettings{Enabled: ptr.Bool(true)}, pillar.Data.Encryption)
	})
}

func TestTopicConfigVersionMatches(t *testing.T) {
	testData := []struct {
		topicConfigVersion string
		kafkaVersion       string
		versionsMatches    bool
	}{
		{"", "blank", true},
		{kfmodels.Version2_8, kfmodels.Version2_8, true},
		{kfmodels.Version2_8, kfmodels.Version3_0, false},
		{kfmodels.Version3_0, kfmodels.Version3_0, true},
		{kfmodels.Version3_0, kfmodels.Version3_1, false},
		{kfmodels.Version3_1, kfmodels.Version3_1, true},
		{kfmodels.Version3_1, kfmodels.Version2_8, false},
		{kfmodels.СonfigVersion3, kfmodels.Version3_0, true},
		{kfmodels.СonfigVersion3, kfmodels.Version3_1, true},
		{kfmodels.СonfigVersion3, "3.7", false},
		{kfmodels.СonfigVersion3, "4.1", false},
		{kfmodels.СonfigVersion3, kfmodels.Version2_8, false},
	}

	for _, data := range testData {
		require.Equal(t, data.versionsMatches, topicConfigVersionMatches(data.topicConfigVersion, data.kafkaVersion))
	}
}
