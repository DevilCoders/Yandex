package handlers

import (
	"context"
	"errors"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/servers/logbroker/handlers/mocks"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type dumpErrorsTestSuite struct {
	suite.Suite
	messagesMocks

	target  *mocks.DumpErrorsTarget
	handler *DumpErrorsHandler
}

func TestDumpErrors(t *testing.T) {
	suite.Run(t, new(dumpErrorsTestSuite))
}

func (suite *dumpErrorsTestSuite) SetupTest() {
	suite.messagesMocks.SetupTest()

	suite.target = &mocks.DumpErrorsTarget{}
	suite.handler = NewDumpErrorsHandler("test/src", "dump", "dump-pipe", suite.getTarget)
	suite.handler.clock = getClock()

	suite.pushMessage(`{
		"sequence_id": 42,
		"reason": "fail_reason",
		"metric_source_id": "some error source",
		"source_id": "logbroker-grpc:topic:partition",
		"hostname": "host with failed metric",
		"raw_metric": null,
		"reason_comment": "this metric contains error",
		"metric_resource_id": "metric resource",
		"source_name": "test/source",
		"metric_schema": "schema",
		"metric": "{\"metric\":\"json data\"}",
		"uploaded_at": 1000,
		"metric_id": "77d5583c-4a52-41c7-8f41-72abb4f142f4"
	}`)
}

func (suite *dumpErrorsTestSuite) getTarget() DumpErrorsTarget {
	return suite.target
}

func (suite *dumpErrorsTestSuite) TestDump() {
	var calbackErr error

	suite.reporter.On("Error", mock.MatchedBy(func(err error) bool { calbackErr = err; return true }))
	suite.reporter.On("Consumed")
	suite.target.On("PushIncorrectDump", mock.Anything, mock.Anything, mock.Anything).Once().Return(nil)
	suite.target.On("FlushIncorrectDumps", mock.Anything).Return(nil)

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().NoError(calbackErr)

	var pushCall mock.Call
	for _, c := range suite.target.Calls {
		if c.Method == "PushIncorrectDump" {
			pushCall = c
		}
	}
	suite.Require().Equal("PushIncorrectDump", pushCall.Method)

	wantScope := entities.ProcessingScope{
		SourceName:       "test/src",
		SourceType:       "logbroker-grpc",
		SourceID:         "test-source",
		StartTime:        getClock().Now(),
		Hostname:         "test-host",
		Pipeline:         "dump-pipe",
		MinMessageOffset: 1,
		MaxMessageOffset: 1,
	}
	wantDump := entities.IncorrectMetricDump{
		SequenceID:       42,
		MetricID:         "77d5583c-4a52-41c7-8f41-72abb4f142f4",
		Reason:           "fail_reason",
		ReasonComment:    "this metric contains error",
		MetricSourceID:   "some error source",
		SourceID:         "logbroker-grpc:topic:partition",
		SourceName:       "test/source",
		Hostname:         "host with failed metric",
		UploadedAt:       time.Unix(1000, 0),
		MetricSchema:     "schema",
		MetricResourceID: "metric resource",
		MetricData:       `{"metric":"json data"}`,
	}

	suite.EqualValues(wantScope, pushCall.Arguments[1])
	suite.EqualValues(wantDump, pushCall.Arguments[2])
}

func (suite *dumpErrorsTestSuite) TestError() {
	var calbackErr error

	testErr := errors.New("testErr")

	suite.reporter.On("Error", mock.MatchedBy(func(err error) bool { calbackErr = err; return true }))
	suite.target.On("PushIncorrectDump", mock.Anything, mock.Anything, mock.Anything).Once().Return(testErr)

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().Error(calbackErr)
	suite.ErrorIs(calbackErr, testErr)
}

func (suite *dumpErrorsTestSuite) TestParseError() {
	suite.pushMessage(`Not json`)

	var calbackErr error

	suite.reporter.On("Error", mock.MatchedBy(func(err error) bool { calbackErr = err; return true }))

	suite.handler.Handle(context.TODO(), lbtypes.SourceID("test-source"), &suite.messages)
	suite.Require().Error(calbackErr)
}
