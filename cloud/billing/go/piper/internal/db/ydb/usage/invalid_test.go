package usage

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
)

type invalidTestSuite struct {
	baseSuite
}

func TestInvalid(t *testing.T) {
	suite.Run(t, new(invalidTestSuite))
}

func (suite *invalidTestSuite) TestTableCompliance() {
	err := suite.queries.PushInvalid(suite.ctx, InvalidMetricRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllInvalid(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *invalidTestSuite) TestPushRows() {
	epoch := time.Unix(0, 0)
	rows := []InvalidMetricRow{
		{
			SourceName:       "logbroker-something",
			UploadedAt:       qtool.UInt64Ts(epoch.Add(time.Second * 1)),
			Reason:           "some reason",
			MetricID:         "mertic 1",
			SourceID:         "src1",
			ReasonComment:    "some error in metric",
			Hostname:         "no host here",
			RawMetric:        "",
			Metric:           "{}",
			MetricSchema:     "schema",
			MetricSourceID:   "src1",
			MetricResourceID: "res1",
			SequenceID:       0,
		},
		{
			SourceName:       "logbroker-something",
			UploadedAt:       qtool.UInt64Ts(epoch.Add(time.Second * 2)),
			Reason:           "some other reason",
			MetricID:         "mertic 2",
			SourceID:         "src1",
			ReasonComment:    "other error in metric",
			Hostname:         "host1",
			RawMetric:        "",
			Metric:           "null",
			MetricSchema:     "schema",
			MetricSourceID:   "src33",
			MetricResourceID: "res1",
			SequenceID:       1,
		},
	}

	err := suite.queries.PushInvalid(suite.ctx, rows...)
	suite.Require().NoError(err)

	got, err := suite.queries.getAllInvalid(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(got, len(rows))
	suite.EqualValues(rows[0], got[0])
	suite.EqualValues(rows[1], got[1])
}

func (q *Queries) getAllInvalid(ctx context.Context) (result []InvalidMetricRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllInvalidQuery.WithParams(q.qp))
	return
}

var getAllInvalidQuery = qtool.Query(
	"SELECT * FROM", qtool.Table("usage/realtime/invalid_metrics"),
	"ORDER BY source_name, uploaded_at, reason, metric_id;",
)
