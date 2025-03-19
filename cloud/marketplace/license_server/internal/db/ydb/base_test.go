package ydb

import (
	"context"
	"os"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/migrations"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
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

type BaseYDBTestSuite struct {
	suite.Suite

	connector *ydb.Connector
	provider  LicenseServerProvider
	migrator  *migrations.Migrator

	logger log.Logger
}

func (suite *BaseYDBTestSuite) SetupSuite() {
	var err error

	suite.logger = &zap.Logger{
		L: zaptest.NewLogger(suite.T()),
	}

	connectCtx, cancel := context.WithTimeout(context.Background(), testConnectionTimepout)
	defer cancel()

	suite.connector, err = ydb.Connect(connectCtx,
		ydb.WithDBRoot("license_server"),
		ydb.WithEndpoint(testYDBEndpoint),
		ydb.WithDatabase(testYDBDatabase),
		ydb.WithLogger(suite.logger),
	)

	suite.Require().NoError(err)
	suite.Require().NotNil(suite.connector)

	suite.provider = NewLicenseServerProvider(suite.connector)
	suite.migrator = migrations.NewMigrator(suite.connector)

	err = suite.createDatabase()
	suite.Require().NoError(err)
}

func (suite *BaseYDBTestSuite) TearDownSuite() {
	err := suite.dropDatabase()
	suite.Require().NoError(err)

	err = suite.connector.Close()
	suite.Require().NoError(err)
}

func (suite *BaseYDBTestSuite) TearDownTest() {
	err := suite.purgeDatabase()
	suite.Require().NoError(err)
}

func (suite *BaseYDBTestSuite) createDatabase() error {
	err := suite.migrator.InitUp(context.Background())
	if err != nil {
		return err
	}
	return suite.migrator.Up(context.Background())
}

func (suite *BaseYDBTestSuite) dropDatabase() error {
	err := suite.migrator.DownAll(context.Background())
	if err != nil {
		return err
	}

	return suite.migrator.InitDown(context.Background())
}

func (suite *BaseYDBTestSuite) purgeDatabase() error {
	err := suite.dropDatabase()
	if err != nil {
		return err
	}

	return suite.createDatabase()
}

func (suite *BaseYDBTestSuite) newDBContext() (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), testQueryTimeout)
}
