package clickhouse

import (
	"testing"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/stretchr/testify/suite"
)

type retryTestSuite struct {
	baseSessionTestSuite

	remainTries int
}

func TestInvalid(t *testing.T) {
	suite.Run(t, new(retryTestSuite))
}

func (suite *retryTestSuite) SetupTest() {
	suite.baseSessionTestSuite.SetupTest()
	suite.session.backoffOverride = func() backoff.BackOff {
		return suite
	}
}

func (suite *retryTestSuite) TestOk() {
	err := suite.session.retryInsert(suite.ctx, func() error {
		return nil
	})
	suite.Require().NoError(err)
}

func (suite *retryTestSuite) TestGeneralError() {
	suite.remainTries = 10
	err := suite.session.retryInsert(suite.ctx, func() error {
		return errTest
	})
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, errTest)
	suite.Require().Zero(suite.remainTries)
}

func (suite *retryTestSuite) NextBackOff() time.Duration {
	if suite.remainTries <= 0 {
		return backoff.Stop
	}
	suite.remainTries--
	return time.Microsecond
}

func (suite *retryTestSuite) Reset() {}
