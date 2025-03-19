package ydb

import (
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/rtc/mediator/cityhash"
)

type uniqTestSuite struct {
	baseSessionTestSuite
	session *UniqSession
}

func TestUniq(t *testing.T) {
	suite.Run(t, new(uniqTestSuite))
}

func (suite *uniqTestSuite) SetupTest() {
	suite.baseSessionTestSuite.SetupTest()

	maxUint := ^uint64(0)
	suite.schemeMock.On("DescribeTable", mock.Anything, "/uniques/hashed", mock.Anything).Return(table.Description{
		KeyRanges: []table.KeyRange{{From: nil, To: ydb.TupleValue(ydb.Uint64Value(maxUint))}},
	}, nil)

	cfg := DataAdapterConfig{EnableUniq: true}
	a, err := NewDataAdapter(suite.ctx, cfg, suite.dbx(), suite.schemeMock, "", "")
	suite.Require().NoError(err)

	suite.session = a.UniqSession()
}

func (suite *uniqTestSuite) TestPushNewMetrics() {
	at := time.Date(2021, time.February, 1, 0, 0, 0, 0, timetool.DefaultTz())

	suite.mock.ExpectExec("INSERT INTO `uniques/hashed`").WithArgs(anyArg, anyArg).WillReturnResult(sqlResult)

	dups, err := suite.session.FindDuplicates(suite.ctx,
		entities.ProcessingScope{
			SourceID: "vla42--yc-billing--source:999",
		},
		at,
		[]entities.MetricIdentity{
			{Schema: "schema", MetricID: "metric1", Offset: 0},
			{Schema: "schema", MetricID: "metric2", Offset: 1},
		})
	suite.NoError(err)

	suite.Require().NoError(suite.mock.ExpectationsWereMet())

	suite.Empty(dups)
}

func (suite *uniqTestSuite) TestPushWithDuplicates() {
	at := time.Date(2021, time.February, 1, 0, 0, 0, 0, timetool.DefaultTz())

	suite.mock.ExpectExec("INSERT INTO `uniques/hashed`").WithArgs(anyArg, anyArg).WillReturnResult(sqlResult)

	dups, err := suite.session.FindDuplicates(suite.ctx,
		entities.ProcessingScope{
			SourceID: "vla42--yc-billing--source:999",
		},
		at,
		[]entities.MetricIdentity{
			{Schema: "schema", MetricID: "metric1", Offset: 0},
			{Schema: "schema", MetricID: "metric1", Offset: 1},
		})
	suite.NoError(err)

	suite.Require().NoError(suite.mock.ExpectationsWereMet())

	want := []entities.MetricIdentity{
		{Schema: "schema", MetricID: "metric1", Offset: 1},
	}
	suite.ElementsMatch(dups, want)
}

func (suite *uniqTestSuite) TestPushDuplicates() {
	at := time.Date(2021, time.February, 1, 0, 0, 0, 0, timetool.DefaultTz())
	dupMetric := entities.MetricIdentity{Schema: "schema", MetricID: "metric2", Offset: 1}
	dupRow := suite.session.makeMetricIdentityRow("vla42--yc-billing--source:999", dupMetric)
	warnMetric := entities.MetricIdentity{Schema: "schema", MetricID: "metric3", Offset: 2}
	warnRow := suite.session.makeMetricIdentityRow("vla42--yc-billing--source:999", warnMetric)

	// Insert will be failed and search will be performed
	suite.mock.ExpectExec("INSERT INTO `uniques/hashed`").WithArgs(anyArg, anyArg).
		WillReturnError(&ydb.OpError{Reason: ydb.StatusPreconditionFailed})

	pushRows := suite.mock.NewRows([]string{"cnt"}).AddRow(uint64(1))
	suite.mock.ExpectQuery(`SELECT count\(\*\) as cnt FROM \$joint`).WithArgs(anyArg, anyArg).
		WillReturnRows(pushRows).RowsWillBeClosed()

	dupRows := suite.mock.NewRows([]string{"key_hash", "chk"}).
		AddRow(dupRow.KeyHash, dupRow.Check).
		AddRow(warnRow.KeyHash, 0)

	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("SELECT table.key_hash as key_hash, table.chk as chk").WithArgs().
		WillReturnRows(dupRows).RowsWillBeClosed()

	dups, err := suite.session.FindDuplicates(suite.ctx,
		entities.ProcessingScope{
			SourceID: "vla42--yc-billing--source:999",
		},
		at,
		[]entities.MetricIdentity{
			{Schema: "schema", MetricID: "metric1", Offset: 0},
			dupMetric, warnMetric,
		})
	suite.NoError(err)

	suite.Require().NoError(suite.mock.ExpectationsWereMet())

	want := []entities.MetricIdentity{
		{Schema: "schema", MetricID: "metric2", Offset: 1},
	}
	suite.ElementsMatch(dups, want)
}

func BenchmarkCityhash64(b *testing.B) {
	data := []byte("some text for hashing")
	for i := 0; i < b.N; i++ {
		_ = cityhash.Hash64(data)
	}
}

func BenchmarkCityhash218(b *testing.B) {
	data := []byte("some text for hashing")
	for i := 0; i < b.N; i++ {
		_, _ = cityhash.Hash128(data)
	}
}
