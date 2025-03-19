package states

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"
)

type statsProviderTestSuite struct {
	suite.Suite
}

func TestStatsProvider(t *testing.T) {
	suite.Run(t, new(statsProviderTestSuite))
}

func (suite *statsProviderTestSuite) TestProvider() {
	prov := getStatsProvider(dummyGood{})
	suite.Require().NotNil(prov)

	suite.EqualValues("stats", prov())
}

func (suite *statsProviderTestSuite) TestNoProviderByOut() {
	prov := getStatsProvider(dummyIncorrectOut{})
	suite.Require().Nil(prov)
}

func (suite *statsProviderTestSuite) TestNoProviderByIn() {
	prov := getStatsProvider(dummyIncorrectIn{})
	suite.Require().Nil(prov)
}

func (suite *statsProviderTestSuite) TestNoProvider() {
	prov := getStatsProvider(struct{}{})
	suite.Require().Nil(prov)
}

type healthCheckerTestSuite struct {
	suite.Suite
}

func TestHealthChecker(t *testing.T) {
	suite.Run(t, new(healthCheckerTestSuite))
}

func (suite *healthCheckerTestSuite) TestHasInterface() {
	prov := getHealthChecker(dummyGood{})
	suite.Require().NotNil(prov)
}

type closerTestSuite struct {
	suite.Suite
}

func TestCloser(t *testing.T) {
	suite.Run(t, new(closerTestSuite))
}

func (suite *closerTestSuite) TestHasInterface() {
	prov := getCloseCall(dummyGood{})
	suite.Require().NotNil(prov)
}

func (suite *closerTestSuite) TestHasInterfaceWithoutErr() {
	prov := getCloseCall(dummyCloseNoErr{})
	suite.Require().NotNil(prov)
}

func (suite *healthCheckerTestSuite) TestNoInterface() {
	prov := getCloseCall(struct{}{})
	suite.Require().Nil(prov)
}

type dummyGood struct{}

func (dummyGood) GetStats() string                  { return "stats" }
func (dummyGood) HealthCheck(context.Context) error { return nil }
func (dummyGood) Close() error                      { return nil }

type dummyIncorrectOut struct{}

func (s dummyIncorrectOut) GetStats() (string, error) { return "stats", nil }

type dummyIncorrectIn struct{}

func (s dummyIncorrectIn) GetStats(string) string { return "stats" }

type dummyCloseNoErr struct{}

func (dummyCloseNoErr) Close() {}
