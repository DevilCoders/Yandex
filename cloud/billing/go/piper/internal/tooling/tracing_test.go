package tooling

import (
	"errors"
	"reflect"
	"testing"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/mocktracer"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/library/go/core/log"
)

type actionTraceSuite struct {
	baseSuite
}

func TestActionTracingSuite(t *testing.T) {
	suite.Run(t, new(actionTraceSuite))
}

func (suite *actionTraceSuite) TestActionOk() {
	ctx := ActionWithNameStarted(suite.ctx, "test action")
	ActionDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	logs := span.Logs()
	suite.NotZero(span.ParentID)
	suite.Equal("test action", span.OperationName)

	suite.Len(tags, 2)
	suite.Contains(tags, "action")
	suite.Contains(tags, "span_type")
	suite.EqualValues("test action", tags["action"])
	suite.EqualValues("action", tags["span_type"])

	suite.Empty(logs)
}

func (suite *actionTraceSuite) TestActionErr() {
	ctx := ActionWithNameStarted(suite.ctx, "test action")
	ActionDone(ctx, errors.New("test error"))

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	logs := span.Logs()
	suite.NotZero(span.ParentID)
	suite.Equal("test action", span.OperationName)

	suite.Len(tags, 3)
	suite.Contains(tags, "action")
	suite.Contains(tags, "span_type")
	suite.Contains(tags, "error")
	suite.EqualValues("test action", tags["action"])
	suite.EqualValues("action", tags["span_type"])
	suite.EqualValues(true, tags["error"])

	suite.Len(logs, 1)
	log := logs[0]
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "event", ValueKind: reflect.String, ValueString: "action failed"})
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "error.object", ValueKind: reflect.String, ValueString: "test error"})
}

func (suite *actionTraceSuite) TestActionLog() {
	ctx := ActionWithNameStarted(suite.ctx, "test action")
	TraceEvent(ctx, "test event", log.String("field", "value"))
	ActionDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	log := span.Logs()[0]

	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "event", ValueKind: reflect.String, ValueString: "test event"})
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "field", ValueKind: reflect.String, ValueString: "value"})
}

func (suite *actionTraceSuite) TestActionTag() {
	ctx := ActionWithNameStarted(suite.ctx, "test action")
	TraceTag(ctx, opentracing.Tag{Key: "key", Value: "value"})
	ActionDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	suite.Contains(tags, "key")
	suite.EqualValues("value", tags["key"])
}

type queryTraceSuite struct {
	baseSuite
}

func TestQueryTracingSuite(t *testing.T) {
	suite.Run(t, new(queryTraceSuite))
}

func (suite *queryTraceSuite) TestQueryOk() {
	ctx := QueryWithNameStarted(suite.ctx, "test query")
	QueryDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	logs := span.Logs()
	suite.NotZero(span.ParentID)
	suite.Equal("test query", span.OperationName)

	suite.Len(tags, 2)
	suite.Contains(tags, "query_name")
	suite.Contains(tags, "span_type")
	suite.EqualValues("test query", tags["query_name"])
	suite.EqualValues("query", tags["span_type"])

	suite.Empty(logs)
}

func (suite *queryTraceSuite) TestQueryErr() {
	ctx := QueryWithNameStarted(suite.ctx, "test query")
	QueryDone(ctx, errors.New("test error"))

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	logs := span.Logs()
	suite.NotZero(span.ParentID)
	suite.Equal("test query", span.OperationName)

	suite.Len(tags, 3)
	suite.Contains(tags, "query_name")
	suite.Contains(tags, "span_type")
	suite.EqualValues("test query", tags["query_name"])
	suite.Contains(tags, "error")
	suite.EqualValues("query", tags["span_type"])
	suite.EqualValues(true, tags["error"])

	suite.Len(logs, 1)
	log := logs[0]
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "event", ValueKind: reflect.String, ValueString: "query failed"})
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "error.object", ValueKind: reflect.String, ValueString: "test error"})
}

func (suite *queryTraceSuite) TestQueryLog() {
	ctx := QueryWithNameStarted(suite.ctx, "test query")
	TraceEvent(ctx, "test event", log.String("field", "value"))
	QueryDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	log := span.Logs()[0]

	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "event", ValueKind: reflect.String, ValueString: "test event"})
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "field", ValueKind: reflect.String, ValueString: "value"})
}

func (suite *queryTraceSuite) TestQueryTag() {
	ctx := QueryWithNameStarted(suite.ctx, "test query")
	TraceTag(ctx, opentracing.Tag{Key: "key", Value: "value"})
	QueryDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	suite.Contains(tags, "key")
	suite.EqualValues("value", tags["key"])
}

type interconnectTraceSuite struct {
	baseSuite
}

func TestInterconnectTracingSuite(t *testing.T) {
	suite.Run(t, new(interconnectTraceSuite))
}

func (suite *interconnectTraceSuite) TestInterconnectOk() {
	ctx := ICRequestWithNameStarted(suite.ctx, "test system", "test request")
	ICRequestDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	logs := span.Logs()
	suite.NotZero(span.ParentID)
	suite.Equal("test system.test request", span.OperationName)

	suite.Len(tags, 3)
	suite.Contains(tags, "service")
	suite.Contains(tags, "request_name")
	suite.Contains(tags, "span_type")
	suite.EqualValues("test system", tags["service"])
	suite.EqualValues("test request", tags["request_name"])
	suite.EqualValues("rpc", tags["span_type"])

	suite.Empty(logs)
}

func (suite *interconnectTraceSuite) TestInterconnectErr() {
	ctx := ICRequestWithNameStarted(suite.ctx, "test system", "test request")
	ICRequestDone(ctx, errors.New("test error"))

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	logs := span.Logs()
	suite.NotZero(span.ParentID)
	suite.Equal("test system.test request", span.OperationName)

	suite.Len(tags, 4)
	suite.Contains(tags, "service")
	suite.Contains(tags, "request_name")
	suite.Contains(tags, "span_type")
	suite.Contains(tags, "error")
	suite.EqualValues("test system", tags["service"])
	suite.EqualValues("test request", tags["request_name"])
	suite.EqualValues("rpc", tags["span_type"])
	suite.EqualValues(true, tags["error"])

	suite.Len(logs, 1)
	log := logs[0]
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "event", ValueKind: reflect.String, ValueString: "request failed"})
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "error.object", ValueKind: reflect.String, ValueString: "test error"})
}

func (suite *interconnectTraceSuite) TestInterconnectLog() {
	ctx := ICRequestWithNameStarted(suite.ctx, "test system", "test request")
	TraceEvent(ctx, "test event", log.String("field", "value"))
	ICRequestDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	log := span.Logs()[0]

	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "event", ValueKind: reflect.String, ValueString: "test event"})
	suite.Contains(log.Fields, mocktracer.MockKeyValue{Key: "field", ValueKind: reflect.String, ValueString: "value"})
}

func (suite *interconnectTraceSuite) TestInterconnectTag() {
	ctx := ICRequestWithNameStarted(suite.ctx, "test system", "test request")
	TraceTag(ctx, opentracing.Tag{Key: "key", Value: "value"})
	ICRequestDone(ctx, nil)

	spans := suite.tracer.FinishedSpans()
	suite.Require().Len(spans, 1)

	span := spans[0]
	tags := span.Tags()
	suite.Contains(tags, "key")
	suite.EqualValues("value", tags["key"])
}
