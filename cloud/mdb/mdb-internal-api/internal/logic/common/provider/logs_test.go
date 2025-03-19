package provider

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/jonboulle/clockwork"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	logsdbmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type logsFixture struct {
	*testhelpers.Fixture
	LogsDB *logsdbmock.MockBackend
	Logs   *Logs
	Clock  clockwork.FakeClock
}

type logsOutputs struct {
	Logs []logs.Message
	More bool
	Err  error
}

func (f logsFixture) ExpectClusterByClusterID(ctx context.Context, cid string, ct clusters.Type) {
	f.Reader.EXPECT().ClusterByClusterID(ctx, cid, ct, models.VisibilityVisible).
		Return(clusterslogic.Cluster{}, nil)
}

func (f logsFixture) ExpectLogs(ctx context.Context, cid string, lt logsdb.LogType, limit int64, opts common.LogsOptions, outputs logsOutputs) {
	if !opts.FromTS.Valid {
		opts.FromTS.Time = defaultFromTS(f.Clock)
	}

	if !opts.ToTS.Valid {
		opts.ToTS.Time = defaultToTS(f.Clock)
	}

	f.LogsDB.EXPECT().Logs(ctx, cid, lt, opts.ColumnFilter, opts.FromTS.Time, opts.ToTS.Time, limit, opts.Offset.Int64, opts.Filter).
		Return(outputs.Logs, outputs.More, outputs.Err).
		Do(func(_, _, _, _, _, _, _, _, _ interface{}) { f.Clock.Advance(time.Second) })
}

func newLogsFixture(t *testing.T) (context.Context, logsFixture) {
	ctx, f := testhelpers.NewFixture(t)
	logsDB := logsdbmock.NewMockBackend(f.Ctrl)
	cfg := logic.DefaultConfig()
	cfg.Logs.TailWaitPeriod = 0
	cfg.Logs.Retry.MaxRetries = 0
	clock := clockwork.NewFakeClock()
	l := NewLogs(cfg, f.Sessions, f.Reader, logsDB, clock)
	return ctx, logsFixture{Fixture: f, LogsDB: logsDB, Logs: l, Clock: clock}
}

