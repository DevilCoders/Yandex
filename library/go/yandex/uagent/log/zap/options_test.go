package uazap

import (
	"context"
	"runtime"
	"strings"
	"sync/atomic"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/goleak"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/time/rate"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
	"a.yandex-team.ru/library/go/yandex/uagent/log/zap/client/mock"
)

func TestFlushInterval(t *testing.T) {
	defer goleak.VerifyNone(t)

	const recordsNumber = 50

	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)
	commonExpects(client, stat, &state)

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel,
		WithFlushInterval(10*time.Minute),
	)

	// Wait first tick.
	for {
		uaCore.bg.mu.Lock()
		iter := uaCore.bg.iter
		uaCore.bg.mu.Unlock()

		if iter != 0 {
			break
		}
		runtime.Gosched()
	}

	log0 := zap.New(uaCore)

	for i := 0; i < recordsNumber; i++ {
		log0.Error("123")
	}

	assert.Zero(t, atomic.LoadInt32(&state.receivedCount))

	uaCore.Stop(context.Background())

	assert.EqualValues(t, recordsNumber, atomic.LoadInt32(&state.receivedCount))
}

func TestRateLimiter(t *testing.T) {
	ctrl := gomock.NewController(t)

	var state clientState
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)
	commonExpects(client, stat, &state)

	const burstLimit = 100

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel,
		WithRateLimiter(rate.NewLimiter(1.0, burstLimit)),
	)

	log0 := zap.New(uaCore)

	log0.Error(strings.Repeat("1", burstLimit+1))
	log0.Error("123")
	log0.Error("123")
	log0.Error(strings.Repeat("1", burstLimit+1))

	uaCore.Stop(context.Background())

	assert.EqualValues(t, 1, atomic.LoadInt32(&state.receivedCount))
}

func TestMetaExtractor(t *testing.T) {
	ctrl := gomock.NewController(t)

	var gotMeta map[string]string
	client, stat := mock.NewMockClient(ctrl), mock.NewMockStats(ctrl)

	gomock.InOrder(
		client.EXPECT().Stat().Return(stat),
		client.EXPECT().GRPCMaxMessageSize().Return(int64(10_000)),
		stat.EXPECT().AckedBytes().Return(int64(0)),
		client.EXPECT().Send(gomock.Any()).DoAndReturn(func(messageBatch []uaclient.Message) error {
			require.Len(t, messageBatch, 1)
			gotMeta = messageBatch[0].Meta
			return nil
		}),
	)

	uaCore := NewCore(
		zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
		client,
		zap.DebugLevel,
		WithMetaExtractor(func(entry zapcore.Entry, fields []zapcore.Field) map[string]string {
			baseMeta := PriorityMetaExtractor(entry, fields)
			baseMeta["_logger_name"] = "pavel"
			return baseMeta
		}),
	)
	defer uaCore.Stop(context.Background())

	log0 := zap.New(uaCore)
	log0.Error("123")

	require.NoError(t, uaCore.Sync())

	assert.Equal(t, map[string]string{
		"_priority":    "error",
		"_logger_name": "pavel",
	}, gotMeta)
}
