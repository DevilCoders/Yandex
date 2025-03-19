package usage

import (
	"context"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/clickhouse/testdb"
	"a.yandex-team.ru/library/go/test/yatest"
)

var scriptsPath = yatest.SourcePath("cloud/billing/go/piper/internal/db/clickhouse/usage/sql/")

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
}

func (suite *baseSuite) SetupTest() {
	suite.ctx, suite.ctxCancel = context.WithTimeout(context.Background(), time.Second*5)

	err := testdb.CleanupSchema(suite.ctx, scriptsPath)
	suite.Require().NoError(err)

	suite.queries = New(testdb.DB())
}

func (suite *baseSuite) TearDownTest() {
	defer suite.ctxCancel()
}
