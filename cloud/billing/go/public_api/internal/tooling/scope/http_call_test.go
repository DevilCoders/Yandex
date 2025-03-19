package scope

import (
	"fmt"
	"net/http"
	"strconv"
	"testing"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/metrics"
)

type httpCallTestSuite struct {
	baseGrpcRequestTestSuite
	client   *resty.Client
	request  *resty.Request
	response *resty.Response
}

func (suite *httpCallTestSuite) SetupTest() {
	suite.baseGrpcRequestTestSuite.SetupTest()

	globalScope := getCurrent(suite.ctx)
	handlerScope := &grpcRequest{
		logger:    globalScope.Logger(),
		requestID: "request-id",
		StartedAt: suite.clock.Now(),
		Method:    "foo",
	}

	suite.ctx = newScopeContext(handlerScope, suite.ctx)
	suite.client = &resty.Client{}
	suite.request = suite.client.R()
	suite.request.URL = "test-url"
	suite.request.Method = "GET"
	suite.request.SetContext(suite.ctx)
	suite.response = &resty.Response{Request: suite.request, RawResponse: &http.Response{StatusCode: 200}}
}

func TestHTTPCall(t *testing.T) {
	suite.Run(t, new(httpCallTestSuite))
}

func (suite *httpCallTestSuite) TestStartHTTPCallCorrectScope() {
	suite.clock.Advance(time.Second)
	expectedStartedAt := suite.clock.Now()
	parentScope := getCurrent(suite.ctx).(handlerScope)

	err := StartHTTPCall(suite.client, suite.request)
	scope := getCurrent(suite.request.Context())

	suite.Require().NoError(err)
	suite.Require().IsType(&httpCall{}, scope)
	suite.Require().NotNil(scope.Logger())
	suite.Require().Equal(parentScope.RequestID(), scope.(handlerScope).RequestID())
	suite.Require().Equal(expectedStartedAt, scope.(*httpCall).StartedAt)
}

func (suite *httpCallTestSuite) TestStartHTTPCallLogging() {
	err := StartHTTPCall(suite.client, suite.request)

	suite.Require().NoError(err)

	records := suite.observer.AllUntimed()
	suite.Require().Len(records, 1)

	record := records[0]
	suite.Require().Equal("start console call", record.Message)

	fields := records[0].ContextMap()
	suite.Require().Equal(suite.request.Method, fields["http_method"])
	suite.Require().Equal(suite.request.URL, fields["http_url_pattern"])
}

func (suite *httpCallTestSuite) TestStartHTTPCallMetrics() {
	metrics.ResetMetrics()
	err := StartHTTPCall(suite.client, suite.request)

	suite.Require().NoError(err)

	started := suite.getMetric(metrics.HTTPCallStarted)
	method := suite.findLabel(started.Label, "method")
	url := suite.findLabel(started.Label, "url_pattern")

	suite.EqualValues(1, started.Counter.GetValue())
	suite.EqualValues(suite.request.Method, method.GetValue())
	suite.EqualValues(suite.request.URL, url.GetValue())
}

func (suite *httpCallTestSuite) TestFinishHTTPCallResponseLogging() {
	cases := []struct {
		statusCode      int
		expectedMessage string
	}{
		{
			statusCode:      200,
			expectedMessage: "success http response",
		},
		{
			statusCode:      400,
			expectedMessage: "error http response",
		},
		{
			statusCode:      500,
			expectedMessage: "server error http response",
		},
	}

	for i, testCase := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			suite.SetupTest()

			parentScope := getCurrent(suite.ctx).(handlerScope)
			scope := &httpCall{
				logger:    suite.logger,
				requestID: parentScope.RequestID(),
				StartedAt: suite.clock.Now(),
			}
			ctx := newScopeContext(scope, suite.ctx)
			suite.request.SetContext(ctx)

			expectedDuration := time.Second
			suite.clock.Advance(expectedDuration)

			suite.response.RawResponse.StatusCode = testCase.statusCode
			err := FinishHTTPCallResponse(suite.client, suite.response)

			suite.Require().NoError(err)

			records := suite.observer.AllUntimed()
			suite.Require().Len(records, 1)

			record := records[0]
			suite.Require().Equal(testCase.expectedMessage, record.Message)

			fields := records[0].ContextMap()
			suite.Require().EqualValues(suite.response.StatusCode(), fields["http_status_code"])
			suite.Require().Equal(expectedDuration.Milliseconds(), fields["duration_ms"])
		})
	}
}

