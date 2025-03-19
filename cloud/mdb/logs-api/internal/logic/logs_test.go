package logic

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/jonboulle/clockwork"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	authmock "a.yandex-team.ru/cloud/mdb/logs-api/internal/auth/mocks"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb"
	logsdbmock "a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type logsFixture struct {
	Auth   *authmock.MockAuthenticator
	LogsDB *logsdbmock.MockBackend
	Clock  clockwork.FakeClock
}

type logsOutputs struct {
	Logs []models.Log
	More bool
	Err  error
}

func (f logsFixture) ExpectLogs(ctx context.Context, l Logs, criteria models.Criteria, outputs logsOutputs) {
	if !criteria.From.Valid {
		criteria.From = models.DefaultFrom(f.Clock)
	}

	if !criteria.To.Valid {
		criteria.To = models.DefaultTo(f.Clock)
	}

	if !criteria.Limit.Valid {
		criteria.Limit = optional.NewInt64(100)
	}
	if !criteria.Offset.Valid {
		criteria.Offset = optional.NewInt64(0)
	}

	f.LogsDB.EXPECT().Logs(ctx, l.criteriaToDB(criteria)).
		Return(outputs.Logs, outputs.More, outputs.Err).
		Do(func(_, _ interface{}) { f.Clock.Advance(time.Second) })

}

func (f logsFixture) ExpectAuth(ctx context.Context, resources []models.LogSource) {
	f.Auth.EXPECT().Authorize(ctx, resources).Return(nil).AnyTimes()
}

func (f logsFixture) ExpectStreamingLogs(ctx context.Context, logs Logs, criteria models.Criteria, outputs []models.Log, offset int64, prev *gomock.Call, found bool, timeTics time.Duration) *gomock.Call {
	criteria.Offset = optional.NewInt64(offset)
	criteria.To = models.DefaultTo(f.Clock)
	criteria.To.Time = criteria.To.Time.Add(time.Second * timeTics)

	call := f.LogsDB.EXPECT().Logs(ctx, logs.criteriaToDB(criteria))
	if found {
		call = call.Return([]models.Log{outputs[offset]}, true, nil)
	} else {
		call = call.Return(nil, true, nil)
	}

	call = call.Do(func(_, _ interface{}) { f.Clock.Advance(time.Second) })

	if prev != nil {
		call = call.After(prev)
	}

	return call
}

func newLogsFixture(t *testing.T) (context.Context, Logs, logsFixture) {
	ctrl := gomock.NewController(t)
	auth := authmock.NewMockAuthenticator(ctrl)
	logsdb := logsdbmock.NewMockBackend(ctrl)
	clock := clockwork.NewFakeClock()

	return context.Background(), NewLogs(Config{TailWaitPeriod: time.Second}, auth, logsdb, clock), logsFixture{
		Auth:   auth,
		LogsDB: logsdb,
		Clock:  clock,
	}
}

