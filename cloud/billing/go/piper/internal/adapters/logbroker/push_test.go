package logbroker

import (
	context "context"
	"testing"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

type basePushSuite struct {
	clientMockedSuite
	scope   entities.ProcessingScope
	adapter *Adapter
	sess    *Session
}

func (suite *basePushSuite) SetupTest() {
	suite.clientMockedSuite.SetupTest()
	suite.adapter, _ = New(suite.ctx, Writers{})
	suite.scope.SourceID = "source"
	suite.sess = suite.adapter.Session()
	suite.sess.backoffOverride = func() backoff.BackOff {
		return suite
	}
}

func (suite *basePushSuite) NextBackOff() time.Duration {
	return backoff.Stop
}

func (suite *basePushSuite) Reset() {}

type pushIncorrectSuite struct {
	basePushSuite
}

func TestPushIncorrect(t *testing.T) {
	suite.Run(t, new(pushIncorrectSuite))
}

func (suite *pushIncorrectSuite) SetupTest() {
	suite.basePushSuite.SetupTest()
	suite.adapter.writers.IncorrectMetrics = suite.mockIncorrect
}

func (suite *pushIncorrectSuite) TestEmptyPush() {
	err := suite.sess.FlushInvalidMetrics(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *pushIncorrectSuite) TestPush() {
	metrics := []entities.InvalidMetric{
		{IncorrectRawMessage: entities.IncorrectRawMessage{MessageOffset: 99}},
		{IncorrectRawMessage: entities.IncorrectRawMessage{MessageOffset: 42}},
	}
	for _, m := range metrics {
		err := suite.sess.PushInvalidMetric(suite.ctx, suite.scope, m)
		suite.Require().NoError(err)
	}

	var callMessages []lbtypes.ShardMessage
	suite.mockIncorrect.
		On("Write", mock.Anything, lbtypes.SourceID("source"), uint32(0), mock.Anything).
		Return(func(_ context.Context, _ lbtypes.SourceID, _ uint32, msgs []lbtypes.ShardMessage) uint64 {
			callMessages = msgs
			return 999
		}, nil)

	err := suite.sess.FlushInvalidMetrics(suite.ctx)
	suite.Require().NoError(err)
	suite.Require().Len(callMessages, 2)
	suite.EqualValues(42, callMessages[0].Offset())
	suite.EqualValues(99, callMessages[1].Offset())

	suite.mockIncorrect.AssertExpectations(suite.T())
}

func (suite *pushIncorrectSuite) TestPushError() {
	err := suite.sess.PushInvalidMetric(suite.ctx, suite.scope,
		entities.InvalidMetric{IncorrectRawMessage: entities.IncorrectRawMessage{MessageOffset: 99}},
	)
	suite.Require().NoError(err)
	suite.mockIncorrect.
		On("Write", mock.Anything, lbtypes.SourceID("source"), uint32(0), mock.Anything).
		Return(uint64(0), errTest)

	err = suite.sess.FlushInvalidMetrics(suite.ctx)
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrWrite)
	suite.ErrorIs(err, errTest)
}

type pushEnrichedSuite struct {
	basePushSuite
}

func TestPushEnriched(t *testing.T) {
	suite.Run(t, new(pushEnrichedSuite))
}

func (suite *pushEnrichedSuite) SetupTest() {
	suite.basePushSuite.SetupTest()
	suite.adapter.writers.ReshardedMetrics = suite.mockResharded
}

func (suite *pushEnrichedSuite) TestEmptyPush() {
	err := suite.sess.FlushEnriched(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *pushEnrichedSuite) TestPush() {
	callMessages := make([][]lbtypes.ShardMessage, 2)
	suite.mockResharded.
		On("PartitionsCount").Return(uint32(2)).
		On("Write", mock.Anything, lbtypes.SourceID("source"), mock.Anything, mock.Anything).
		Return(func(_ context.Context, _ lbtypes.SourceID, part uint32, msgs []lbtypes.ShardMessage) uint64 {
			callMessages[part] = append(callMessages[part], msgs...)
			return 999
		}, nil)

	metrics := make([]entities.EnrichedMetric, 2)
	metrics[0].MessageOffset = 99
	metrics[1].MessageOffset = 42
	for part := 0; part < 2; part++ {
		for _, m := range metrics {
			err := suite.sess.PushEnrichedMetricToPartition(suite.ctx, suite.scope, m, part)
			suite.Require().NoError(err)
		}
	}

	err := suite.sess.FlushEnriched(suite.ctx)
	suite.Require().NoError(err)
	for part, partMessages := range callMessages {
		suite.Require().Len(partMessages, 2, "partition: %d", part)
		suite.EqualValues(42, partMessages[0].Offset(), "partition: %d", part)
		suite.EqualValues(99, partMessages[1].Offset(), "partition: %d", part)
	}
	suite.mockIncorrect.AssertExpectations(suite.T())
}

func (suite *pushEnrichedSuite) TestPushNoPartition() {
	suite.mockResharded.
		On("PartitionsCount").Return(uint32(2))

	err := suite.sess.PushEnrichedMetricToPartition(suite.ctx, suite.scope,
		entities.EnrichedMetric{}, 2,
	)
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrNoSuchPartition)
}

func (suite *pushEnrichedSuite) TestPushError() {
	suite.mockResharded.
		On("PartitionsCount").Return(uint32(2)).
		On("Write", mock.Anything, lbtypes.SourceID("source"), mock.Anything, mock.Anything).
		Return(uint64(0), errTest)

	err := suite.sess.PushEnrichedMetricToPartition(suite.ctx, suite.scope,
		entities.EnrichedMetric{}, 1,
	)
	suite.Require().NoError(err)

	err = suite.sess.FlushEnriched(suite.ctx)
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrWrite)
	suite.ErrorIs(err, errTest)
}
