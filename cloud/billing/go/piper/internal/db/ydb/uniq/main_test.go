package uniq

import (
	"context"
	"os"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/testdb"
	"a.yandex-team.ru/library/go/test/yatest"
)

var scriptsPath = yatest.SourcePath("cloud/billing/go/piper/internal/db/ydb/uniq/sql/")

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

	queries *Queries
	schemes *Schemes
}

func (suite *baseSuite) SetupTest() {
	suite.ctx, suite.ctxCancel = context.WithCancel(context.Background())

	err := testdb.CleanupSchema(suite.ctx, scriptsPath)
	suite.Require().NoError(err)

	qp := qtool.QueryParams{
		RootPath: "test/",
		DB:       testdb.DBPath,
	}
	sess, err := testdb.YDBPool().Get(suite.ctx)
	suite.Require().NoError(err)

	suite.queries = New(testdb.DB(), qp)
	suite.schemes = NewScheme(sess, qp)
}

func (suite *baseSuite) TearDownTest() {
	defer suite.ctxCancel()
}
