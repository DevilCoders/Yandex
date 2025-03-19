package tooling

import (
	"context"
	"io"

	"github.com/jonboulle/clockwork"
	"github.com/opentracing/opentracing-go/mocktracer"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracing"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type baseSuite struct {
	suite.Suite
	ctx    context.Context
	tracer *mocktracer.MockTracer

	clock clockwork.FakeClock
}

func (suite *baseSuite) SetupSuite() {
	suite.tracer = mocktracer.New()
	tracing.SetTracer(suite.tracer, io.NopCloser(nil))
}

func (suite *baseSuite) SetupTest() {
	zl := zaptest.NewLogger(suite.T())
	logging.SetLogger(&zap.Logger{L: zl})

	var cm ContextModifier
	suite.ctx, cm = InitContext(context.TODO(), "testing")
	suite.clock = clockwork.NewFakeClock()

	suite.tracer.Reset()
	cm.SetSource("test/src").
		SetHandler("some.handler").
		InitTracing("testing").
		mockClock(suite.clock)

	metrics.ResetMetrics()
}

func (suite *baseSuite) TearDownTest() {
	ModifyContext(suite.ctx).
		FinishTracing()

	logging.SetLogger(&nop.Logger{})
	features.SetDefault(features.Flags{})
}

func (cm ContextModifier) mockClock(clock clockwork.Clock) ContextModifier {
	cm.ctxStore.clockOverride = clock
	return cm
}
