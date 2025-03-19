package usage

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
)

type invalidTestSuite struct {
	baseSuite
}

func TestInvalid(t *testing.T) {
	suite.Run(t, new(invalidTestSuite))
}

func (suite *invalidTestSuite) TestPushRows() {
	epoch := time.Unix(0, 0)
	rows := []InvalidMetricRow{
		{
			SourceName:       "logbroker-something",
			UploadedAt:       uint64(epoch.Add(time.Second * 1).Unix()),
			Reason:           "some reason",
			MetricID:         "metric 1",
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
			UploadedAt:       uint64(epoch.Add(time.Second * 2).Unix()),
			Reason:           "some other reason",
			MetricID:         "metric 2",
			SourceID:         "src2",
			ReasonComment:    "other error in metric",
			Hostname:         "host1",
			RawMetric:        "",
			Metric:           "null",
			MetricSchema:     "schema",
			MetricSourceID:   "src2",
			MetricResourceID: "res2",
			SequenceID:       1,
		},
	}

	err := suite.queries.PushInvalid(suite.ctx, rows...)
	suite.Require().NoError(err)

	got, err := suite.queries.getAllInvalid(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(got, len(rows))

	got[0].UploadedAt = rows[0].UploadedAt
	got[0].SequenceID = rows[0].SequenceID
	got[1].UploadedAt = rows[1].UploadedAt
	got[1].SequenceID = rows[1].SequenceID
	suite.EqualValues(rows[0], got[0])
	suite.EqualValues(rows[1], got[1])
}

func (q *Queries) getAllInvalid(ctx context.Context) (result []InvalidMetricRow, err error) {
	query := `SELECT source_name, reason, metric_id, source_id, reason_comment, hostname,
					raw_metric, metric, metric_schema, metric_source_id, metric_resource_id
			  FROM invalid_metrics
			  ORDER BY metric_id ASC`
	err = q.connection.SelectContext(ctx, &result, query)
	return
}
