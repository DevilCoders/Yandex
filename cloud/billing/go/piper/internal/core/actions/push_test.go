package actions

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions/mocks"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
)

type pushTestSuite struct {
	suite.Suite
	metricsPusher           mocks.MetricsPusher
	invalidMetricsPusher    mocks.InvalidMetricsPusher
	oversizedMessagesPusher mocks.OversizedMessagePusher
	incorrectMetricsDumper  mocks.IncorrectMetricDumper
}

func TestPush(t *testing.T) {
	suite.Run(t, new(pushTestSuite))
}

func (suite *pushTestSuite) SetupTest() {
	suite.metricsPusher = mocks.MetricsPusher{}
	suite.invalidMetricsPusher = mocks.InvalidMetricsPusher{}
	suite.oversizedMessagesPusher = mocks.OversizedMessagePusher{}
	suite.incorrectMetricsDumper = mocks.IncorrectMetricDumper{}
}

func (suite *pushTestSuite) TestPushEnrichedMetrics() {
	metric := entities.EnrichedMetric{}
	metrics := []entities.EnrichedMetric{metric}

	suite.metricsPusher.On("EnrichedMetricPartitions").Return(1)
	suite.metricsPusher.On("FlushEnriched", mock.Anything).Return(nil)
	suite.metricsPusher.On("PushEnrichedMetricToPartition", mock.Anything, mock.Anything, metric, 0).
		Return(nil)

	err := PushEnrichedMetrics(context.Background(), entities.ProcessingScope{}, &suite.metricsPusher, metrics)

	suite.Require().NoError(err)
	suite.metricsPusher.AssertExpectations(suite.T())
}

func (suite *pushTestSuite) TestPushEnrichedMetricsAndMultiplePartitions() {
	metric := entities.EnrichedMetric{}
	metrics := []entities.EnrichedMetric{metric}

	suite.metricsPusher.On("EnrichedMetricPartitions").Return(2)
	suite.metricsPusher.On("FlushEnriched", mock.Anything).Return(nil)
	suite.metricsPusher.On("PushEnrichedMetricToPartition", mock.Anything, mock.Anything, metric, 1).
		Return(nil)

	err := PushEnrichedMetrics(context.Background(), entities.ProcessingScope{}, &suite.metricsPusher, metrics)

	suite.Require().NoError(err)
	suite.metricsPusher.AssertExpectations(suite.T())
}

func (suite *pushTestSuite) TestPushInvalidMetrics() {
	metric := entities.InvalidMetric{}
	metrics := []entities.InvalidMetric{metric}

	suite.invalidMetricsPusher.On("PushInvalidMetric", mock.Anything, mock.Anything, metric).
		Return(nil)
	suite.invalidMetricsPusher.On("FlushInvalidMetrics", mock.Anything).Return(nil)

	err := PushInvalidMetrics(context.Background(), entities.ProcessingScope{}, &suite.invalidMetricsPusher, metrics)

	suite.Require().NoError(err)
	suite.metricsPusher.AssertExpectations(suite.T())
}

func (suite *pushTestSuite) TestPushOversizedMetrics() {
	metric := entities.InvalidMetric{}
	metrics := []entities.InvalidMetric{metric}

	suite.oversizedMessagesPusher.On("PushOversizedMessage", mock.Anything, mock.Anything, metric.IncorrectRawMessage).
		Return(nil)
	suite.oversizedMessagesPusher.On("FlushOversized", mock.Anything).Return(nil)

	err := PushOversizedMessages(context.Background(), entities.ProcessingScope{}, &suite.oversizedMessagesPusher, metrics)

	suite.Require().NoError(err)
	suite.metricsPusher.AssertExpectations(suite.T())
}

func (suite *pushTestSuite) TestDumpInvalidMetrics() {
	metric := entities.IncorrectMetricDump{}
	metrics := []entities.IncorrectMetricDump{metric}

	suite.incorrectMetricsDumper.On("PushIncorrectDump", mock.Anything, mock.Anything, metric).
		Return(nil)
	suite.incorrectMetricsDumper.On("FlushIncorrectDumps", mock.Anything).Return(nil)

	err := DumpInvalidMetrics(context.Background(), entities.ProcessingScope{}, &suite.incorrectMetricsDumper, metrics)

	suite.Require().NoError(err)
	suite.metricsPusher.AssertExpectations(suite.T())
}
