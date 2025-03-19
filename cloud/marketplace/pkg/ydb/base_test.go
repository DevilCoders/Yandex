package ydb

import (
	"context"
	"os"
	"path"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/test/yatest"
)

var scriptsPath = yatest.SourcePath("cloud/marketplace/lich/internal/db/ydb/gotest/sql")

type BaseYDBTestSuite struct {
	suite.Suite

	dbConnection *Connector

	logger log.Logger
}

func (suite *BaseYDBTestSuite) SetupSuite() {
	var err error

	suite.logger = &zap.Logger{
		L: zaptest.NewLogger(suite.T()),
	}

	connectCtx, cancel := context.WithTimeout(context.Background(), testConnectionTimepout)
	defer cancel()

	suite.dbConnection, err = Connect(connectCtx,
		WithDBRoot("test/mkt"),
		WithEndpoint(testYDBEndpoint),
		WithDatabase(testYDBDatabase),
		WithLogger(suite.logger),
	)

	suite.Require().NoError(err)
	suite.Require().NotNil(suite.dbConnection)

	err = suite.createDatabase()
	suite.Require().NoError(err)
}

func (suite *BaseYDBTestSuite) TearDownSuite() {
	err := suite.dropDatabase()
	suite.Require().NoError(err)

	err = suite.dbConnection.Close()
	suite.Require().NoError(err)
}

func (suite *BaseYDBTestSuite) SetupTest() {
	err := suite.populateDatabase()
	suite.Require().NoError(err)
}

func (suite *BaseYDBTestSuite) TearDownTest() {
	err := suite.purgeDatabase()
	suite.Require().NoError(err)
}

func (suite *BaseYDBTestSuite) createDatabase() error {
	queryCtx, cancel := context.WithTimeout(context.Background(), testQueryTimeout)
	defer cancel()

	return suite.runSchemaFileContext(queryCtx, path.Join(scriptsPath, "create.sql"))
}

func (suite *BaseYDBTestSuite) populateDatabase() error {
	queryCtx, cancel := context.WithTimeout(context.Background(), testQueryTimeout)
	defer cancel()

	return suite.runQueryFromFileContext(queryCtx, path.Join(scriptsPath, "populate.sql"))
}

func (suite *BaseYDBTestSuite) dropDatabase() error {
	queryCtx, cancel := context.WithTimeout(context.Background(), testQueryTimeout)
	defer cancel()

	return suite.runSchemaFileContext(queryCtx, path.Join(scriptsPath, "drop.sql"))
}

func (suite *BaseYDBTestSuite) purgeDatabase() error {
	queryCtx, cancel := context.WithTimeout(context.Background(), testQueryTimeout)
	defer cancel()

	return suite.runQueryFromFileContext(queryCtx, path.Join(scriptsPath, "purge.sql"))
}

func (suite *BaseYDBTestSuite) runQueryFromFileContext(ctx context.Context, fileName string) error {
	content, err := os.ReadFile(fileName)
	if err != nil {
		return err
	}

	_, err = suite.dbConnection.db.ExecContext(ctx, string(content))
	return err
}

func (suite *BaseYDBTestSuite) runSchemaFileContext(ctx context.Context, filename string) error {
	sess, err := suite.dbConnection.directPool.Get(ctx)
	if err != nil {
		return err
	}

	content, err := os.ReadFile(filename)
	if err != nil {
		return err
	}

	return sess.ExecuteSchemeQuery(ctx, string(content))
}

func (suite *BaseYDBTestSuite) newDBContext() (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), testQueryTimeout)
}
