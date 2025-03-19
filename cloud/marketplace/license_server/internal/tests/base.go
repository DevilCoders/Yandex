package tests

import (
	"context"
	"os"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/mocks"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/migrations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	ydb_pkg "a.yandex-team.ru/cloud/marketplace/pkg/ydb"
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

type BaseTestSuite struct {
	suite.Suite

	Env *env.Env

	connector *ydb_pkg.Connector
	provider  ydb.LicenseServerProvider
	migrator  *migrations.Migrator

	BillingMock     *mocks.Billing
	MarketplaceMock *mocks.Marketplace
	DefaultAuthMock *mocks.AuthTokenProvider

	logger log.Logger
}

func (suite *BaseTestSuite) SetupSuite() {
	var err error

	suite.logger = &zap.Logger{
		L: zaptest.NewLogger(suite.T()),
	}

	connectCtx, cancel := context.WithTimeout(context.Background(), testConnectionTimepout)
	defer cancel()

	suite.connector, err = ydb_pkg.Connect(connectCtx,
		ydb_pkg.WithDBRoot("license_server"),
		ydb_pkg.WithEndpoint(testYDBEndpoint),
		ydb_pkg.WithDatabase(testYDBDatabase),
		ydb_pkg.WithLogger(suite.logger),
	)

	suite.Require().NoError(err)
	suite.Require().NotNil(suite.connector)

	suite.provider = ydb.NewLicenseServerProvider(suite.connector)
	suite.migrator = migrations.NewMigrator(suite.connector)

	err = suite.createDatabase()
	suite.Require().NoError(err)

	opts := []env.BackendsOption{env.BackendsWithYdb(suite.provider)}
	opts = append(opts, suite.initMocks()...)

	suite.Env = env.NewEnvBuilder().
		WithHandlersLogger(suite.logger).
		WithBackendsFabrics(opts...).
		WithCloudIDGeneratorPrefix("tst").
		Build()
}

func (suite *BaseTestSuite) initMocks() []env.BackendsOption {
	suite.BillingMock = new(mocks.Billing)
	suite.MarketplaceMock = new(mocks.Marketplace)
	suite.DefaultAuthMock = new(mocks.AuthTokenProvider)

	return []env.BackendsOption{
		env.BackendsWithBilling(suite.BillingMock),
		env.BackendsWithMarketplace(suite.MarketplaceMock),
		env.BackendsWithYCDefaultCredentials(suite.DefaultAuthMock),
	}
}

func (suite *BaseTestSuite) TearDownSuite() {
	err := suite.dropDatabase()
	suite.Require().NoError(err)

	err = suite.connector.Close()
	suite.Require().NoError(err)
}

func (suite *BaseTestSuite) TearDownTest() {
	err := suite.purgeDatabase()
	suite.Require().NoError(err)
}

func (suite *BaseTestSuite) createDatabase() error {
	err := suite.migrator.InitUp(context.Background())
	if err != nil {
		return err
	}
	return suite.migrator.Up(context.Background())
}

func (suite *BaseTestSuite) dropDatabase() error {
	err := suite.migrator.DownAll(context.Background())
	if err != nil {
		return err
	}

	return suite.migrator.InitDown(context.Background())
}

func (suite *BaseTestSuite) purgeDatabase() error {
	err := suite.dropDatabase()
	if err != nil {
		return err
	}

	return suite.createDatabase()
}

func (suite *BaseTestSuite) newDBContext() (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), testQueryTimeout)
}
