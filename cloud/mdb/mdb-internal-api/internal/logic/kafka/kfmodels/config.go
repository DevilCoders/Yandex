package kfmodels

import (
	"reflect"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/validation"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type MDBClusterSpec struct {
	Version         string
	Kafka           KafkaConfigSpec
	BrokersCount    int64
	ZooKeeper       ZookeperConfigSpec
	ZoneID          []string
	AssignPublicIP  bool
	UnmanagedTopics bool
	SchemaRegistry  bool
	SyncTopics      bool
	Access          Access
}

type DataCloudClusterSpec struct {
	Version      string
	Kafka        KafkaConfigSpec
	BrokersCount int64
	ZoneCount    int64
	Access       clusters.Access
	Encryption   clusters.Encryption
}

type ClusterConfigSpec struct {
	Version         string
	Kafka           KafkaConfigSpec
	BrokersCount    int64
	ZooKeeper       ZookeperConfigSpec
	ZoneID          []string
	AssignPublicIP  bool
	UnmanagedTopics bool
	SchemaRegistry  bool
	SyncTopics      bool
	Access          clusters.Access
	Encryption      clusters.Encryption
}

func (cs ClusterConfigSpec) Validate() error {
	if err := cs.Kafka.Validate(); err != nil {
		return err
	}
	return cs.ZooKeeper.Validate()
}

type ClusterConfigSpecMDBUpdate struct {
	Version         optional.String
	Kafka           KafkaConfigSpecUpdate
	ZooKeeper       ZookeeperConfigSpecUpdate
	ZoneID          optional.Strings
	BrokersCount    optional.Int64
	AssignPublicIP  optional.Bool
	UnmanagedTopics optional.Bool
	SchemaRegistry  optional.Bool
	Access          clusters.Access
}

type ClusterConfigSpecDataCloudUpdate struct {
	Version      optional.String
	Kafka        KafkaConfigSpecUpdate
	BrokersCount optional.Int64
	ZoneCount    optional.Int64
	Access       clusters.Access
}

type KafkaConfig struct {
	CompressionType             string
	LogFlushIntervalMessages    *int64
	LogFlushIntervalMs          *int64
	LogFlushSchedulerIntervalMs *int64
	LogRetentionBytes           *int64
	LogRetentionHours           *int64
	LogRetentionMinutes         *int64
	LogRetentionMs              *int64
	LogSegmentBytes             *int64
	LogPreallocate              *bool
	SocketSendBufferBytes       *int64
	SocketReceiveBufferBytes    *int64
	AutoCreateTopicsEnable      *bool
	NumPartitions               *int64
	DefaultReplicationFactor    *int64
	MessageMaxBytes             *int64
	ReplicaFetchMaxBytes        *int64
	SslCipherSuites             []string
	OffsetsRetentionMinutes     *int64
}

func KafkaConfigIsEmpty(config KafkaConfig) bool {
	if config.SslCipherSuites != nil {
		if len(config.SslCipherSuites) > 0 {
			return false
		} else {
			config.SslCipherSuites = nil
		}
	}
	return reflect.DeepEqual(config, KafkaConfig{})
}

type KafkaConfigUpdate struct {
	CompressionType             optional.String
	LogFlushIntervalMessages    optional.Int64Pointer
	LogFlushIntervalMs          optional.Int64Pointer
	LogFlushSchedulerIntervalMs optional.Int64Pointer
	LogRetentionBytes           optional.Int64Pointer
	LogRetentionHours           optional.Int64Pointer
	LogRetentionMinutes         optional.Int64Pointer
	LogRetentionMs              optional.Int64Pointer
	LogSegmentBytes             optional.Int64Pointer
	LogPreallocate              optional.BoolPointer
	SocketSendBufferBytes       optional.Int64Pointer
	SocketReceiveBufferBytes    optional.Int64Pointer
	AutoCreateTopicsEnable      optional.BoolPointer
	NumPartitions               optional.Int64Pointer
	DefaultReplicationFactor    optional.Int64Pointer
	MessageMaxBytes             optional.Int64Pointer
	ReplicaFetchMaxBytes        optional.Int64Pointer
	SslCipherSuites             optional.Strings
	OffsetsRetentionMinutes     optional.Int64Pointer
}

type KafkaConfigSpec struct {
	Resources models.ClusterResources
	Config    KafkaConfig
}

func (ks KafkaConfigSpec) Validate() error {
	if err := ks.Config.Validate(); err != nil {
		return err
	}
	return ks.Resources.Validate()
}

func (kc KafkaConfig) Validate() error {
	if kc.LogPreallocate != nil && *kc.LogPreallocate {
		return semerr.InvalidInput("preallocate flag is disabled due the kafka issue KAFKA-13664")
	}

	if kc.SslCipherSuites != nil && len(kc.SslCipherSuites) > 0 {
		return validation.IsValidSslCipherSuitesSlice(kc.SslCipherSuites)
	}
	return nil
}

type KafkaConfigSpecUpdate struct {
	Resources models.ClusterResourcesSpec
	Config    KafkaConfigUpdate
}

type ZookeperConfigSpec struct {
	Resources models.ClusterResources
}

func (zs ZookeperConfigSpec) Validate() error {
	return zs.Resources.Validate()
}

type ZookeeperConfigSpecUpdate struct {
	Resources models.ClusterResourcesSpec
}

type Access struct {
	DataTransfer optional.Bool
	WebSQL       optional.Bool
	Serverless   optional.Bool
}
