package actions

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions/mocks"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
)

type presenterTestSuite struct {
	suite.Suite
	presenterPusher mocks.PresenterPusher
}

func TestPresenter(t *testing.T) {
	suite.Run(t, new(presenterTestSuite))
}

func (suite *presenterTestSuite) SetupTest() {
	suite.presenterPusher = mocks.PresenterPusher{}
}

func (suite *presenterTestSuite) TestPushPresenterMetrics() {
	metric := entities.PresenterMetric{}
	metrics := []entities.PresenterMetric{metric}

	suite.presenterPusher.On("PushPresenterMetric", mock.Anything, mock.Anything, metric).
		Return(nil)
	suite.presenterPusher.On("FlushPresenterMetric", mock.Anything).Return(nil)

	err := PushPresenterMetrics(context.Background(), entities.ProcessingScope{}, &suite.presenterPusher, metrics)

	suite.Require().NoError(err)
	suite.presenterPusher.AssertExpectations(suite.T())
}
