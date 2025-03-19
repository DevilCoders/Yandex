package scope

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/metrics"
)

type globalTestSuite struct {
	baseTestSuite
}

func TestGlobal(t *testing.T) {
	suite.Run(t, new(globalTestSuite))
}

func (suite *globalTestSuite) TestStartGlobalScope() {
	StartGlobal(suite.logger)

	suite.Require().NotNil(globalScopeInstance)
	suite.Require().Equal(globalScopeInstance.Logger(), suite.logger)
}

func (suite *globalTestSuite) TestStartGlobalMetrics() {
	StartGlobal(suite.logger)

	appAlive := suite.getMetric(metrics.AppAlive)
	suite.EqualValues(1, appAlive.Counter.GetValue())
}

func (suite *globalTestSuite) TestFinishGlobalScope() {
	StartGlobal(suite.logger)
	FinishGlobal()

	suite.Require().Nil(globalScopeInstance)
}

func (suite *globalTestSuite) TestFinishGlobalMetrics() {
	StartGlobal(suite.logger)
	FinishGlobal()

	suite.Len(suite.getMetricsFamily(metrics.AppAlive).GetMetric(), 0)
}
