package scope

import (
	"context"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/prometheus/client_golang/prometheus"
	dto "github.com/prometheus/client_model/go"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/clock"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type baseTestSuite struct {
	suite.Suite
	metricsTool

	clock       clockwork.FakeClock
	initialTime time.Time

	observer *observer.ObservedLogs
	logger   log.Logger
}

type metricsTool struct{}

func (metricsTool) getMetricsFamily(c prometheus.Collector) *dto.MetricFamily {
	reg := prometheus.NewRegistry()
	reg.MustRegister(c)
	mt, err := reg.Gather()
	if err != nil {
		panic(err)
	}
	if len(mt) > 0 {
		return mt[0]
	}
	return nil
}

func (m metricsTool) getMetric(c prometheus.Collector) *dto.Metric {
	mf := m.getMetricsFamily(c)
	return mf.GetMetric()[0]
}

func (metricsTool) findLabel(lbs []*dto.LabelPair, name string) *dto.LabelPair {
	for _, l := range lbs {
		if l.GetName() == name {
			return l
		}
	}
	return nil
}

func (suite *baseTestSuite) SetupTest() {
	suite.initialTime = time.Date(1984, time.April, 4, 0, 0, 0, 0, time.UTC)
	suite.clock = clockwork.NewFakeClockAt(suite.initialTime)
	clock.SetFakeClock(suite.clock)

	core, obs := observer.New(zapcore.DebugLevel)
	suite.observer = obs
	suite.logger = zap.NewWithCore(core)
}

type baseGrpcRequestTestSuite struct {
	baseTestSuite
	ctx context.Context
}

func (suite *baseGrpcRequestTestSuite) SetupTest() {
	suite.baseTestSuite.SetupTest()
	suite.ctx = context.Background()

	StartGlobal(suite.logger)
}