func TestLogs_logs_Defaults(t *testing.T) {
	t.Run("Defaults", func(t *testing.T) {
		ctx, logs, f := newLogsFixture(t)

		sources := []models.LogSource{{
			Type: models.LogSourceTypeClickhouse,
			ID:   testutil.NewUUIDStr(t),
		}}
		criteria := models.Criteria{
			Sources: sources,
		}

		out := logsOutputs{
			Logs: []models.Log{
				{
					Timestamp: time.Now(),
					Severity:  models.LogLevelDebug,
					Message:   "msg1 " + testutil.NewUUIDStr(t),
					Offset:    83,
				},
				{
					Timestamp: time.Now(),
					Severity:  models.LogLevelWarning,
					Message:   "msg2 " + testutil.NewUUIDStr(t),
					Offset:    84,
				},
			},
			More: true,
		}

		f.ExpectAuth(ctx, sources)
		f.ExpectLogs(ctx, logs, criteria, out)

		msg, _, offset, err := logs.ListLogs(ctx, criteria)
		require.NoError(t, err)
		require.Equal(t, msg, out.Logs)
		require.Equal(t, int64(84), offset)
	})

	t.Run("SanitizeUTF8", func(t *testing.T) {
		ctx, logs, f := newLogsFixture(t)

		sources := []models.LogSource{{
			Type: models.LogSourceTypeClickhouse,
			ID:   testutil.NewUUIDStr(t),
		}}
		criteria := models.Criteria{
			Sources: sources,
		}
		out := logsOutputs{
			Logs: []models.Log{
				{
					Timestamp: time.Now(),
					Severity:  models.LogLevelDebug,
					Message:   "msg1 msg\u0000msg",
					Offset:    83,
				},
				{
					Timestamp: time.Now(),
					Severity:  models.LogLevelWarning,
					Message:   "msg2 msg\u0000msg",
					Offset:    84,
				},
			},
			More: true,
		}

		expected := out
		expected.Logs[0].Message = "msg2 msg?msg"
		expected.Logs[1].Message = "msg2 msg?msg"

		f.ExpectAuth(ctx, sources)
		f.ExpectLogs(ctx, logs, criteria, out)

		msg, _, offset, err := logs.ListLogs(ctx, criteria)
		require.NoError(t, err)
		require.Equal(t, msg, expected.Logs)
		require.Equal(t, int64(84), offset)
	})
}

func TestLogs_Stream_Simple(t *testing.T) {
	ctx, logs, f := newLogsFixture(t)

	sources := []models.LogSource{{
		Type: models.LogSourceTypeClickhouse,
		ID:   testutil.NewUUIDStr(t),
	}}
	criteria := models.Criteria{
		Sources: sources,
		From:    optional.NewTime(f.Clock.Now()),
		To:      optional.NewTime(f.Clock.Now()),
	}

	out := logsOutputs{Logs: []models.Log{
		{
			Timestamp: time.Now(),
			Severity:  models.LogLevelDebug,
			Message:   "msg1" + testutil.NewUUIDStr(t),
		},
		{
			Timestamp: time.Now(),
			Severity:  models.LogLevelError,
			Message:   "msg2" + testutil.NewUUIDStr(t),
		},
	},
	}

	f.ExpectAuth(ctx, sources)
	f.ExpectLogs(ctx, logs, criteria, out)

	ch, err := logs.StreamLogs(ctx, criteria)
	require.NoError(t, err)
	require.NotNil(t, ch)

	var count int
	for batch := range ch {
		require.NoError(t, batch.Err)

		for _, msg := range batch.Logs {
			require.Contains(t, out.Logs, msg)
			count++
		}
	}
	require.Equal(t, len(out.Logs), count)
}

func TestLogs_Stream_Defaults(t *testing.T) {
	ctx, logs, f := newLogsFixture(t)

	sources := []models.LogSource{{
		Type: models.LogSourceTypeClickhouse,
		ID:   testutil.NewUUIDStr(t),
	}}
	criteria := models.Criteria{
		Sources: sources,
		From:    optional.NewTime(f.Clock.Now()),
		To:      optional.NewTime(f.Clock.Now()),
		Offset:  optional.NewInt64(0),
		Limit:   optional.NewInt64(DefaultConfig().BatchSize),
	}

	out := []models.Log{
		{
			Timestamp: time.Now(),
			Message:   "msg1" + testutil.NewUUIDStr(t),
			Offset:    1,
		},
		{
			Timestamp: time.Now(),
			Message:   "msg2" + testutil.NewUUIDStr(t),
			Offset:    2,
		},
	}

	f.ExpectAuth(ctx, sources)

	// FromTS must be equal in both calls
	var firstFrom int64
	f.LogsDB.EXPECT().Logs(ctx, logs.criteriaToDB(criteria)).Return([]models.Log{out[criteria.Offset.Int64]}, true, nil).
		Do(func(_ interface{}, c logsdb.Criteria) {
			firstFrom = c.FromSeconds
			f.Clock.Advance(time.Second)
		})

	nextCriteria := logs.criteriaToDB(criteria)
	nextCriteria.Offset += 1
	f.LogsDB.EXPECT().Logs(ctx, nextCriteria).Return([]models.Log{out[nextCriteria.Offset]}, false, nil).
		Do(func(_ interface{}, c logsdb.Criteria) {
			require.Equal(t, firstFrom, c.FromSeconds)
		})

	ch, err := logs.StreamLogs(ctx, criteria)
	require.NoError(t, err)
	require.NotNil(t, ch)

	var count int
	for batch := range ch {
		require.NoError(t, batch.Err)

		for _, msg := range batch.Logs {
			require.Contains(t, out, msg)
			count++
		}
	}
	require.Equal(t, len(out), count)
}

