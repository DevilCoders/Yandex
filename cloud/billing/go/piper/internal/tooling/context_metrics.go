package tooling

import (
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
)

func (s *ctxStoreCommon) actionStarLabels() []string {
	return metrics.ActionStartLabels(s.service, s.sourceShort, s.action)
}

func (s *ctxStoreCommon) actionDoneLabels(success bool) []string {
	return metrics.ActionDoneLabels(s.service, s.sourceShort, s.action, success)
}

func (s *ctxStore) incomingMetricsLabels() []string {
	return metrics.IncomingMetricsLabels(s.service, s.sourceShort, s.sourcePartition)
}

func (s *ctxStore) invalidMetricsLabels(reason string) []string {
	return metrics.InvalidMetricsLabels(s.service, s.sourceShort, s.sourcePartition, reason)
}

func (s *ctxStore) processedMetricsLabels() []string {
	return metrics.ProcessedMetricsLabels(s.service, s.sourceShort, s.sourcePartition)
}

func (s *ctxStore) schemaLagLabels(schema string) []string {
	return metrics.SchemaLagMetrics(s.service, s.sourceShort, schema)
}

func (s *ctxStore) cumulativeChangesLabels() []string {
	return metrics.CumulativeChangesLabels(s.service, s.sourceShort)
}

func (s *ctxStore) queryStartLabels() []string {
	return metrics.QueryStartLabels(s.dbQueryName)
}

func (s *ctxStore) queryDoneLabels(success bool) []string {
	return metrics.QueryDoneLabels(s.dbQueryName, success)
}

func (s *ctxStore) queryDurationMicroseconds() float64 {
	return float64(s.getClock().Since(s.dbQueryCallTime).Microseconds())
}

func (s *ctxStore) icStartLabels() []string {
	return metrics.InterconnectStartLabels(s.icSystem, s.icRequest)
}

func (s *ctxStore) icDoneLabels(success bool) []string {
	return metrics.InterconnectDoneLabels(s.icSystem, s.icRequest, success)
}

func (s *ctxStore) icDurationMicroseconds() float64 {
	return float64(s.getClock().Since(s.icRequestCallTime).Microseconds())
}
