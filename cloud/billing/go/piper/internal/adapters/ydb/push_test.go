package ydb

import (
	"bytes"
	"compress/zlib"
	"io"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/usage"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

type pushTestSuite struct {
	baseMetaSessionTestSuite
}

func TestPush(t *testing.T) {
	suite.Run(t, new(pushTestSuite))
}

func (suite *pushTestSuite) SetupTest() {
	suite.baseMetaSessionTestSuite.SetupTest()
}

func (suite *pushTestSuite) TestPushInvalidMetric() {
	err := suite.session.PushInvalidMetric(suite.ctx,
		entities.ProcessingScope{
			SourceName: "yc-billing/source",
			SourceType: "logbroker-grpc",
			SourceID:   "vla42--yc-billing--source:999",
			Hostname:   "this.host",
		},
		entities.InvalidMetric{
			IncorrectRawMessage: entities.IncorrectRawMessage{
				Reason:        entities.FailedByExpired,
				ReasonComment: "test comment",
				MessageOffset: 9,
				UploadTime:    time.Unix(1000, 0),
			},
			Metric: entities.SourceMetric{
				MetricID:   "metric",
				Schema:     "schema",
				Version:    "default",
				SkuID:      "sku",
				SourceID:   "metric write source",
				ResourceID: "resource",
				Tags:       types.JSONAnything("{}"),
			},
		},
	)
	suite.NoError(err)

	want := usage.InvalidMetricRow{
		SourceName:       "yc-billing/source",
		UploadedAt:       qtool.UInt64Ts(time.Unix(1000, 0)),
		Reason:           "expired",
		MetricID:         "metric",
		SourceID:         "logbroker-grpc:vla42--yc-billing--source:999",
		ReasonComment:    "test comment",
		Hostname:         "this.host",
		RawMetric:        "",
		MetricSchema:     "schema",
		MetricSourceID:   "metric write source",
		MetricResourceID: "resource",
		SequenceID:       10,
	}
	wantJSON := `{
		"labels":{},"id":"metric","schema":"schema","cloud_id":"","folder_id":"","abc_id":0,"abc_folder_id":"",
		"billing_account_id":"","usage":{"quantity":0,"type":"","unit":"","start":0,"finish":0},"tags":{},
		"sku_id":"sku","resource_id":"resource","source_wt":0,"source_id":"metric write source","version":"default"
	}`

	suite.Require().Len(suite.session.invalidBuffer.rows, 1)
	got := suite.session.invalidBuffer.rows[0]
	gotJSON := got.Metric
	got.Metric = ""
	suite.EqualValues(want, got)
	suite.JSONEq(wantJSON, string(gotJSON), gotJSON)

	suite.mock.ExpectExec("REPLACE INTO `usage/realtime/invalid_metrics`").WithArgs(anyArg).WillReturnResult(sqlResult)
	err = suite.session.FlushInvalidMetrics(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().NoError(suite.mock.ExpectationsWereMet())
}

func (suite *pushTestSuite) TestPushOversized() {
	err := suite.session.PushOversizedMessage(suite.ctx,
		entities.ProcessingScope{
			SourceName: "yc-billing/source",
			SourceType: "logbroker-grpc",
			SourceID:   "vla42--yc-billing--source:999",
			Hostname:   "this.host",
		},
		entities.IncorrectRawMessage{
			Reason:        entities.FailedByExpired,
			ReasonComment: "test comment",
			MessageOffset: 9,
			RawMetric:     []byte("raw message"),
			UploadTime:    time.Unix(1000, 0),
		},
	)
	suite.NoError(err)

	want := usage.OversizedRow{
		SourceID:  "logbroker-grpc:vla42--yc-billing--source:999",
		SeqNo:     10,
		CreatedAt: ydbsql.Datetime(time.Unix(1000, 0)),
	}
	wantRaw := "raw message"

	suite.Require().Len(suite.session.oversizedBuffer.rows, 1)
	got := suite.session.oversizedBuffer.rows[0]
	gotRaw := uncompressMessage(got.CompressedMessage)
	got.CompressedMessage = nil
	suite.EqualValues(want, got)
	suite.EqualValues(wantRaw, gotRaw)

	suite.mock.ExpectExec("REPLACE INTO `usage/realtime/oversized_messages`").WithArgs(anyArg).WillReturnResult(sqlResult)
	err = suite.session.FlushOversized(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().NoError(suite.mock.ExpectationsWereMet())
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
		UploadedAt:       qtool.UInt64Ts(time.Unix(1000, 0)),
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
		Metric:           qtool.JSONAnything(`{"metric":"dump"}`),
	}

	suite.Require().Len(suite.session.invalidBuffer.rows, 1)
	got := suite.session.invalidBuffer.rows[0]
	suite.EqualValues(want, got)

	suite.mock.ExpectExec("REPLACE INTO `usage/realtime/invalid_metrics`").WithArgs(anyArg).WillReturnResult(sqlResult)
	err = suite.session.FlushIncorrectDumps(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().NoError(suite.mock.ExpectationsWereMet())
}

func uncompressMessage(data []byte) []byte {
	r, _ := zlib.NewReader(bytes.NewReader(data))
	result, _ := io.ReadAll(r)
	return result
}
