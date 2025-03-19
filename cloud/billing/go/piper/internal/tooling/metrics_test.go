package tooling

import (
	"errors"
	"testing"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	dto "github.com/prometheus/client_model/go"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
)

type metricsTool struct{}

func (metricsTool) getMetricsFamily(c prometheus.Collector) *dto.MetricFamily {
	reg := prometheus.NewRegistry()
	reg.MustRegister(c)
	mt, err := reg.Gather()
	if err != nil {
		panic(err)
	}
	if len(mt) > 0 {
		return mt[0]
	}
	return nil
}

func (m metricsTool) getMetric(c prometheus.Collector) *dto.Metric {
	mf := m.getMetricsFamily(c)
	return mf.GetMetric()[0]
}

func (metricsTool) findLabel(lbs []*dto.LabelPair, name string) *dto.LabelPair {
	for _, l := range lbs {
		if l.GetName() == name {
			return l
		}
	}
	return nil
}

type actionMetricsSuite struct {
	baseSuite
	metricsTool
}

func TestActionMetricsSuite(t *testing.T) {
	suite.Run(t, new(actionMetricsSuite))
}

func (suite *actionMetricsSuite) TestStartAction() {
	ActionStarted(suite.ctx)

	m := suite.getMetric(metrics.ActionsStarted)
	suite.Require().EqualValues(1, m.Gauge.GetValue())
}

func (suite *actionMetricsSuite) TestDoneActionOk() {
	ActionDone(suite.ctx, nil)

	m := suite.getMetric(metrics.ActionsDone)
	suite.Require().EqualValues(1, m.Gauge.GetValue())
	l := suite.findLabel(m.Label, "success")
	suite.Require().NotNil(l)
	suite.Equal("yes", l.GetValue())
}

func (suite *actionMetricsSuite) TestDoneActionErr() {
	ActionDone(suite.ctx, errors.New("test error"))

	m := suite.getMetric(metrics.ActionsDone)
	suite.Require().EqualValues(1, m.Gauge.GetValue())
	l := suite.findLabel(m.Label, "success")
	suite.Require().NotNil(l)
	suite.Equal("no", l.GetValue())
}

type metricsCountSuite struct {
	baseSuite
	metricsTool
}

func TestInvalidMetricsSuite(t *testing.T) {
	suite.Run(t, new(metricsCountSuite))
}

func (suite *metricsCountSuite) TestIncoming() {
	IncomingMetricsCount(suite.ctx, 42)

	m := suite.getMetric(metrics.IncomingMetrics)
	suite.EqualValues(42, m.Gauge.GetValue())
}

func (suite *metricsCountSuite) TestProcessed() {
	ProcessedMetricsCount(suite.ctx, 42)

	m := suite.getMetric(metrics.ProcessedMetrics)
	suite.EqualValues(42, m.Gauge.GetValue())
}

func (suite *metricsCountSuite) TestEmpty() {
	InvalidMetrics(suite.ctx, nil)

	mf := suite.getMetricsFamily(metrics.InvalidMetrics)
	suite.Nil(mf)
}

func (suite *metricsCountSuite) TestWithReasons() {
	reasons := []entities.MetricFailReason{
		entities.FailedByBillingAccountResolving,
		entities.FailedByExpired,
		entities.FailedByFinishedAfterWrite,
		entities.FailedByInvalidModel,
		entities.FailedByInvalidTags,
		entities.FailedByNegativeQuantity,
		entities.FailedBySkuResolving,
		entities.FailedByUnparsedJSON,
		entities.FailedByTooBigChunk,
	}
	want := map[string]int{}
	var input []entities.InvalidMetric
	for i, reason := range reasons {
		var m entities.InvalidMetric
		m.Reason = reason
		want[reason.String()] = i + 1
		for j := 0; j <= i; j++ {
			input = append(input, m)
		}
	}
	InvalidMetrics(suite.ctx, input)

	mf := suite.getMetricsFamily(metrics.InvalidMetrics)
	suite.Require().Len(mf.Metric, len(reasons))

	for _, m := range mf.Metric {
		resLabel := suite.findLabel(m.Label, "reason")
		mRes := resLabel.GetValue()
		suite.EqualValues(want[mRes], m.Gauge.GetValue(), "reason=%s", mRes)
	}
}

type lagMetricsSuite struct {
	baseSuite
	metricsTool
}

func TestLagMetricsSuite(t *testing.T) {
	suite.Run(t, new(lagMetricsSuite))
}

func (suite *lagMetricsSuite) TestProcessingObserver() {
	observer := MetricSchemaUsageLagObserver(suite.ctx, "test_schema")

	suite.Require().NotNil(observer)
	observer.ObserveInt(100)

	m := suite.getMetric(metrics.ProducerProcessLag)
	suite.EqualValues(100, m.Histogram.GetSampleSum())
	suite.EqualValues(1, m.Histogram.GetSampleCount())

	schemaLabel := suite.findLabel(m.Label, "schema")
	suite.EqualValues("test_schema", schemaLabel.GetValue())
}

