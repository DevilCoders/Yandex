package uazap

import (
	"bytes"
	"context"
	"errors"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/goleak"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
	"a.yandex-team.ru/library/go/yandex/uagent/log/zap/client/mock"
)

// Most tests could potentially find unexpected complicated bugs,
// so it's advisable to run tests many times (e.g. -count=N for go toolchain).

type clientState struct {
	payload       []byte
	receivedCount int32
}

func commonExpects(client *mock.MockClient, stat *mock.MockStats, state *clientState) {
	client.EXPECT().Stat().Return(stat)
	client.EXPECT().GRPCMaxMessageSize().Return(int64(10_000)).AnyTimes()
	stat.EXPECT().AckedMessages().Return(int64(0)).AnyTimes()
	stat.EXPECT().AckedBytes().Return(int64(0)).AnyTimes()
	stat.EXPECT().DroppedMessages().Return(int64(0)).AnyTimes()
	stat.EXPECT().DroppedBytes().Return(int64(0)).AnyTimes()
	client.EXPECT().Send(gomock.Any()).DoAndReturn(func(messageBatch []uaclient.Message) error {
		atomic.AddInt32(&state.receivedCount, int32(len(messageBatch)))
		return nil
	}).AnyTimes()
}

func TestCompareToDefault(t *testing.T) {
	defer goleak.VerifyNone(t)
	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)

	gomock.InOrder(
		client.EXPECT().Stat().Return(stat),
		client.EXPECT().GRPCMaxMessageSize().Return(int64(10_000)),
		stat.EXPECT().AckedBytes().Return(int64(0)),
		client.EXPECT().Send(gomock.Any()).DoAndReturn(func(messageBatch []uaclient.Message) error {
			require.NotEmpty(t, messageBatch)
			state.payload = messageBatch[0].Payload
			return nil
		}),
	)

	var buf1 bytes.Buffer
	out1 := zapcore.AddSync(&buf1)

	format := zap.NewProductionEncoderConfig()
	format.EncodeTime = func(t time.Time, e zapcore.PrimitiveArrayEncoder) {
		e.AppendString("10:00")
	}

	uaCore := NewCore(zapcore.NewJSONEncoder(format), client, zap.DebugLevel)

	log0 := zap.New(uaCore)
	log0.Error("foo")

	require.NoError(t, uaCore.Sync())
	uaCore.Stop(context.Background())

	syncCore := zapcore.NewCore(zapcore.NewJSONEncoder(format), out1, zap.DebugLevel)

	log1 := zap.New(syncCore)
	log1.Error("foo")

	require.Equal(t, string(state.payload)+"\n", buf1.String())
}

// TestSync basically checks that number of records in sink after sync is not less than logged records before sync.
func TestSync(t *testing.T) {
	defer goleak.VerifyNone(t)
	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)
	commonExpects(client, stat, &state)

	format := zap.NewProductionEncoderConfig()
	format.EncodeTime = func(t time.Time, e zapcore.PrimitiveArrayEncoder) {
		e.AppendString("10:00")
	}

	uaCore := NewCore(
		zapcore.NewJSONEncoder(format),
		client,
		zap.DebugLevel)
	defer uaCore.Stop(context.Background())

	log0 := zap.New(uaCore)

	var logsWritten int32

	var wg sync.WaitGroup
	for i := 0; i < 100; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for j := 0; j < 100; j++ {
				log0.Error("123")
				written := atomic.AddInt32(&logsWritten, 1)
				_ = log0.Sync()

				require.GreaterOrEqual(t, atomic.LoadInt32(&state.receivedCount), written)
			}
		}()
	}

	wg.Wait()
}

