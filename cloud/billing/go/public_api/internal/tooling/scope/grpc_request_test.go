package scope

import (
	"fmt"
	"strconv"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/metrics"
)

type grpcRequestTestSuite struct {
	baseGrpcRequestTestSuite
}

func TestGRPCRequest(t *testing.T) {
	suite.Run(t, new(grpcRequestTestSuite))
}

func (suite *grpcRequestTestSuite) TestStartGRPCRequestCorrectScope() {
	expectedMethod := "foo"

	ctx := StartGRPCRequest(suite.ctx, expectedMethod)
	scope := getCurrent(ctx)

	suite.Require().IsType(&grpcRequest{}, scope)
	suite.Require().NotNil(scope.Logger())
	suite.Require().NotEmpty(scope.(handlerScope).RequestID())
	suite.Require().Equal(suite.initialTime, scope.(*grpcRequest).StartedAt)
	suite.Require().Equal(expectedMethod, scope.(*grpcRequest).Method)
}

func (suite *grpcRequestTestSuite) TestStartGRPCRequestLogging() {
	expectedMethod := "foo"

	StartGRPCRequest(suite.ctx, expectedMethod)

	records := suite.observer.AllUntimed()
	suite.Require().Len(records, 1)

	record := records[0]
	suite.Require().Equal("start unary grpc call", record.Message)

	fields := records[0].ContextMap()
	suite.NotEmpty(fields["request_id"])
	suite.Require().Equal(expectedMethod, fields["grpc_full_method"])
}

func (suite *grpcRequestTestSuite) TestStartGRPCRequestMetrics() {
	metrics.ResetMetrics()
	expectedMethod := "foo"

	StartGRPCRequest(suite.ctx, expectedMethod)

	started := suite.getMetric(metrics.GRPCRequestStarted)
	method := suite.findLabel(started.Label, "method")

	suite.EqualValues(1, started.Counter.GetValue())
	suite.EqualValues(expectedMethod, method.GetValue())

}

func (suite *grpcRequestTestSuite) TestFinishGRPCRequestOKLogging() {
	expectedDuration := time.Second

	scope := &grpcRequest{
		logger:    suite.logger,
		requestID: "request-id",
		StartedAt: suite.initialTime,
		Method:    "foo",
	}
	ctx := newScopeContext(scope, suite.ctx)
	suite.clock.Advance(expectedDuration)

	FinishGRPCRequest(ctx, nil)

	records := suite.observer.FilterMessage("success unary grpc call").AllUntimed()
	suite.Require().Len(records, 1)

	fields := records[0].ContextMap()
	suite.Require().Equal("OK", fields["grpc_code"])
	suite.Require().Equal(expectedDuration.Milliseconds(), fields["duration_ms"])
}

func (suite *grpcRequestTestSuite) TestFinishGRPCRequestErrLogging() {
	expectedDuration := time.Second
	expectedError := fmt.Errorf("test error")

	scope := &grpcRequest{
		logger:    suite.logger,
		requestID: "request-id",
		StartedAt: suite.initialTime,
		Method:    "foo",
	}
	ctx := newScopeContext(scope, suite.ctx)
	suite.clock.Advance(expectedDuration)

	FinishGRPCRequest(ctx, expectedError)

	records := suite.observer.FilterMessage("error unary grpc call").AllUntimed()
	suite.Require().Len(records, 1)

	fields := records[0].ContextMap()
	suite.Require().Equal("Unknown", fields["grpc_code"])
	suite.Require().Equal(expectedDuration.Milliseconds(), fields["duration_ms"])
	suite.Require().Equal(expectedError.Error(), fields["error"])
}

func (suite *grpcRequestTestSuite) TestFinishGRPCRequestMetrics() {
	metrics.ResetMetrics()
	expectedMethod := "foo"
	expectedDuration := time.Second

	scope := &grpcRequest{
		logger:    suite.logger,
		requestID: "request-id",
		StartedAt: suite.initialTime,
		Method:    expectedMethod,
	}
	ctx := newScopeContext(scope, suite.ctx)
	suite.clock.Advance(expectedDuration)

	FinishGRPCRequest(ctx, nil)

	{
		done := suite.getMetric(metrics.GRPCRequestFinished)
		method := suite.findLabel(done.Label, "method")
		code := suite.findLabel(done.Label, "code")

		suite.EqualValues(1, done.Counter.GetValue())
		suite.EqualValues(expectedMethod, method.GetValue())
		suite.EqualValues(strconv.Itoa(int(codes.OK)), code.GetValue())
	}

	{
		duration := suite.getMetric(metrics.GRPCRequestDuration)
		method := suite.findLabel(duration.Label, "method")
		code := suite.findLabel(duration.Label, "code")

		suite.EqualValues(1, duration.Histogram.GetSampleCount())
		suite.EqualValues(time.Second.Milliseconds(), duration.Histogram.GetSampleSum())
		suite.EqualValues(expectedMethod, method.GetValue())
		suite.EqualValues(strconv.Itoa(int(codes.OK)), code.GetValue())
	}
}
