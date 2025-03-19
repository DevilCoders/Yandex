package requestid

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type UtilsTestSuite struct {
	suite.Suite
}

func TestUtils(t *testing.T) {
	suite.Run(t, new(UtilsTestSuite))
}

func (suite *UtilsTestSuite) TestIsValidUUIDOk() {
	s := "65356fa2-43bd-11ec-81d3-0242ac130003"
	u, err := parseValidUUID(s)
	suite.Require().NoError(err)
	suite.Require().Equal(s, u)

	s = "1724edd8-d8c3-4997-8db3-0deb85f55336"
	u, err = parseValidUUID(s)
	suite.Require().NoError(err)
	suite.Require().Equal(s, u)

	s = "1724edd8d8c349978db30deb85f55336"
	u, err = parseValidUUID(s)
	suite.Require().NoError(err)
	suite.Require().Equal("1724edd8-d8c3-4997-8db3-0deb85f55336", u)
}

func (suite *UtilsTestSuite) TestIsValidUUIDErr() {
	_, err := parseValidUUID("foo")
	suite.Require().Error(err)

	_, err = parseValidUUID("1724edd8-d8c3-4997-8db3")
	suite.Require().Error(err)
}
