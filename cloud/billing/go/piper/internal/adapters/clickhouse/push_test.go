package clickhouse

import (
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/clickhouse/usage"
)

type pushTestSuite struct {
	baseSessionTestSuite
}

func TestPush(t *testing.T) {
	suite.Run(t, new(pushTestSuite))
}

func (suite *pushTestSuite) TestPushIncorrectDump() {
	err := suite.session.PushIncorrectDump(suite.ctx,
		entities.ProcessingScope{},
		entities.IncorrectMetricDump{
			SequenceID:       10,
			MetricID:         "metric",
			Reason:           "some legacy reason",
			ReasonComment:    "this reason from external log",
			MetricSourceID:   "metric write source",
			SourceID:         "logbroker-grpc:vla42--yc-billing--source:999",
			SourceName:       "yc-billing/source",
			Hostname:         "other.host",
			UploadedAt:       time.Unix(1000, 0),
			MetricSchema:     "schema",
			MetricResourceID: "resource",
			MetricData:       `{"metric":"dump"}`,
		},
	)
	suite.NoError(err)

	want := usage.InvalidMetricRow{
		SourceName:       "yc-billing/source",
		Reason:           "some legacy reason",
		MetricID:         "metric",
		SourceID:         "logbroker-grpc:vla42--yc-billing--source:999",
		ReasonComment:    "this reason from external log",
		Hostname:         "other.host",
		RawMetric:        "",
		MetricSchema:     "schema",
		MetricSourceID:   "metric write source",
		MetricResourceID: "resource",
		SequenceID:       10,
		Metric:           `{"metric":"dump"}`,
	}

	partition := suite.session.adapter.splitter.Partition(want.MetricID)
	suite.Require().Len(suite.session.invalidBufferForPartition[partition].rows, 1)
	got := suite.session.invalidBufferForPartition[partition].rows[0]
	suite.EqualValues(want, got)

	suite.mock.ExpectBegin()
	suite.mock.ExpectPrepare("INSERT INTO invalid_metrics")
	suite.mock.ExpectExec("INSERT INTO invalid_metrics").WillReturnResult(sqlResult)
	suite.mock.ExpectCommit()
	err = suite.session.FlushIncorrectDumps(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().NoError(suite.mock.ExpectationsWereMet())
}
