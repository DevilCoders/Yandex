package teamintegration

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type adapterSuite struct {
	clientMockedSuite
}

func TestAdapter(t *testing.T) {
	suite.Run(t, new(adapterSuite))
}

func (suite *adapterSuite) TestNew() {
	adapter, err := New(suite.ctx, suite.mock)
	suite.Require().NoError(err)
	suite.NotNil(adapter)
}
