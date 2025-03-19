package iam

import (
	"context"
	"errors"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/iam/mocks"
)

var errTest = errors.New("test error")

type clientMockedSuite struct {
	suite.Suite

	ctx  context.Context
	mock *mocks.RMClient
}

func (suite *clientMockedSuite) SetupTest() {
	suite.ctx = context.TODO()
	suite.mock = &mocks.RMClient{}
}