// TestStop checks three things:
// 1) Stop includes sync, i.e. that records in sink after stop call >= records written before stop call.
// 2) After stop new records are not accepted (and increase drop metric).
// 3) No goroutines are running after test, i.e. nothing leaks.
func TestStop(t *testing.T) {
	defer goleak.VerifyNone(t)

	const (
		recordsBeforeStop = 50
		recordsAfterStop  = 50
		allRecords        = recordsBeforeStop + recordsAfterStop
	)

	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)
	commonExpects(client, stat, &state)
	stat.EXPECT().ErrorCount().Return(int64(0))

	var wg sync.WaitGroup
	var logsWritten, receivedAfterStop int32

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel)

	log0 := zap.New(uaCore)

	for j := 0; j < recordsBeforeStop; j++ {
		j := j
		wg.Add(1)
		go func() {
			defer wg.Done()

			log0.Error("123")
			written := atomic.AddInt32(&logsWritten, 1)
			if j == recordsBeforeStop/2 {
				uaCore.Stop(context.Background())
				receivedAfterStop = atomic.LoadInt32(&state.receivedCount)
				require.GreaterOrEqual(t, receivedAfterStop, written)
			}
		}()
	}
	wg.Wait()

	for i := 0; i < recordsAfterStop; i++ {
		log0.Error("123")
	}

	assert.Equal(t, receivedAfterStop, atomic.LoadInt32(&state.receivedCount))

	stats := uaCore.Stat()
	expectedDropped := int(allRecords - receivedAfterStop)

	// At least records after stop are dropped.
	assert.GreaterOrEqual(t, expectedDropped, recordsAfterStop)
	assert.EqualValues(t, expectedDropped, stats.DroppedMessages)
}

// TestStat checks the following things:
// 1) ReceiveMessages equals number of tries to emplace log into queue.
// Note that in case of encode errors this metric isn't increased.
// 2) DroppedMessages is increased on
// GRPCMaxMessageBytes exceeded, core stopped (including timeout error), MaxMemoryUsage exceeded.
// 3) InflightMessages is increased if only not dropped.
func TestStat(t *testing.T) {
	defer goleak.VerifyNone(t)
	const (
		recordsBeforeStop             = 50
		recordsAfterStop              = 10
		recordsGRPCMaxBytesExceeded   = 5
		recordsMaxMemoryUsageExceeded = 3
		allRecords                    = recordsBeforeStop + recordsAfterStop +
			recordsGRPCMaxBytesExceeded + recordsMaxMemoryUsageExceeded

		recordsBelowLogLevel = 3 // Not processed and doesn't affect metrics.
	)

	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)
	commonExpects(client, stat, &state)
	stat.EXPECT().ErrorCount().Return(int64(0))

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.InfoLevel,
		WithMaxMemoryUsage(5_000))

	log0 := zap.New(uaCore)

	for i := 0; i < recordsBelowLogLevel; i++ {
		log0.Debug("123")
	}
	for i := 0; i < recordsBeforeStop; i++ {
		log0.Error("123")
	}
	for i := 0; i < recordsGRPCMaxBytesExceeded; i++ {
		log0.Error(strings.Repeat("1", int(client.GRPCMaxMessageSize()+1)))
	}
	for i := 0; i < recordsMaxMemoryUsageExceeded; i++ {
		leftSpace := uaCore.bg.options.maxMemoryUsage - int(uaCore.bg.stat.InflightBytes)

		// Ensure that encoded message with leftSpace bytes won't exceed MaxMessageSize.
		require.Less(t, leftSpace, int(client.GRPCMaxMessageSize()/2))
		require.Positive(t, leftSpace)
		log0.Error(strings.Repeat("1", leftSpace))
	}
	uaCore.Stop(context.Background())

	for i := 0; i < recordsAfterStop; i++ {
		log0.Error("123")
	}

	stats := uaCore.Stat()
	assert.EqualValues(t, allRecords, stats.ReceivedMessages)
	assert.EqualValues(t, recordsBeforeStop, stats.InflightMessages)
	assert.EqualValues(t,
		recordsAfterStop+recordsGRPCMaxBytesExceeded+recordsMaxMemoryUsageExceeded, stats.DroppedMessages)
}

