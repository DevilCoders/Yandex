package logbroker

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
)

type countersSuite struct {
	suite.Suite

	c *counters
}

func TestCounters(t *testing.T) {
	suite.Run(t, new(countersSuite))
}

func (suite *countersSuite) SetupTest() {
	suite.c = &counters{}
	suite.c.init()
}

func (suite *countersSuite) TearDownTest() {
	suite.c.shutdown()
}

func (suite *countersSuite) TestShutdown() {
	for i := 0; i < 1000; i++ {
		suite.c.handleStart()
	}
	suite.c.shutdown()
	v, _ := suite.c.getValues()
	suite.Equal(1000, v.handleCalls)
}

func (suite *countersSuite) TestPQ() {
	g := pqGetter(persqueue.Stat{MemUsage: 999})
	suite.c.setPQ(g)

	_, s := suite.c.getValues()
	suite.EqualValues(g, s)

	suite.c.resetPQ()

	_, s = suite.c.getValues()
	suite.EqualValues(persqueue.Stat{}, s)
}

func (suite *countersSuite) TestRestarted() {
	suite.c.restarted()

	suite.c.shutdown()
	v, _ := suite.c.getValues()

	suite.Equal(countersValues{restarts: 1}, v)
}

func (suite *countersSuite) TestSuspended() {
	suite.c.suspended()

	suite.c.shutdown()
	v, _ := suite.c.getValues()

	suite.Equal(countersValues{suspends: 1}, v)
}

func (suite *countersSuite) TestReaderReseted() {
	suite.c.lockUpdated(10, 100)
	suite.c.inflyUpdated(20, 200)

	suite.c.readerReseted()

	suite.c.shutdown()
	v, _ := suite.c.getValues()

	suite.Equal(countersValues{locks: 100, readMessages: 200}, v)
}

func (suite *countersSuite) TestHandling() {
	for i := 0; i < 1; i++ {
		suite.c.handleStart()
	}
	for i := 0; i < 10; i++ {
		suite.c.handleCancel()
	}
	for i := 0; i < 100; i++ {
		suite.c.handleFailed()
	}
	for i := 0; i < 1000; i++ {
		suite.c.handleDone()
	}

	suite.c.shutdown()
	v, _ := suite.c.getValues()

	suite.Equal(countersValues{
		handleCalls:         1,
		handleCancellations: 10,
		handleFailures:      100,
		handled:             1000,
	}, v)
}

type pqGetter persqueue.Stat

func (p pqGetter) Stat() persqueue.Stat {
	return persqueue.Stat(p)
}
