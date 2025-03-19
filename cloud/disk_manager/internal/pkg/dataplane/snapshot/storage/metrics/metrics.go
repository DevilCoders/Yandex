package metrics

import (
	common_metrics "a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
)

////////////////////////////////////////////////////////////////////////////////

type Metrics interface {
	StatOperation(name string) func(err *error)
	OnChunkCompressed(origSize int, compressedSize int)
}

func New(registry common_metrics.Registry) Metrics {
	return &storageMetricsImpl{
		registry:         registry,
		operationMetrics: make(map[string]*operationStats),
		chunkCompressionRatio: registry.Histogram(
			"chunkCompressionRatio",
			common_metrics.MakeExponentialBuckets(1, 1.5, 10)),
	}
}
