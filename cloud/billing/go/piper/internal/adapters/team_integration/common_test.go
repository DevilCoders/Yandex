package teamintegration

import (
	"context"
	"errors"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/team_integration/mocks"
)

var errTest = errors.New("test error")

type clientMockedSuite struct {
	suite.Suite

	ctx  context.Context
	mock *mocks.TIClient
}

func (suite *clientMockedSuite) SetupTest() {
	suite.ctx = context.TODO()
	suite.mock = &mocks.TIClient{}
}
