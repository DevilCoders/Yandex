package reports

import (
	"context"
	"os"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/testdb"
	"a.yandex-team.ru/library/go/test/yatest"
)

var scriptsPath = yatest.SourcePath("cloud/billing/go/piper/internal/db/ydb/reports/sql/")

func TestMain(m *testing.M) {
	_ = testdb.DB()

	ctx := context.Background()

	testdb.SetupSchema(ctx, scriptsPath)
	exitCode := m.Run()
	testdb.TearDownSchema(ctx, scriptsPath)

	os.Exit(exitCode)
}

type baseSuite struct {
	suite.Suite

	ctx       context.Context
	ctxCancel context.CancelFunc

	queries     Queries
	testQueries *testQueries
}

func NewTestQueries(db DBTX, params qtool.QueryParams) *testQueries {
	return &testQueries{db: db, qp: params}
}

type testQueries struct {
	db DBTX
	qp qtool.QueryParams
}

func (suite *baseSuite) SetupTest() {
	suite.ctx, suite.ctxCancel = context.WithCancel(context.Background())

	err := testdb.CleanupSchema(suite.ctx, scriptsPath)
	suite.Require().NoError(err)

	suite.queries = NewQueries(testdb.DB(), qtool.QueryParams{RootPath: "test/"})
	suite.testQueries = NewTestQueries(testdb.DB(), qtool.QueryParams{RootPath: "test/"})
}

func (suite *baseSuite) TearDownTest() {
	defer suite.ctxCancel()
}