func (suite *httpCallTestSuite) TestFinishHTTPCallResponseMetrics() {
	metrics.ResetMetrics()
	parentScope := getCurrent(suite.ctx).(handlerScope)
	scope := &httpCall{
		logger:    suite.logger,
		requestID: parentScope.RequestID(),
		StartedAt: suite.clock.Now(),
	}
	ctx := newScopeContext(scope, suite.ctx)
	suite.request.SetContext(ctx)

	expectedDuration := time.Second
	suite.clock.Advance(expectedDuration)

	err := FinishHTTPCallResponse(suite.client, suite.response)

	suite.Require().NoError(err)

	{
		done := suite.getMetric(metrics.HTTPCallFinished)
		method := suite.findLabel(done.Label, "method")
		url := suite.findLabel(done.Label, "url_pattern")
		code := suite.findLabel(done.Label, "code")

		suite.EqualValues(1, done.Counter.GetValue())
		suite.EqualValues(suite.request.Method, method.GetValue())
		suite.EqualValues(suite.request.URL, url.GetValue())
		suite.EqualValues(strconv.Itoa(suite.response.StatusCode()), code.GetValue())
	}

	{
		duration := suite.getMetric(metrics.HTTPCallDuration)
		method := suite.findLabel(duration.Label, "method")
		url := suite.findLabel(duration.Label, "url")
		code := suite.findLabel(duration.Label, "code")

		suite.EqualValues(1, duration.Histogram.GetSampleCount())
		suite.EqualValues(expectedDuration.Milliseconds(), duration.Histogram.GetSampleSum())
		suite.EqualValues(suite.request.Method, method.GetValue())
		suite.EqualValues(suite.request.URL, url.GetValue())
		suite.EqualValues(strconv.Itoa(suite.response.StatusCode()), code.GetValue())
	}
}

func (suite *httpCallTestSuite) TestFinishHTTPCallErrLogging() {
	parentScope := getCurrent(suite.ctx).(handlerScope)
	scope := &httpCall{
		logger:    suite.logger,
		requestID: parentScope.RequestID(),
		StartedAt: suite.clock.Now(),
	}
	ctx := newScopeContext(scope, suite.ctx)
	suite.request.SetContext(ctx)

	expectedDuration := time.Second
	suite.clock.Advance(expectedDuration)

	expectedErr := fmt.Errorf("test error")

	FinishHTTPCallErr(suite.request, expectedErr)

	records := suite.observer.AllUntimed()
	suite.Require().Len(records, 1)

	record := records[0]
	suite.Require().Equal("error http request", record.Message)

	fields := records[0].ContextMap()
	suite.Require().Equal(expectedErr.Error(), fields["error"])
}

func (suite *httpCallTestSuite) TestFinishHTTPCallErrMetrics() {
	metrics.ResetMetrics()
	parentScope := getCurrent(suite.ctx).(handlerScope)
	scope := &httpCall{
		logger:    suite.logger,
		requestID: parentScope.RequestID(),
		StartedAt: suite.clock.Now(),
	}
	ctx := newScopeContext(scope, suite.ctx)
	suite.request.SetContext(ctx)

	expectedDuration := time.Second
	suite.clock.Advance(expectedDuration)

	expectedErr := fmt.Errorf("test error")

	FinishHTTPCallErr(suite.request, expectedErr)

	errorCounter := suite.getMetric(metrics.HTTPCallError)
	method := suite.findLabel(errorCounter.Label, "method")
	url := suite.findLabel(errorCounter.Label, "url_pattern")
	err := suite.findLabel(errorCounter.Label, "error")

	suite.EqualValues(1, errorCounter.Counter.GetValue())
	suite.EqualValues(suite.request.Method, method.GetValue())
	suite.EqualValues(suite.request.URL, url.GetValue())
	suite.EqualValues(expectedErr.Error(), err.GetValue())
}
