package provider

import (
	"context"
	"testing"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
	perfdiagdbmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb/mocks"
)

type sessionsStatsFixture struct {
	*testhelpers.Fixture
	PerfDiagDB *perfdiagdbmock.MockBackend
	PerfDiag   *PerfDiag
	Clock      clockwork.FakeClock
}

type SessionsStatsOutputs struct {
	SessionsStats []mymodels.SessionsStats
	More          bool
	Err           error
}

func (f sessionsStatsFixture) ExpectSessionsStats(ctx context.Context, cid string, limit int64, opts mymodels.GetSessionsStatsOptions, outputs SessionsStatsOutputs) {
	if !opts.FromTS.Valid {
		opts.FromTS.Time = defaultFromTS(f.Clock)
	}

	if !opts.ToTS.Valid {
		opts.ToTS.Time = defaultToTS(f.Clock)
	}

	if len(opts.GroupBy) == 0 {
		opts.GroupBy = []mymodels.MySessionsColumn{mymodels.MySessionsStage, mymodels.MySessionsUser}
	}

	if opts.RollupPeriod.Int64 < 1 {
		opts.RollupPeriod.Int64 = 1
	}

	f.PerfDiagDB.EXPECT().SessionsStats(ctx, cid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.RollupPeriod.Int64, opts.GroupBy, opts.OrderBy, opts.Filter).
		Return(outputs.SessionsStats, outputs.More, outputs.Err).
		Do(func(_, _, _, _, _, _, _, _, _, _ interface{}) { f.Clock.Advance(time.Second) })
}

func newSessionsStatsFixture(t *testing.T) (context.Context, sessionsStatsFixture) {
	ctx, f := testhelpers.NewFixture(t)
	PerfDiagDB := perfdiagdbmock.NewMockBackend(f.Ctrl)
	cfg := logic.DefaultConfig()
	clock := clockwork.NewFakeClock()
	l := NewPerfDiag(cfg, f.Sessions, PerfDiagDB, clock)
	return ctx, sessionsStatsFixture{Fixture: f, PerfDiagDB: PerfDiagDB, PerfDiag: l, Clock: clock}
}

func TestSessionsStats_Defaults(t *testing.T) {
	ctx, f := newSessionsStatsFixture(t)
	cid := testutil.NewUUIDStr(t)
	limit := int64(42)
	opts := mymodels.GetSessionsStatsOptions{}
	out := SessionsStatsOutputs{
		SessionsStats: []mymodels.SessionsStats{
			{
				Timestamp:        optional.Time{Valid: true, Time: time.Now()},
				Dimensions:       map[string]string{"wait_event": testutil.NewUUIDStr(t)},
				SessionsCount:    1488,
				NextMessageToken: 83,
			},
			{
				Timestamp:        optional.Time{Valid: true, Time: time.Now()},
				Dimensions:       map[string]string{"datname": testutil.NewUUIDStr(t)},
				SessionsCount:    228,
				NextMessageToken: 84,
			},
		},
		More: true,
	}
	f.ExpectBegin(ctx)
	f.ExpectSessionsStats(ctx, cid, limit, opts, out)

	msg, more, err := f.PerfDiag.GetSessionsStats(ctx, cid, limit, mymodels.GetSessionsStatsOptions{})
	require.NoError(t, err)
	require.Equal(t, out.More, more)
	require.Equal(t, msg, out.SessionsStats)
}
