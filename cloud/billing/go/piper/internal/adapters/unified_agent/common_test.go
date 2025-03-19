package unifiedagent

import (
	"context"
	"errors"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/unified_agent/mocks"
)

var errTest = errors.New("test error")

type clientMockedSuite struct {
	suite.Suite

	ctx  context.Context
	mock *mocks.UAClient
}

func (suite *clientMockedSuite) SetupTest() {
	suite.ctx = context.TODO()
	suite.mock = &mocks.UAClient{}
}
