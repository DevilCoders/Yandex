package models

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

type LogLevel string

const (
	LogLevelTrace   LogLevel = "Trace"
	LogLevelDebug   LogLevel = "Debug"
	LogLevelInfo    LogLevel = "Info"
	LogLevelWarning LogLevel = "Warning"
	LogLevelError   LogLevel = "Error"
	LogLevelFatal   LogLevel = "Fatal"
)

type LogSourceType int32

const (
	LogSourceTypeClickhouse LogSourceType = iota
	LogSourceTypeKafka
	LogSourceTypeTransfer
)

var (
	typeMapping = map[LogSourceType]struct {
		ClusterType metadb.ClusterType
		Name        string
	}{
		LogSourceTypeClickhouse: {
			ClusterType: metadb.ClickhouseCluster,
			Name:        "ClickHouse",
		},
		LogSourceTypeKafka: {
			ClusterType: metadb.KafkaCluster,
			Name:        "Kafka",
		},
		LogSourceTypeTransfer: {
			Name: "Data Transfer",
		},
	}
)

func (t LogSourceType) ClusterType() metadb.ClusterType {
	return typeMapping[t].ClusterType
}

func (t LogSourceType) Stringify() string {
	return typeMapping[t].Name
}

type LogSource struct {
	Type LogSourceType `json:"type"`
	ID   string        `json:"id"`
}

type Log struct {
	Source    LogSource
	Instance  optional.String
	Timestamp time.Time
	Severity  LogLevel
	Message   string

	Offset int64
}
