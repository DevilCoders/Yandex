package ydb

import (
	"context"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	testConnectionTimepout = 10 * time.Second
	testQueryTimeout       = 5 * time.Second
)

var (
	testYDBEndpoint = os.Getenv("YDB_ENDPOINT")
	testYDBDatabase = os.Getenv("YDB_DATABASE")
)

func TestYDBConnection(t *testing.T) {
	suite.Run(t, new(YDBConnectionTestSuite))
}

type YDBConnectionTestSuite struct {
	suite.Suite

	dbConnection *Connector

	logger log.Logger
}

func (suite *YDBConnectionTestSuite) SetupSuite() {
	suite.logger = &zap.Logger{
		L: zaptest.NewLogger(suite.T()),
	}
}

func (suite *YDBConnectionTestSuite) TearDownTest() {
	if suite.dbConnection == nil {
		return
	}

	err := suite.dbConnection.Close()
	suite.Require().NoError(err)
}

func (suite *YDBConnectionTestSuite) TestConnectionOk() {
	var err error

	connectCtx, cancel := context.WithTimeout(context.Background(), testConnectionTimepout)
	defer cancel()

	suite.dbConnection, err = Connect(connectCtx,
		WithEndpoint(testYDBEndpoint),
		WithDatabase(testYDBDatabase),
		WithLogger(suite.logger),
	)

	suite.Require().NoError(err)
	suite.Require().NotNil(suite.dbConnection)

	err = suite.dbConnection.db.PingContext(connectCtx)
	suite.Require().NoError(err)
}

func (suite *YDBConnectionTestSuite) TestConnectionWithoutLoggerError() {
	var err error

	connectCtx, cancel := context.WithTimeout(context.Background(), testConnectionTimepout)
	defer cancel()

	suite.dbConnection, err = Connect(connectCtx,
		WithEndpoint(testYDBEndpoint),
		WithDatabase(testYDBDatabase),
	)

	suite.Require().Error(err)
	suite.Require().Nil(suite.dbConnection)
}
