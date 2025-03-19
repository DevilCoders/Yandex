package tooling

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type loggingTestSuite struct {
	baseSuite

	observer *observer.ObservedLogs
}

func TestLogging(t *testing.T) {
	suite.Run(t, new(loggingTestSuite))
}

func (suite *loggingTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()

	core, obs := observer.New(zapcore.DebugLevel)
	suite.observer = obs
	logging.SetLogger(zap.NewWithCore(core))
}

func (suite *loggingTestSuite) TestGetLogger() {
	toolLog := Logger(suite.ctx)
	logger := toolLog.Logger()

	logger.Error("debug")

	recs := suite.observer.FilterMessage("debug").AllUntimed()
	suite.Require().Len(recs, 1)

	rec := recs[0]

	fields := rec.ContextMap()
	suite.NotEmpty(fields["request_id"])
	suite.Equal("testing", fields["service"])
	suite.Equal("test/src", fields["source_name"])
	suite.Equal("some.handler", fields["handler"])
}

func (suite *loggingTestSuite) TestCommonLog() {
	logger := Logger(suite.ctx)
	logger.Debug("debug", log.String("k", "v"))

	recs := suite.observer.FilterMessage("debug").AllUntimed()
	suite.Require().Len(recs, 1)

	rec := recs[0]
	wantEntry := zapcore.Entry{
		Level:   zapcore.DebugLevel,
		Message: "debug",
	}
	suite.Equal(wantEntry, rec.Entry)

	fields := rec.ContextMap()
	suite.NotEmpty(fields["request_id"])
	suite.Equal("testing", fields["service"])
	suite.Equal("test/src", fields["source_name"])
	suite.Equal("some.handler", fields["handler"])
	suite.Equal("v", fields["k"])
}

func (suite *loggingTestSuite) TestLogFromAction() {
	ctx := ActionWithNameStarted(suite.ctx, "test action")
	logger := Logger(ctx)
	logger.Debug("debug", log.String("k", "v"))

	recs := suite.observer.FilterMessage("debug").AllUntimed()
	suite.Require().Len(recs, 1)

	rec := recs[0]
	wantEntry := zapcore.Entry{
		Level:   zapcore.DebugLevel,
		Message: "debug",
	}
	suite.Equal(wantEntry, rec.Entry)

	fields := rec.ContextMap()
	suite.NotEmpty(fields["request_id"])
	suite.Equal("testing", fields["service"])
	suite.Equal("test/src", fields["source_name"])
	suite.Equal("some.handler", fields["handler"])
	suite.Equal("test action", fields["action"])
	suite.Equal("v", fields["k"])
}

func (suite *loggingTestSuite) TestLogFromQuery() {
	ctx := ActionWithNameStarted(suite.ctx, "test action")
	ctx = QueryWithNameStarted(ctx, "test query")
	logger := Logger(ctx)
	logger.Debug("debug", log.String("k", "v"))

	recs := suite.observer.FilterMessage("debug").AllUntimed()
	suite.Require().Len(recs, 1)

	rec := recs[0]
	wantEntry := zapcore.Entry{
		Level:   zapcore.DebugLevel,
		Message: "debug",
	}
	suite.Equal(wantEntry, rec.Entry)

	fields := rec.ContextMap()
	suite.NotEmpty(fields["request_id"])
	suite.Equal("testing", fields["service"])
	suite.Equal("test/src", fields["source_name"])
	suite.Equal("some.handler", fields["handler"])
	suite.Equal("test action", fields["action"])
	suite.Equal("test query", fields["query_name"])
	suite.Equal("v", fields["k"])
}

func (suite *loggingTestSuite) TestLogFromRequest() {
	ctx := ActionWithNameStarted(suite.ctx, "test action")
	ctx = ICRequestWithNameStarted(ctx, "test system", "test request")
	logger := Logger(ctx)
	logger.Debug("debug", log.String("k", "v"))

	recs := suite.observer.FilterMessage("debug").AllUntimed()
	suite.Require().Len(recs, 1)

	rec := recs[0]
	wantEntry := zapcore.Entry{
		Level:   zapcore.DebugLevel,
		Message: "debug",
	}
	suite.Equal(wantEntry, rec.Entry)

	fields := rec.ContextMap()
	suite.NotEmpty(fields["request_id"])
	suite.Equal("testing", fields["service"])
	suite.Equal("test/src", fields["source_name"])
	suite.Equal("some.handler", fields["handler"])
	suite.Equal("test action", fields["action"])
	suite.Equal("test system.test request", fields["request"])
	suite.Equal("v", fields["k"])
}

func (suite *loggingTestSuite) TestLogLevels() {
	logger := Logger(suite.ctx)

	cases := []struct {
		name      string
		method    func(string, ...log.Field)
		wantLevel zapcore.Level
	}{
		{"Trace", logger.Trace, zapcore.DebugLevel},
		{"Debug", logger.Debug, zapcore.DebugLevel},
		{"Info", logger.Info, zapcore.InfoLevel},
		{"Warn", logger.Warn, zapcore.WarnLevel},
		{"Error", logger.Error, zapcore.ErrorLevel},
		// {"Fatal", logger.Fatal, zapcore.FatalLevel},
	}

	for _, c := range cases {
		suite.Run(c.name, func() {
			msg := fmt.Sprintf("test message %s", c.name)
			c.method(msg)
			logged := suite.observer.FilterMessage(msg).Len()
			suite.Equal(1, logged)
		})
	}
}