func TestLogs_logs_Defaults(t *testing.T) {
	t.Run("Defaults", func(t *testing.T) {
		ctx, f := newLogsFixture(t)
		cid := testutil.NewUUIDStr(t)
		st := logs.ServiceTypeClickHouse
		limit := int64(42)
		opts := common.LogsOptions{}
		out := logsOutputs{
			Logs: []logs.Message{
				{
					Timestamp:        time.Now(),
					Message:          map[string]string{"msg1": testutil.NewUUIDStr(t)},
					NextMessageToken: 83,
				},
				{
					Timestamp:        time.Now(),
					Message:          map[string]string{"msg2": testutil.NewUUIDStr(t)},
					NextMessageToken: 84,
				},
			},
			More: true,
		}

		f.ExpectLogs(ctx, cid, logsdb.LogsServiceTypeToLogType(st)[0], limit, opts, out)

		msg, more, err := f.Logs.logs(ctx, cid, st, limit, opts)
		require.NoError(t, err)
		require.Equal(t, out.More, more)
		require.Equal(t, msg, out.Logs)
	})

	t.Run("SaneValues", func(t *testing.T) {
		ctx, f := newLogsFixture(t)
		cid := testutil.NewUUIDStr(t)
		st := logs.ServiceTypeClickHouse
		out := logsOutputs{
			Logs: []logs.Message{
				{
					Timestamp:        time.Now(),
					Message:          map[string]string{"msg1": testutil.NewUUIDStr(t)},
					NextMessageToken: 83,
				},
				{
					Timestamp:        time.Now(),
					Message:          map[string]string{"msg2": testutil.NewUUIDStr(t)},
					NextMessageToken: 84,
				},
			},
			More: true,
		}

		f.ExpectLogs(ctx, cid, logsdb.LogsServiceTypeToLogType(st)[0], 100, common.LogsOptions{Offset: optional.NewInt64(0)}, out)

		msg, more, err := f.Logs.logs(ctx, cid, st, -1, common.LogsOptions{Offset: optional.NewInt64(-1)})
		require.NoError(t, err)
		require.Equal(t, out.More, more)
		require.Equal(t, msg, out.Logs)
	})

	// MultiService is for service type that maps to multiple log service types.
	// The code it tests and the test itself should probably be removed when PGBouncer is gone.
	t.Run("MultiService", func(t *testing.T) {
		ctx, f := newLogsFixture(t)
		cid := testutil.NewUUIDStr(t)
		st := logs.ServiceTypePooler
		limit := int64(42)
		opts := common.LogsOptions{}
		out := logsOutputs{
			Logs: []logs.Message{
				{
					Timestamp: time.Now(),
					Message:   map[string]string{"msg1": testutil.NewUUIDStr(t)},
				},
				{
					Timestamp: time.Now(),
					Message:   map[string]string{"msg2": testutil.NewUUIDStr(t)},
				},
			},
			More: true,
		}
		lsts := logsdb.LogsServiceTypeToLogType(st)
		require.Len(t, lsts, 2)

		// Receive messages from first log service, do not attempt to get messages from the second one
		f.ExpectLogs(ctx, cid, lsts[0], limit, opts, out)
		msg, more, err := f.Logs.logs(ctx, cid, st, limit, opts)
		require.NoError(t, err)
		require.Equal(t, out.More, more)
		require.Equal(t, msg, out.Logs)

		// Receive messages from second log service with the first one returning nothing
		f.ExpectLogs(ctx, cid, lsts[0], limit, opts, logsOutputs{})
		f.ExpectLogs(ctx, cid, lsts[1], limit, opts, out)
		msg, more, err = f.Logs.logs(ctx, cid, st, limit, opts)
		require.NoError(t, err)
		require.Equal(t, out.More, more)
		require.Equal(t, msg, out.Logs)
	})

	t.Run("SanitizeUTF8", func(t *testing.T) {
		ctx, f := newLogsFixture(t)
		cid := testutil.NewUUIDStr(t)
		st := logs.ServiceTypeClickHouse
		out := logsOutputs{
			Logs: []logs.Message{
				{
					Timestamp:        time.Now(),
					Message:          map[string]string{"msg1": "msg\u0000msg"},
					NextMessageToken: 83,
				},
				{
					Timestamp:        time.Now(),
					Message:          map[string]string{"msg2": "msg\u0000msg"},
					NextMessageToken: 84,
				},
			},
			More: true,
		}
		expected := out
		expected.Logs[0].Message = map[string]string{"msg1": "msg?msg"}
		expected.Logs[1].Message = map[string]string{"msg2": "msg?msg"}

		f.ExpectLogs(ctx, cid, logsdb.LogsServiceTypeToLogType(st)[0], 100, common.LogsOptions{Offset: optional.NewInt64(0)}, out)

		msg, more, err := f.Logs.logs(ctx, cid, st, -1, common.LogsOptions{Offset: optional.NewInt64(-1)})
		require.NoError(t, err)
		require.Equal(t, expected.More, more)
		require.Equal(t, msg, expected.Logs)
	})
}

