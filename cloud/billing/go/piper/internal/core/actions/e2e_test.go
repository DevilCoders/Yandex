package actions

import (
	"context"
	"github.com/stretchr/testify/mock"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions/mocks"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
)

type e2eTestSuite struct {
	suite.Suite
	e2eQuantityPusher mocks.E2EQuantityPusher
}

func TestE2E(t *testing.T) {
	suite.Run(t, new(e2eTestSuite))
}

func (suite *e2eTestSuite) SetupTest() {
	suite.e2eQuantityPusher = mocks.E2EQuantityPusher{}
}

func (suite *e2eTestSuite) TestReportE2EQuantityWithoutE2ELabel() {
	metric := entities.EnrichedMetric{}
	metric.Labels.User = map[string]string{"some_user_label": "test_value"}
	metrics := []entities.EnrichedMetric{metric}

	suite.e2eQuantityPusher.On("FlushE2EQuantityMetrics", mock.Anything).Return(nil)

	err := ReportE2EQuantity(context.Background(), entities.ProcessingScope{}, &suite.e2eQuantityPusher, metrics)

	suite.Require().NoError(err)
	suite.e2eQuantityPusher.AssertExpectations(suite.T())
}

func (suite *e2eTestSuite) TestReportE2EQuantity() {
	metric := entities.EnrichedMetric{}
	metric.Labels.User = map[string]string{e2eLabelName: "test_value"}
	metrics := []entities.EnrichedMetric{metric}

	suite.e2eQuantityPusher.On("PushE2EUsageQuantityMetric", mock.Anything, mock.Anything, mock.Anything).
		Return(nil)
	suite.e2eQuantityPusher.On("PushE2EPricingQuantityMetric", mock.Anything, mock.Anything, mock.Anything).
		Return(nil)
	suite.e2eQuantityPusher.On("FlushE2EQuantityMetrics", mock.Anything).Return(nil)

	err := ReportE2EQuantity(context.Background(), entities.ProcessingScope{}, &suite.e2eQuantityPusher, metrics)

	suite.Require().NoError(err)
	suite.e2eQuantityPusher.AssertExpectations(suite.T())
}
