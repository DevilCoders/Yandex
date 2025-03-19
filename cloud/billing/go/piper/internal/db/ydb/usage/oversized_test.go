package usage

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

type oversizedTestSuite struct {
	baseSuite
}

func TestOversized(t *testing.T) {
	suite.Run(t, new(oversizedTestSuite))
}

func (suite *oversizedTestSuite) TestTableCompliance() {
	err := suite.queries.PushOversized(suite.ctx, OversizedRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllOversized(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *oversizedTestSuite) TestPushRows() {
	epoch := time.Unix(0, 0)
	rows := []OversizedRow{
		{SourceID: "src1", SeqNo: 1, CompressedMessage: []byte("some msg"), CreatedAt: ydbsql.Datetime(epoch.Add(time.Second * 1))},
		{SourceID: "src1", SeqNo: 2, CompressedMessage: []byte("some msg"), CreatedAt: ydbsql.Datetime(epoch.Add(time.Second * 2))},
		{SourceID: "src2", SeqNo: 999, CompressedMessage: []byte("some msg"), CreatedAt: ydbsql.Datetime(epoch.Add(time.Second * 3))},
		{SourceID: "src3", SeqNo: 1, CompressedMessage: []byte(""), CreatedAt: ydbsql.Datetime(epoch.Add(time.Second * 4))},
	}

	err := suite.queries.PushOversized(suite.ctx, rows...)
	suite.Require().NoError(err)

	got, err := suite.queries.getAllOversized(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(got, len(rows))
	suite.EqualValues(rows[0], got[0])
	suite.EqualValues(rows[1], got[1])
	suite.EqualValues(rows[2], got[2])
	suite.EqualValues(rows[3], got[3])
}

func (q *Queries) getAllOversized(ctx context.Context) (result []OversizedRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllOversizedQuery.WithParams(q.qp))
	return
}

var getAllOversizedQuery = qtool.Query(
	"SELECT * FROM", qtool.Table("usage/realtime/oversized_messages"),
	"ORDER BY source_id, seq_no;",
)
