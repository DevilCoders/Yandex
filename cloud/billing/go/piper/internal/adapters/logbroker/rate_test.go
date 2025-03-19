package logbroker

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
)

type rateTestSuite struct {
	suite.Suite

	ctx    context.Context
	cancel context.CancelFunc

	l limiter
}

func TestRate(t *testing.T) {
	suite.Run(t, new(rateTestSuite))
}

func (suite *rateTestSuite) SetupTest() {
	suite.ctx, suite.cancel = context.WithTimeout(context.TODO(), time.Second)
	suite.l = limiter{}
}

func (suite *rateTestSuite) TearDownTest() {
	suite.cancel()
}

func (suite *rateTestSuite) TestNoLimit() {
	suite.Require().NoError(suite.l.Acquire(suite.ctx, 1000000000))
}

func (suite *rateTestSuite) TestInSize() {
	suite.l.SetSize(10)
	suite.Require().NoError(suite.l.Acquire(suite.ctx, 10))
}

func (suite *rateTestSuite) TestOversize() {
	suite.l.SetSize(1)
	suite.Require().NoError(suite.l.Acquire(suite.ctx, 1000000000))
}

func (suite *rateTestSuite) TestLock() {
	suite.l.SetSize(3)

	suite.Require().NoError(suite.l.Acquire(suite.ctx, 3))
	locked := []bool{false, false, false}
	for i := range locked {
		go func(i int) {
			_ = suite.l.Acquire(suite.ctx, 1)
			locked[i] = true
		}(i)
	}

	time.Sleep(time.Millisecond)
	suite.NotContains(locked, true)
	suite.l.Release(3)
	time.Sleep(time.Millisecond)
	suite.NotContains(locked, false)
}

func (suite *rateTestSuite) TestCtxCancel() {
	suite.l.SetSize(1)

	_ = suite.l.Acquire(suite.ctx, 1)

	ctx, cancel := context.WithTimeout(suite.ctx, time.Millisecond*5)
	defer cancel()

	suite.Require().Error(suite.l.Acquire(ctx, 1))
}

func (suite *rateTestSuite) TestReleaseByResize() {
	suite.l.SetSize(1)

	_ = suite.l.Acquire(suite.ctx, 1)

	locked := false
	go func() {
		_ = suite.l.Acquire(suite.ctx, 1)
		locked = true
	}()

	time.Sleep(time.Millisecond)
	suite.False(locked)
	suite.l.SetSize(2)
	time.Sleep(time.Millisecond)
	suite.True(locked)
}

func (suite *rateTestSuite) TestReleaseAfterResize() {
	suite.l.SetSize(99)

	_ = suite.l.Acquire(suite.ctx, 99)
	suite.l.SetSize(1)
	suite.l.Release(99) // semaphore will panic on counter goes below 0, we will keep working
}
