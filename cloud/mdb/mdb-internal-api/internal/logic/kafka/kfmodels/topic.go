package kfmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

const (
	CleanupPolicyUnspecified      string = ""
	CleanupPolicyDelete           string = "delete"
	CleanupPolicyCompact          string = "compact"
	CleanupPolicyCompactAndDelete string = "delete,compact"
)

type TopicConfig struct {
	Version            string
	CleanupPolicy      string
	CompressionType    string
	DeleteRetentionMs  *int64
	FileDeleteDelayMs  *int64
	FlushMessages      *int64
	FlushMs            *int64
	MinCompactionLagMs *int64
	RetentionBytes     *int64
	RetentionMs        *int64
	MaxMessageBytes    *int64
	MinInsyncReplicas  *int64
	SegmentBytes       *int64
	Preallocate        *bool
}

type Topic struct {
	ClusterID         string
	Name              string
	Partitions        int64
	ReplicationFactor int64
	Config            TopicConfig
}

type TopicSpec struct {
	Name              string
	Partitions        int64
	ReplicationFactor int64
	Config            TopicConfig
}

type TopicSpecUpdate struct {
	Partitions        optional.Int64
	ReplicationFactor optional.Int64
	Config            TopicConfigUpdate
}

type TopicConfigUpdate struct {
	CleanupPolicy      optional.String
	CompressionType    optional.String
	DeleteRetentionMs  optional.Int64Pointer
	FileDeleteDelayMs  optional.Int64Pointer
	FlushMessages      optional.Int64Pointer
	FlushMs            optional.Int64Pointer
	MinCompactionLagMs optional.Int64Pointer
	RetentionBytes     optional.Int64Pointer
	RetentionMs        optional.Int64Pointer
	MaxMessageBytes    optional.Int64Pointer
	MinInsyncReplicas  optional.Int64Pointer
	SegmentBytes       optional.Int64Pointer
	Preallocate        optional.BoolPointer
}

var TopicBlackList = []string{
	"__consumer_offsets",
	"__transaction_state",
	"__connect-configs",
	"__connect-offsets",
	"__connect-status",
}
var TopicNameValidator = models.MustTopicNameValidator(models.DefaultTopicNamePattern, TopicBlackList)

func (ts TopicSpec) Validate() error {
	if ts.Config.Preallocate != nil && *ts.Config.Preallocate {
		return semerr.InvalidInput("preallocate flag is disabled due the kafka issue KAFKA-13664")
	}
	return TopicNameValidator.ValidateString(ts.Name)
}