func TestLogs_Logs(t *testing.T) {
	inputs := []struct {
		Name          string
		Limit         int64
		ExpectedLimit int64
		Res           []logs.Message
		More          bool
		Err           error
	}{
		{
			Name:          "NoLimit",
			ExpectedLimit: 100,
		},
		{
			Name:          "Limit",
			Limit:         42,
			ExpectedLimit: 42,
		},
		{
			Name:          "Results",
			ExpectedLimit: 100,
			Res: []logs.Message{
				{
					Timestamp: time.Now(),
					Message:   map[string]string{"msg1": testutil.NewUUIDStr(t)},
				},
				{
					Timestamp: time.Now(),
					Message:   map[string]string{"msg2": testutil.NewUUIDStr(t)},
				},
			},
			More: true,
		},
		{
			Name:          "Error",
			ExpectedLimit: 100,
			Err:           xerrors.New("test error"),
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			ctx, f := newLogsFixture(t)
			cid := testutil.NewUUIDStr(t)
			st := logs.ServiceTypeClickHouse

			f.ExpectBegin(ctx)
			f.ExpectClusterByClusterID(ctx, cid, clusters.TypeClickHouse)
			f.ExpectLogs(
				ctx, cid, logsdb.LogsServiceTypeToLogType(st)[0], input.ExpectedLimit, common.LogsOptions{},
				logsOutputs{
					Logs: input.Res,
					More: input.More,
					Err:  input.Err,
				},
			)
			msg, more, err := f.Logs.Logs(ctx, cid, st, input.Limit, common.LogsOptions{})
			if input.Err != nil {
				require.Error(t, err)
				require.True(t, xerrors.Is(err, input.Err))
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}
}

func TestLogs_Stream_Simple(t *testing.T) {
	ctx, f := newLogsFixture(t)
	cid := testutil.NewUUIDStr(t)
	lst := logs.ServiceTypeClickHouse
	limit := logic.DefaultLogsConfig().BatchSize
	opts := common.LogsOptions{
		FromTS: optional.NewTime(f.Clock.Now()),
		ToTS:   optional.NewTime(f.Clock.Now()),
	}

	out := []logs.Message{
		{
			Timestamp: time.Now(),
			Message:   map[string]string{"msg1": testutil.NewUUIDStr(t)},
		},
		{
			Timestamp: time.Now(),
			Message:   map[string]string{"msg2": testutil.NewUUIDStr(t)},
		},
	}
	f.ExpectBegin(ctx)
	f.ExpectClusterByClusterID(ctx, cid, clusters.TypeClickHouse)
	f.ExpectLogs(
		ctx, cid, logsdb.LogsServiceTypeToLogType(lst)[0], limit, opts,
		logsOutputs{
			Logs: out,
		},
	)

	ch, err := f.Logs.Stream(ctx, cid, lst, opts)
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

func TestLogs_Stream_Defaults(t *testing.T) {
	ctx, f := newLogsFixture(t)
	cid := testutil.NewUUIDStr(t)
	lst := logs.ServiceTypeClickHouse
	limit := logic.DefaultLogsConfig().BatchSize
	opts := common.LogsOptions{
		ToTS: optional.NewTime(f.Clock.Now()),
	}

	out := []logs.Message{
		{
			Timestamp:        time.Now(),
			Message:          map[string]string{"msg1": testutil.NewUUIDStr(t)},
			NextMessageToken: 1,
		},
		{
			Timestamp:        time.Now(),
			Message:          map[string]string{"msg2": testutil.NewUUIDStr(t)},
			NextMessageToken: 2,
		},
	}

	f.ExpectBegin(ctx)
	f.ExpectClusterByClusterID(ctx, cid, clusters.TypeClickHouse)

	// FromTS must be equal in both calls
	var firstFromTS *time.Time
	var offset int64
	f.LogsDB.EXPECT().Logs(ctx, cid, logsdb.LogsServiceTypeToLogType(lst)[0], opts.ColumnFilter, defaultFromTS(f.Clock), opts.ToTS.Time, limit, offset, opts.Filter).
		Return([]logs.Message{out[offset]}, true, nil).
		Do(func(_, _, _, _, arg4, _, _, _, _ interface{}) {
			fromTS := arg4.(time.Time)
			firstFromTS = &fromTS
			f.Clock.Advance(time.Second)
		})

	offset++
	f.LogsDB.EXPECT().Logs(ctx, cid, logsdb.LogsServiceTypeToLogType(lst)[0], opts.ColumnFilter, defaultFromTS(f.Clock), opts.ToTS.Time, limit, offset, opts.Filter).
		Return([]logs.Message{out[offset]}, false, nil).
		Do(func(_, _, _, _, arg4, _, _, _, _ interface{}) {
			fromTS := arg4.(time.Time)
			require.Equal(t, *firstFromTS, fromTS)
		})

	ch, err := f.Logs.Stream(ctx, cid, lst, opts)
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
	ctx, f := newLogsFixture(t)
	var cancel context.CancelFunc
	ctx, cancel = context.WithTimeout(ctx, time.Second*5)
	defer cancel()
	cid := testutil.NewUUIDStr(t)
	lst := logs.ServiceTypeClickHouse
	limit := logic.DefaultLogsConfig().BatchSize
	opts := common.LogsOptions{
		FromTS: optional.NewTime(f.Clock.Now()),
	}

	// Log messages we will be receiving
	out := []logs.Message{
		{
			Timestamp:        time.Now(),
			Message:          map[string]string{"msg1": testutil.NewUUIDStr(t)},
			NextMessageToken: 1,
		},
		{
			Timestamp:        time.Now(),
			Message:          map[string]string{"msg2": testutil.NewUUIDStr(t)},
			NextMessageToken: 2,
		},
		{
			Timestamp:        time.Now(),
			Message:          map[string]string{"msg3": testutil.NewUUIDStr(t)},
			NextMessageToken: 3,
		},
	}
	f.ExpectBegin(ctx)

	lts := logsdb.LogsServiceTypeToLogType(lst)
	require.Len(t, lts, 1)
	lt := lts[0]

	f.ExpectClusterByClusterID(ctx, cid, clusters.TypeClickHouse)

	// Received first message
	var offset int64
	call := f.LogsDB.EXPECT().Logs(ctx, cid, lt, opts.ColumnFilter, opts.FromTS.Time, gomock.Any(), limit, offset, opts.Filter).
		Return([]logs.Message{out[offset]}, true, nil).
		Do(func(_, _, _, _, _, _, _, _, _ interface{}) { f.Clock.Advance(time.Second) })

	// Received second message
	offset++
	call = f.LogsDB.EXPECT().Logs(ctx, cid, lt, opts.ColumnFilter, opts.FromTS.Time, gomock.Any(), limit, offset, opts.Filter).
		Return([]logs.Message{out[offset]}, true, nil).
		Do(func(_, _, _, _, _, _, _, _, _ interface{}) { f.Clock.Advance(time.Second) }).
		After(call)

	// No messages in database
	offset++
	call = f.LogsDB.EXPECT().Logs(ctx, cid, lt, opts.ColumnFilter, opts.FromTS.Time, gomock.Any(), limit, offset, opts.Filter).
		Return(nil, true, nil).
		Do(func(_, _, _, _, _, _, _, _, _ interface{}) { f.Clock.Advance(time.Second) }).
		After(call)

	// Received third message
	call = f.LogsDB.EXPECT().Logs(ctx, cid, lt, opts.ColumnFilter, opts.FromTS.Time, gomock.Any(), limit, offset, opts.Filter).
		Return([]logs.Message{out[offset]}, true, nil).
		Do(func(_, _, _, _, _, _, _, _, _ interface{}) { f.Clock.Advance(time.Second) }).
		After(call)

	// No messages in database
	offset++
	call = f.LogsDB.EXPECT().Logs(ctx, cid, lt, opts.ColumnFilter, opts.FromTS.Time, gomock.Any(), limit, offset, opts.Filter).
		Return(nil, true, nil).
		AnyTimes().
		After(call) // We cannot expect this method to be called only once because its racy
	// If context is canceled, replace returned error with the one in context
	call.Do(func(arg0, _, _, _, _, _, _, _, _ interface{}) {
		ctx := arg0.(context.Context)
		if ctx.Err() != nil {
			call.Return(nil, true, ctx.Err())
		}
	})

	// Lets stream!
	ch, err := f.Logs.Stream(ctx, cid, lst, opts)
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