func TestLogs_Stream_TailF(t *testing.T) {
	ctx, logs, f := newLogsFixture(t)
	var cancel context.CancelFunc
	ctx, cancel = context.WithTimeout(ctx, time.Second*5)
	defer cancel()

	sources := []models.LogSource{{
		Type: models.LogSourceTypeClickhouse,
		ID:   testutil.NewUUIDStr(t),
	}}
	criteria := models.Criteria{
		Sources: sources,
		From:    optional.NewTime(f.Clock.Now()),
		Offset:  optional.NewInt64(0),
		Limit:   optional.NewInt64(DefaultConfig().BatchSize),
	}

	// Log messages we will be receiving
	out := []models.Log{
		{
			Timestamp: time.Now(),
			Message:   "msg1" + testutil.NewUUIDStr(t),
			Offset:    1,
		},
		{
			Timestamp: time.Now(),
			Message:   "msg2" + testutil.NewUUIDStr(t),
			Offset:    2,
		},
		{
			Timestamp: time.Now(),
			Message:   "msg3" + testutil.NewUUIDStr(t),
			Offset:    3,
		},
	}

	f.ExpectAuth(ctx, sources)

	var call *gomock.Call
	// Received first message
	call = f.ExpectStreamingLogs(ctx, logs, criteria, out, 0, call, true, 0)
	// Received second message
	call = f.ExpectStreamingLogs(ctx, logs, criteria, out, 1, call, true, 1)
	// No messages in database
	call = f.ExpectStreamingLogs(ctx, logs, criteria, out, 2, call, false, 2)
	// Received third message
	call = f.ExpectStreamingLogs(ctx, logs, criteria, out, 2, call, true, 3)
	// No messages in database
	{
		callCriteria := criteria
		callCriteria.Offset.Int64 = 3
		call = f.LogsDB.EXPECT().Logs(ctx, gomock.Any()).Return(nil, true, nil).AnyTimes().After(call)
		// We cannot expect this method to be called only once because its racy
		// If context is canceled, replace returned error with the one in context
		call.Do(func(arg0, _ interface{}) {
			ctx := arg0.(context.Context)
			if ctx.Err() != nil {
				call.Return(nil, true, ctx.Err())
			}
		})
	}

	// Lets stream!
	ch, err := logs.StreamLogs(ctx, criteria)
	require.NoError(t, err)
	require.NotNil(t, ch)

	var count int
	var canceled bool
	for batch := range ch {
		if batch.Err != nil {
			// Check that error holds context error
			require.True(t, xerrors.Is(batch.Err, ctx.Err()))
			// Channel must be closed now
			continue
		}

		// Verify logs
		for _, msg := range batch.Logs {
			require.Contains(t, out, msg)
			count++
		}

		// Do we need to cancel?
		if count >= len(out) {
			cancel()
			canceled = true
		}
	}

	// Check that we canceled context
	require.True(t, canceled)
	require.Error(t, ctx.Err())
	require.Equal(t, len(out), count)
}