func (suite *lagMetricsSuite) TestWriteObserver() {
	observer := MetricSchemaWriteLagObserver(suite.ctx, "test_schema")

	suite.Require().NotNil(observer)
	observer.ObserveInt(100)

	m := suite.getMetric(metrics.ProducerWriteLag)
	suite.EqualValues(100, m.Histogram.GetSampleSum())
	suite.EqualValues(1, m.Histogram.GetSampleCount())

	schemaLabel := suite.findLabel(m.Label, "schema")
	suite.EqualValues("test_schema", schemaLabel.GetValue())
}

type cumulativeMetricsSuite struct {
	baseSuite
	metricsTool
}

func TestCumulativeMetricsSuite(t *testing.T) {
	suite.Run(t, new(cumulativeMetricsSuite))
}

func (suite *cumulativeMetricsSuite) TestDiffSize() {
	CumulativeDiffSize(suite.ctx, 999)

	m := suite.getMetric(metrics.CumulativeChanges)
	suite.EqualValues(999, m.Gauge.GetValue())
}

type queryMetricsSuite struct {
	baseSuite
	metricsTool
}

func TestQueryMetricsSuite(t *testing.T) {
	suite.Run(t, new(queryMetricsSuite))
}

func (suite *queryMetricsSuite) TestQueryOk() {
	ctx := QueryWithNameStarted(suite.ctx, "test query")

	{
		m := suite.getMetric(metrics.QueryStarted)
		suite.EqualValues(1, m.Gauge.GetValue())
		queryLabel := suite.findLabel(m.Label, "query_name")
		suite.EqualValues("test query", queryLabel.GetValue())
	}

	suite.clock.Advance(time.Second)
	QueryDone(ctx, nil)

	{
		m := suite.getMetric(metrics.QueryDuration)
		suite.EqualValues(1, m.Histogram.GetSampleCount())
		suite.EqualValues(1_000_000, m.Histogram.GetSampleSum())
		queryLabel := suite.findLabel(m.Label, "query_name")
		suite.EqualValues("test query", queryLabel.GetValue())
		successLabel := suite.findLabel(m.Label, "success")
		suite.EqualValues("yes", successLabel.GetValue())
	}
}

func (suite *queryMetricsSuite) TestQueryErr() {
	ctx := QueryWithNameStarted(suite.ctx, "test query")
	suite.clock.Advance(time.Second * 10)
	QueryDone(ctx, errors.New("test error"))

	{
		m := suite.getMetric(metrics.QueryDuration)
		suite.EqualValues(1, m.Histogram.GetSampleCount())
		suite.EqualValues(10_000_000, m.Histogram.GetSampleSum())
		queryLabel := suite.findLabel(m.Label, "query_name")
		suite.EqualValues("test query", queryLabel.GetValue())
		successLabel := suite.findLabel(m.Label, "success")
		suite.EqualValues("no", successLabel.GetValue())
	}
}

type icMetricsSuite struct {
	baseSuite
	metricsTool
}

func TestICMetricsSuite(t *testing.T) {
	suite.Run(t, new(icMetricsSuite))
}

func (suite *icMetricsSuite) TestRequestOk() {
	ctx := ICRequestWithNameStarted(suite.ctx, "test system", "test request")

	{
		m := suite.getMetric(metrics.InterconnectStarted)
		suite.EqualValues(1, m.Gauge.GetValue())
		systemLabel := suite.findLabel(m.Label, "dst_system")
		suite.EqualValues("test system", systemLabel.GetValue())
		requestLabel := suite.findLabel(m.Label, "request_name")
		suite.EqualValues("test request", requestLabel.GetValue())
	}

	suite.clock.Advance(time.Second)
	ICRequestDone(ctx, nil)

	{
		m := suite.getMetric(metrics.InterconnectDuration)
		suite.EqualValues(1, m.Histogram.GetSampleCount())
		suite.EqualValues(1_000_000, m.Histogram.GetSampleSum())
		systemLabel := suite.findLabel(m.Label, "dst_system")
		suite.EqualValues("test system", systemLabel.GetValue())
		requestLabel := suite.findLabel(m.Label, "request_name")
		suite.EqualValues("test request", requestLabel.GetValue())
		successLabel := suite.findLabel(m.Label, "success")
		suite.EqualValues("yes", successLabel.GetValue())
	}
}

func (suite *icMetricsSuite) TestRequestErr() {
	ctx := ICRequestWithNameStarted(suite.ctx, "test system", "test request")
	suite.clock.Advance(time.Second * 10)
	ICRequestDone(ctx, errors.New("test error"))

	{
		m := suite.getMetric(metrics.InterconnectDuration)
		suite.EqualValues(1, m.Histogram.GetSampleCount())
		suite.EqualValues(10_000_000, m.Histogram.GetSampleSum())
		systemLabel := suite.findLabel(m.Label, "dst_system")
		suite.EqualValues("test system", systemLabel.GetValue())
		requestLabel := suite.findLabel(m.Label, "request_name")
		suite.EqualValues("test request", requestLabel.GetValue())
		successLabel := suite.findLabel(m.Label, "success")
		suite.EqualValues("no", successLabel.GetValue())
	}
}