func TestSecondStopNoDeadlockOrPanic(t *testing.T) {
	defer goleak.VerifyNone(t)

	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)
	commonExpects(client, stat, &state)

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel,
	)

	uaCore.Stop(context.Background())

	uaCore.Stop(context.Background())
}

func TestSyncStopNoDeadlock(t *testing.T) {
	defer goleak.VerifyNone(t)
	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)
	commonExpects(client, stat, &state)

	const syncsNumber = 50

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel,
	)

	log0 := zap.New(uaCore)

	const (
		stateBeforeStop = iota
		stateDuringStop
		stateAfterStop
	)
	var stopState int32

	var wg sync.WaitGroup
	for i := 0; i < syncsNumber; i++ {
		i := i
		wg.Add(1)
		go func() {
			defer wg.Done()

			log0.Error("123")

			if i == syncsNumber/3 {
				atomic.StoreInt32(&stopState, stateDuringStop)
				uaCore.Stop(context.Background())
				atomic.StoreInt32(&stopState, stateAfterStop)
			}

			stateBeforeSync := atomic.LoadInt32(&stopState)
			err := uaCore.Sync()
			stateAfterSync := atomic.LoadInt32(&stopState)

			if stateBeforeSync == stateAfterStop {
				require.Error(t, err)
			}
			if stateAfterSync == stateBeforeStop {
				require.NoError(t, err)
			}

			log0.Error("321")
		}()
	}
	wg.Wait()
}

func TestSendError(t *testing.T) {
	defer goleak.VerifyNone(t)

	ctrl := gomock.NewController(t)
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)

	clientErr := errors.New("any error")

	gomock.InOrder(
		client.EXPECT().Stat().Return(stat),
		client.EXPECT().GRPCMaxMessageSize().Return(int64(10_000)),
		stat.EXPECT().AckedBytes().Return(int64(0)),
		client.EXPECT().Send(gomock.Any()).Return(clientErr),
		stat.EXPECT().AckedMessages().Return(int64(0)),
		stat.EXPECT().AckedBytes().Return(int64(0)),
		stat.EXPECT().DroppedMessages().Return(int64(0)),
		stat.EXPECT().DroppedBytes().Return(int64(0)),
		stat.EXPECT().ErrorCount().Return(int64(1)),
	)

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel)
	defer uaCore.Stop(context.Background())

	log0 := zap.New(uaCore)

	log0.Error("err")

	assert.ErrorIs(t, uaCore.Sync(), clientErr)

	stats := uaCore.Stat()
	assert.EqualValues(t, 1, stats.ReceivedMessages)
	// Client should drop messages itself.
	assert.Zero(t, stats.DroppedMessages)
	assert.Zero(t, stats.InflightMessages)
}

func TestStopTimeout(t *testing.T) {
	defer goleak.VerifyNone(t)

	ctrl := gomock.NewController(t)
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)

	gomock.InOrder(
		client.EXPECT().Stat().Return(stat),
		client.EXPECT().GRPCMaxMessageSize().Return(int64(10_000)),
		stat.EXPECT().AckedBytes().Return(int64(0)),
		client.EXPECT().Send(gomock.Any()).Return(nil),
		stat.EXPECT().AckedMessages().Return(int64(1)),
		stat.EXPECT().AckedBytes().Return(int64(0)),
		stat.EXPECT().DroppedMessages().Return(int64(0)),
		stat.EXPECT().DroppedBytes().Return(int64(0)),
		stat.EXPECT().ErrorCount().Return(int64(1)),
	)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel)

	log0 := zap.New(uaCore)
	log0.Error("err")

	cancel()
	uaCore.Stop(ctx)

	stats := uaCore.Stat()
	assert.EqualValues(t, 1, stats.ReceivedMessages)
	// Client should have sent the message after sync (see mock).
	assert.Zero(t, stats.InflightMessages)
	assert.Zero(t, stats.DroppedMessages)
}
