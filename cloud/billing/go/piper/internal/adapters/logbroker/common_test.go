package logbroker

import (
	"context"
	"errors"

	"github.com/stretchr/testify/suite"
)

//go:generate mockery --testonly  --name ShardProducer --structname mockShardProducer --dir ../../../../pkg/logbroker/lbtypes --output . --outpkg logbroker

var errTest = errors.New("test error")

type clientMockedSuite struct {
	suite.Suite

	ctx           context.Context
	mockIncorrect *mockShardProducer
	mockResharded *mockShardProducer
}

func (suite *clientMockedSuite) SetupTest() {
	suite.ctx = context.TODO()
	suite.mockIncorrect = &mockShardProducer{}
	suite.mockResharded = &mockShardProducer{}
}
