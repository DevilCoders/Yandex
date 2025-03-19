package integration

import (
	"context"
	"database/sql"
	"os"
	"runtime"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/perfdiagdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestClickhouseQueries(t *testing.T) {
	chPort, ok := os.LookupEnv("RECIPE_CLICKHOUSE_NATIVE_PORT")
	if !ok {
		if runtime.GOOS == "darwin" {
			t.SkipNow()
		}
		t.Fatal("RECIPE_CLICKHOUSE_NATIVE_PORT unset")
	}
	сtx := context.Background()
	cfg := clickhouse.DefaultConfig()
	cfg.User = "default"
	cfg.Secure = false
	cfg.Addrs = []string{"localhost:" + chPort}
	l, _ := zap.New(zap.KVConfig(log.DebugLevel))
	err := initDB(cfg)
	require.NoError(t, err, "database initialization failed")
	cfg.DB = "perf_diag"
	back, err := clickhouse.New(cfg, l)

	require.NoError(t, err, "backed initialization failed")

	err = back.IsReady(сtx)
	require.NoError(t, err, "backend not ready")

	sessionsStatsInputs := []struct {
		Name         string
		Filter       string
		Limit        int64
		RollupPeriod int64
		GroupBy      []pgmodels.PGStatActivityColumn
		OrderBy      []pgmodels.OrderBy
		Res          []pgmodels.SessionsStats
		More         bool
		Err          error
	}{
		{Name: "Filter",
			Filter: "stats.user='user1'", Limit: 1, RollupPeriod: 60, GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase,
				pgmodels.PGStatActivityTime, pgmodels.PGStatActivityWaitEvent}, OrderBy: []pgmodels.OrderBy{{Field: pgmodels.PGStatActivityTime, SortOrder: pgmodels.OrderByAsc},
				{Field: pgmodels.PGStatActivityWaitEvent, SortOrder: pgmodels.OrderByAsc}}, Res: []pgmodels.SessionsStats{
				{
					Timestamp:        optional.Time{Valid: true, Time: time.Unix(1595688780, 0)},
					Dimensions:       map[string]string{"database": "business", "wait_event": "CPU"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			}, More: true},
		{Name: "FilterIn",
			Filter: "stats.user in ('user1')", Limit: 1, RollupPeriod: 60, GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase,
				pgmodels.PGStatActivityTime, pgmodels.PGStatActivityWaitEvent}, OrderBy: []pgmodels.OrderBy{{Field: pgmodels.PGStatActivityTime, SortOrder: pgmodels.OrderByAsc},
				{Field: pgmodels.PGStatActivityWaitEvent, SortOrder: pgmodels.OrderByAsc}}, Res: []pgmodels.SessionsStats{
				{
					Timestamp:        optional.Time{Valid: true, Time: time.Unix(1595688780, 0)},
					Dimensions:       map[string]string{"database": "business", "wait_event": "CPU"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			}, More: true},
		{Name: "FilterNotIn",
			Filter: "stats.user NOT IN ('user2', 'user3')", Limit: 1, RollupPeriod: 60, GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase,
				pgmodels.PGStatActivityTime, pgmodels.PGStatActivityWaitEvent}, OrderBy: []pgmodels.OrderBy{{Field: pgmodels.PGStatActivityTime, SortOrder: pgmodels.OrderByAsc},
				{Field: pgmodels.PGStatActivityWaitEvent, SortOrder: pgmodels.OrderByAsc}}, Res: []pgmodels.SessionsStats{
				{
					Timestamp:        optional.Time{Valid: true, Time: time.Unix(1595688780, 0)},
					Dimensions:       map[string]string{"database": "business", "wait_event": "CPU"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			}, More: true},
		{Name: "FilterNotEqual",
			Filter: "stats.user!='user2'", Limit: 1, RollupPeriod: 60, GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase,
				pgmodels.PGStatActivityTime, pgmodels.PGStatActivityWaitEvent}, OrderBy: []pgmodels.OrderBy{{Field: pgmodels.PGStatActivityTime, SortOrder: pgmodels.OrderByAsc},
				{Field: pgmodels.PGStatActivityWaitEvent, SortOrder: pgmodels.OrderByAsc}}, Res: []pgmodels.SessionsStats{
				{
					Timestamp:        optional.Time{Valid: true, Time: time.Unix(1595688780, 0)},
					Dimensions:       map[string]string{"database": "business", "wait_event": "CPU"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			}, More: true},
		{Name: "Order",
			Filter: "stats.user='user1'", Limit: 1, RollupPeriod: 60, GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase,
				pgmodels.PGStatActivityTime, pgmodels.PGStatActivityWaitEvent}, OrderBy: []pgmodels.OrderBy{{Field: pgmodels.PGStatActivityTime, SortOrder: pgmodels.OrderByDesc},
				{Field: pgmodels.PGStatActivityWaitEvent, SortOrder: pgmodels.OrderByDesc}}, Res: []pgmodels.SessionsStats{
				{
					Timestamp:        optional.Time{Valid: true, Time: time.Unix(1595688840, 0)},
					Dimensions:       map[string]string{"database": "business", "wait_event": "SyncRep"},
					SessionsCount:    10,
					NextMessageToken: 1,
				},
			}, More: true},
		{Name: "NoFilter",
			Limit: 1, RollupPeriod: 60, GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase,
				pgmodels.PGStatActivityTime, pgmodels.PGStatActivityWaitEvent}, OrderBy: []pgmodels.OrderBy{{Field: pgmodels.PGStatActivityTime, SortOrder: pgmodels.OrderByAsc},
				{Field: pgmodels.PGStatActivityWaitEvent, SortOrder: pgmodels.OrderByAsc}}, Res: []pgmodels.SessionsStats{
				{
					Timestamp:        optional.Time{Valid: true, Time: time.Unix(1595688780, 0)},
					Dimensions:       map[string]string{"database": "business", "wait_event": "CPU"},
					SessionsCount:    52,
					NextMessageToken: 1,
				},
			}, More: true},
		{Name: "NoRollup",
			Limit: 1, RollupPeriod: 1, GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase,
				pgmodels.PGStatActivityTime, pgmodels.PGStatActivityWaitEvent}, OrderBy: []pgmodels.OrderBy{{Field: pgmodels.PGStatActivityTime, SortOrder: pgmodels.OrderByAsc},
				{Field: pgmodels.PGStatActivityWaitEvent, SortOrder: pgmodels.OrderByAsc}}, Res: []pgmodels.SessionsStats{
				{
					Timestamp: optional.Time{Valid: true, Time: time.Unix(1595688814, 0)},

					Dimensions:       map[string]string{"database": "business", "wait_event": "CPU"},
					SessionsCount:    2,
					NextMessageToken: 1,
				},
			}, More: true},
		{Name: "NoPGStatActivityTime",
			Limit: 1, RollupPeriod: 1,
			GroupBy: []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityDatabase, pgmodels.PGStatActivityWaitEvent},
			Res: []pgmodels.SessionsStats{
				{
					Dimensions:       map[string]string{"database": "business", "wait_event": "CPU"},
					SessionsCount:    72,
					NextMessageToken: 1,
				},
			}, More: true},
	}
	// SessionStats runs
	for _, input := range sessionsStatsInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.SessionsStats(сtx, "cid1", input.Limit, 0, time.Unix(1595688814, 0),
				time.Unix(1595688874, 0), input.RollupPeriod, input.GroupBy, input.OrderBy, f)
			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}

	sessionsAtTimeInputs := []struct {
		Name         string
		Filter       string
		Limit        int64
		OrderBy      []pgmodels.OrderBySessionsAtTime
		ColumnFilter []pgmodels.PGStatActivityColumn
		Time         time.Time
		Res          pgmodels.SessionsAtTime
		More         bool
		Err          error
	}{
		{
			Name:         "sessionsAtTimeNoExistsDateLeft",
			Limit:        1,
			ColumnFilter: []pgmodels.PGStatActivityColumn{"wait_event", "user", "collect_time"},
			OrderBy: []pgmodels.OrderBySessionsAtTime{
				{
					Field:     pgmodels.PGStatActivityTime,
					SortOrder: pgmodels.OrderByAsc,
				},
			},
			Time: time.Unix(0, 0),
			Res: pgmodels.SessionsAtTime{
				Sessions: []pgmodels.SessionState{
					{
						Timestamp: time.Unix(1595688814, 0),
						User:      "user1",
						WaitEvent: "CPU"},
				},
				PreviousCollectTime: time.Unix(1595688814, 0),
				NextCollectTime:     time.Unix(1595688815, 0),
				NextMessageToken:    1,
			},
			More: true,
		},
		{
			Name:         "sessionsAtTimeNoExistsDateRight",
			Limit:        1,
			ColumnFilter: []pgmodels.PGStatActivityColumn{"wait_event", "user", "collect_time"},
			OrderBy: []pgmodels.OrderBySessionsAtTime{
				{
					Field:     pgmodels.PGStatActivityTime,
					SortOrder: pgmodels.OrderByAsc,
				},
			},
			Time: time.Unix(1888888888, 0),
			Res: pgmodels.SessionsAtTime{
				Sessions: []pgmodels.SessionState{
					{
						Timestamp: time.Unix(1595688849, 0),
						User:      "user1",
						WaitEvent: "CPU"},
				},
				PreviousCollectTime: time.Unix(1595688848, 0),
				NextCollectTime:     time.Unix(1595688849, 0),
				NextMessageToken:    1,
			},
			More: true,
		},
		{
			Name:         "sessionsAtTimeExistsDate",
			Limit:        2,
			Filter:       "stats.user!='user1'",
			ColumnFilter: []pgmodels.PGStatActivityColumn{"wait_event", "user", "collect_time"},
			OrderBy: []pgmodels.OrderBySessionsAtTime{
				{
					Field:     pgmodels.PGStatActivityWaitEvent,
					SortOrder: pgmodels.OrderByAsc,
				},
			},
			Time: time.Unix(1595688840, 0),
			Res: pgmodels.SessionsAtTime{
				Sessions: []pgmodels.SessionState{
					{
						Timestamp: time.Unix(1595688840, 0),
						User:      "user2",
						WaitEvent: "CPU",
					},
					{
						Timestamp: time.Unix(1595688840, 0),
						User:      "user2",
						WaitEvent: "SyncRep",
					},
				},

				PreviousCollectTime: time.Unix(1595688839, 0),
				NextCollectTime:     time.Unix(1595688841, 0),
				NextMessageToken:    2,
			},
			More: false,
		},
	}

	// SessionsAtTime runs
	for _, input := range sessionsAtTimeInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.SessionsAtTime(сtx, "cid1", input.Limit, 0, input.Time, input.ColumnFilter, f, input.OrderBy)
			// sqlx put some timezone info in time struct
			for i, f := range msg.Sessions {
				msg.Sessions[i].Timestamp = time.Unix(f.Timestamp.Unix(), 0)
			}
			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}

	statementsDiffInputs := []struct {
		Name           string
		Filter         string
		Limit          int64
		OrderBy        []pgmodels.OrderByStatementsAtTime
		ColumnFilter   []pgmodels.PGStatStatementsColumn
		FirstInterval  time.Time
		SecondInterval time.Time
		Duration       int64
		Res            pgmodels.DiffStatements
		More           bool
		Err            error
	}{
		{
			Name:         "StatementsDiff",
			Limit:        1,
			Duration:     5,
			ColumnFilter: []pgmodels.PGStatStatementsColumn{"query", "collect_time", "user", "queryid", "calls"},
			OrderBy: []pgmodels.OrderByStatementsAtTime{
				{
					Field:     pgmodels.PGStatStatementsUser,
					SortOrder: pgmodels.OrderByAsc,
				},
			},
			FirstInterval:  time.Unix(1595688814, 0),
			SecondInterval: time.Unix(1595688819, 0),
			Filter:         "stats.queryid='1'",

			Res: pgmodels.DiffStatements{
				DiffStatements: []pgmodels.DiffStatement{
					{
						FirstIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(1595688814, 0), Valid: true},
							EndTimestamp:   optional.Time{Time: time.Unix(1595688819, 0), Valid: true},
						},
						SecondIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(1595688819, 0), Valid: true},
							EndTimestamp:   optional.Time{Time: time.Unix(1595688824, 0), Valid: true},
						},
						User:        "user1",
						Queryid:     "1",
						Query:       "SELECT abalance FROM pgbench_accounts WHERE aid = $1",
						FirstCalls:  246,
						SecondCalls: 726,
						DiffCalls:   195.121,
					},
				},
				NextMessageToken: 1,
			},
			More: true,
		},
		{
			Name:         "StatementsDiffFirstIntervalNotExists",
			Limit:        1,
			Duration:     5,
			ColumnFilter: []pgmodels.PGStatStatementsColumn{"query", "collect_time", "user", "queryid", "calls"},
			OrderBy: []pgmodels.OrderByStatementsAtTime{
				{
					Field:     pgmodels.PGStatStatementsQueryid,
					SortOrder: pgmodels.OrderByAsc,
				},
				{
					Field:     pgmodels.PGStatStatementsUser,
					SortOrder: pgmodels.OrderByAsc,
				},
			},
			FirstInterval:  time.Unix(0, 0),
			SecondInterval: time.Unix(1595688819, 0),
			Filter:         "stats.queryid='1'",

			Res: pgmodels.DiffStatements{
				DiffStatements: []pgmodels.DiffStatement{
					{
						FirstIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
							EndTimestamp:   optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
						},
						SecondIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(1595688819, 0), Valid: true},
							EndTimestamp:   optional.Time{Time: time.Unix(1595688824, 0), Valid: true},
						},
						User:        "user1",
						Queryid:     "1",
						Query:       "SELECT abalance FROM pgbench_accounts WHERE aid = $1",
						SecondCalls: 726,
						DiffCalls:   0,
					},
				},
				NextMessageToken: 1,
			},
			More: true,
		},
		{
			Name:         "StatementsDiffSecondIntervalNotExists",
			Limit:        1,
			Duration:     5,
			ColumnFilter: []pgmodels.PGStatStatementsColumn{"query", "collect_time", "user", "queryid", "calls"},
			OrderBy: []pgmodels.OrderByStatementsAtTime{
				{
					Field:     pgmodels.PGStatStatementsQueryid,
					SortOrder: pgmodels.OrderByAsc,
				},
				{
					Field:     pgmodels.PGStatStatementsUser,
					SortOrder: pgmodels.OrderByAsc,
				},
			},
			SecondInterval: time.Unix(0, 0),
			FirstInterval:  time.Unix(1595688819, 0),
			Filter:         "stats.queryid='1'",

			Res: pgmodels.DiffStatements{
				DiffStatements: []pgmodels.DiffStatement{
					{
						FirstIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(1595688819, 0), Valid: true},
							EndTimestamp:   optional.Time{Time: time.Unix(1595688824, 0), Valid: true},
						},
						SecondIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
							EndTimestamp:   optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
						},
						User:       "user1",
						Queryid:    "1",
						Query:      "SELECT abalance FROM pgbench_accounts WHERE aid = $1",
						FirstCalls: 726,
						DiffCalls:  -100,
					},
				},
				NextMessageToken: 1,
			},
			More: true,
		},
		{
			Name:         "StatementsDiffSecondSameInterval",
			Limit:        1,
			Duration:     5,
			ColumnFilter: []pgmodels.PGStatStatementsColumn{"query", "collect_time", "user", "queryid", "calls"},
			OrderBy: []pgmodels.OrderByStatementsAtTime{
				{
					Field:     pgmodels.PGStatStatementsQueryid,
					SortOrder: pgmodels.OrderByAsc,
				},
				{
					Field:     pgmodels.PGStatStatementsUser,
					SortOrder: pgmodels.OrderByAsc,
				},
			},
			SecondInterval: time.Unix(1595688819, 0),
			FirstInterval:  time.Unix(1595688819, 0),
			Filter:         "stats.queryid='1'",

			Res: pgmodels.DiffStatements{
				DiffStatements: []pgmodels.DiffStatement{
					{
						FirstIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(1595688819, 0), Valid: true},
							EndTimestamp:   optional.Time{Time: time.Unix(1595688824, 0), Valid: true},
						},
						SecondIntervalTime: pgmodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(1595688819, 0), Valid: true},
							EndTimestamp:   optional.Time{Time: time.Unix(1595688824, 0), Valid: true},
						},
						User:        "user1",
						Queryid:     "1",
						Query:       "SELECT abalance FROM pgbench_accounts WHERE aid = $1",
						FirstCalls:  726,
						SecondCalls: 726,
						DiffCalls:   0,
					},
				},
				NextMessageToken: 1,
			},
			More: true,
		},
	}

	// StatementsDiff runs
	for _, input := range statementsDiffInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.StatementsDiff(сtx, "cid1", input.Limit, 0, input.FirstInterval, input.SecondInterval, input.Duration, input.ColumnFilter, f, input.OrderBy)
			// sqlx put some timezone info in time struct
			for i, f := range msg.DiffStatements {
				msg.DiffStatements[i].FirstIntervalTime.StartTimestamp.Time = time.Unix(f.FirstIntervalTime.StartTimestamp.Time.Unix(), 0)
				msg.DiffStatements[i].FirstIntervalTime.EndTimestamp.Time = time.Unix(f.FirstIntervalTime.EndTimestamp.Time.Unix(), 0)
				msg.DiffStatements[i].SecondIntervalTime.StartTimestamp.Time = time.Unix(f.SecondIntervalTime.StartTimestamp.Time.Unix(), 0)
				msg.DiffStatements[i].SecondIntervalTime.EndTimestamp.Time = time.Unix(f.SecondIntervalTime.EndTimestamp.Time.Unix(), 0)
			}
			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}

	statementsIntervalInputs := []struct {
		Name         string
		Filter       string
		Limit        int64
		OrderBy      []pgmodels.OrderByStatementsAtTime
		ColumnFilter []pgmodels.PGStatStatementsColumn
		FromTS, ToTS time.Time
		Res          pgmodels.StatementsInterval
		More         bool
		Err          error
	}{
		{
			Name:         "NonExistsDate",
			Limit:        1,
			ColumnFilter: []pgmodels.PGStatStatementsColumn{},
			OrderBy:      []pgmodels.OrderByStatementsAtTime{},
			FromTS:       time.Unix(0, 0),
			ToTS:         time.Unix(0, 0),
			Res: pgmodels.StatementsInterval{
				Statements:       nil,
				NextMessageToken: 0,
			},
			More: false,
		},
		{
			Name:         "ExistsDate",
			Limit:        1,
			ColumnFilter: []pgmodels.PGStatStatementsColumn{},
			OrderBy:      []pgmodels.OrderByStatementsAtTime{},
			FromTS:       time.Unix(1495688819, 0),
			ToTS:         time.Unix(1695688819, 0),
			Res: pgmodels.StatementsInterval{
				Statements: []pgmodels.Statements{{
					Timestamp:         time.Unix(1595688849, 0),
					Host:              "host1",
					User:              "user1",
					Database:          "business",
					Queryid:           "3",
					Query:             "UPDATE pgbench_tellers SET tbalance = tbalance + $1 WHERE tid = $2",
					Calls:             10188,
					TotalTime:         10191.6,
					MinTime:           3.1,
					MaxTime:           563.1,
					MeanTime:          283.1,
					StddevTime:        283.1,
					Rows:              10188,
					SharedBlksHit:     10188,
					SharedBlksRead:    10188,
					SharedBlksDirtied: 10188,
					SharedBlksWritten: 10188,
					BlkReadTime:       10191.6,
					BlkWriteTime:      10191.6,
					TempBlksRead:      10188,
					TempBlksWritten:   10188,
					Reads:             10188,
					Writes:            10188,
					UserTime:          10191.6,
					SystemTime:        10191.6,
				},
				},
				NextMessageToken: 1,
			},
			More: true,
		},
	}

	// StatementsInterval runs
	for _, input := range statementsIntervalInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.StatementsInterval(сtx, "cid1", input.Limit, 0, input.FromTS, input.ToTS, input.ColumnFilter, f, input.OrderBy)
			// sqlx put some timezone info in time struct
			for i, f := range msg.Statements {
				msg.Statements[i].Timestamp = time.Unix(f.Timestamp.Unix(), 0)
			}
			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}

	statementsStatsInputs := []struct {
		Name         string
		Filter       string
		Limit        int64
		ColumnFilter []pgmodels.StatementsStatsField
		FromTS, ToTS time.Time
		Res          pgmodels.StatementsStats
		GroupBy      []pgmodels.StatementsStatsGroupBy
		More         bool
		Err          error
	}{
		{
			Name:         "NonExistsDate",
			Limit:        1,
			ColumnFilter: []pgmodels.StatementsStatsField{},
			FromTS:       time.Unix(0, 0),
			ToTS:         time.Unix(0, 0),
			Filter:       "stats.queryid='1'",
			GroupBy:      []pgmodels.StatementsStatsGroupBy{pgmodels.StatementsStatsGroupByUser},
			Res: pgmodels.StatementsStats{
				Statements:       nil,
				NextMessageToken: 0,
				Query:            "SELECT abalance FROM pgbench_accounts WHERE aid = $1",
			},
			More: false,
		},
		{
			Name:         "ExistsDate",
			Limit:        1,
			ColumnFilter: []pgmodels.StatementsStatsField{},
			FromTS:       time.Unix(1495688819, 0),
			ToTS:         time.Unix(1695688819, 0),
			GroupBy:      []pgmodels.StatementsStatsGroupBy{pgmodels.StatementsStatsGroupByUser},
			Filter:       "stats.queryid='3'",
			Res: pgmodels.StatementsStats{
				Statements: []pgmodels.Statements{{
					Timestamp:         time.Unix(1595688810, 0),
					Host:              "",
					User:              "user2",
					Database:          "",
					Queryid:           "",
					Query:             "",
					Calls:             51,
					TotalTime:         51.1,
					MinTime:           51.1,
					MaxTime:           51.1,
					MeanTime:          51.1,
					StddevTime:        51.1,
					Rows:              51,
					SharedBlksHit:     51,
					SharedBlksRead:    51,
					SharedBlksDirtied: 51,
					SharedBlksWritten: 51,
					BlkReadTime:       51.1,
					BlkWriteTime:      51.1,
					TempBlksRead:      51,
					TempBlksWritten:   51,
					Reads:             51,
					Writes:            51,
					UserTime:          51.1,
					SystemTime:        51.1,
				},
				},
				NextMessageToken: 1,
				Query:            "UPDATE pgbench_tellers SET tbalance = tbalance + $1 WHERE tid = $2",
			},
			More: true,
		},
	}

	// StatementsStats runs
	for _, input := range statementsStatsInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.StatementsStats(сtx, "cid1", input.Limit, 0, input.FromTS, input.ToTS, input.GroupBy, f, input.ColumnFilter)
			// sqlx put some timezone info in time struct
			for i, f := range msg.Statements {
				msg.Statements[i].Timestamp = time.Unix(f.Timestamp.Unix(), 0)
			}
			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}

	statementStatsInputs := []struct {
		Name         string
		Queryid      string
		Filter       string
		Limit        int64
		ColumnFilter []pgmodels.StatementsStatsField
		GroupBy      []pgmodels.StatementsStatsGroupBy
		FromTS, ToTS time.Time
		Res          pgmodels.StatementStats
		More         bool
		Err          error
	}{
		{
			Name:         "NonExistsDate",
			Queryid:      "1",
			Limit:        1,
			ColumnFilter: []pgmodels.StatementsStatsField{},
			FromTS:       time.Unix(0, 0),
			ToTS:         time.Unix(0, 0),
			GroupBy:      []pgmodels.StatementsStatsGroupBy{pgmodels.StatementsStatsGroupByUser},
			Res: pgmodels.StatementStats{
				Statements:       nil,
				NextMessageToken: 0,
				Query:            "SELECT abalance FROM pgbench_accounts WHERE aid = $1",
			},
			More: false,
		},
		{
			Name:         "ExistsDate",
			Queryid:      "3",
			Limit:        1,
			ColumnFilter: []pgmodels.StatementsStatsField{},
			FromTS:       time.Unix(1495688819, 0),
			ToTS:         time.Unix(1695688819, 0),
			GroupBy:      []pgmodels.StatementsStatsGroupBy{pgmodels.StatementsStatsGroupByUser},
			Res: pgmodels.StatementStats{
				Statements: []pgmodels.Statements{{
					Timestamp:         time.Unix(1595688810, 0),
					Host:              "",
					User:              "user1",
					Database:          "",
					Queryid:           "3",
					Query:             "",
					Calls:             43,
					TotalTime:         43.1,
					MinTime:           43.1,
					MaxTime:           43.1,
					MeanTime:          43.1,
					StddevTime:        43.1,
					Rows:              43,
					SharedBlksHit:     43,
					SharedBlksRead:    43,
					SharedBlksDirtied: 43,
					SharedBlksWritten: 43,
					BlkReadTime:       43.1,
					BlkWriteTime:      43.1,
					TempBlksRead:      43,
					TempBlksWritten:   43,
					Reads:             43,
					Writes:            43,
					UserTime:          43.1,
					SystemTime:        43.1,
				},
				},
				NextMessageToken: 1,
				Query:            "UPDATE pgbench_tellers SET tbalance = tbalance + $1 WHERE tid = $2",
			},
			More: true,
		},
	}

	// StatementsStats runs
	for _, input := range statementStatsInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.StatementStats(сtx, "cid1", input.Queryid, input.Limit, 0, input.FromTS, input.ToTS, input.GroupBy, f, input.ColumnFilter)
			// sqlx put some timezone info in time struct
			for i, f := range msg.Statements {
				msg.Statements[i].Timestamp = time.Unix(f.Timestamp.Unix(), 0)
			}
			require.NoError(t, err)
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}
}

func initDB(cfg chutil.Config) error {
	cfg.DB = "default"
	db, err := sql.Open("clickhouse", cfg.URI())
	if err != nil {
		return err
	}
	err = db.Ping()
	if err != nil {
		return err
	}
	_, err = db.Exec("DROP DATABASE IF EXISTS perf_diag;")
	if err != nil {
		return err
	}
	_, err = db.Exec("CREATE DATABASE perf_diag;")
	if err != nil {
		return err
	}

	createQueries := map[string]string{
		"pg_stat_activity": `
CREATE TABLE perf_diag.pg_stat_activity (_timestamp DateTime,
 _partition String,
 _offset UInt64,
 _idx UInt32,
 collect_time Nullable(DateTime),
 cluster_id Nullable(String),
 cluster_name Nullable(String),
 host Nullable(String),
 database Nullable(String),
 pid Nullable(UInt32),
 user Nullable(String),
 application_name Nullable(String),
 client_addr Nullable(String),
 client_hostname Nullable(String),
 client_port Nullable(UInt32),
 backend_start Nullable(DateTime),
 xact_start Nullable(DateTime),
 query_start Nullable(DateTime),
 state_change Nullable(DateTime),
 wait_event_type Nullable(String),
 wait_event Nullable(String),
 state Nullable(String),
 backend_xid Nullable(UInt32),
 backend_xmin Nullable(UInt32),
 query Nullable(String),
 backend_type Nullable(String),
 blocking_pids Nullable(String),
 queryid Nullable(String),
 _rest Nullable(String)) ENGINE = Memory;`,
		"pg_stat_statements": `
CREATE TABLE perf_diag.pg_stat_statements (_timestamp DateTime,
 _partition String,
 _offset UInt64,
 _idx UInt32,
 collect_time Nullable(DateTime),
 cluster_id Nullable(String),
 host Nullable(String),
 user Nullable(String),
 database Nullable(String),
 queryid Nullable(String),
 query Nullable(String),
 calls Nullable(UInt64),
 total_time Nullable(Float64),
 min_time Nullable(Float64),
 max_time Nullable(Float64),
 mean_time Nullable(Float64),
 stddev_time Nullable(Float64),
 rows Nullable(UInt64),
 shared_blks_hit Nullable(UInt64),
 shared_blks_read Nullable(UInt64),
 shared_blks_dirtied Nullable(UInt64),
 shared_blks_written Nullable(UInt64),
 local_blks_hit Nullable(UInt64),
 local_blks_read Nullable(UInt64),
 local_blks_dirtied Nullable(UInt64),
 local_blks_written Nullable(UInt64),
 temp_blks_read Nullable(UInt64),
 temp_blks_written Nullable(UInt64),
 blk_read_time Nullable(Float64),
 blk_write_time Nullable(Float64),
 reads Nullable(UInt64),
 writes Nullable(UInt64),
 user_time Nullable(Float64),
 system_time Nullable(Float64),
 _rest Nullable(String)) ENGINE = Memory;`,
	}
	insertQueries := map[string]string{
		"pg_stat_activity": `INSERT INTO perf_diag.pg_stat_activity
(_timestamp, _partition, _offset, _idx, collect_time, cluster_id, cluster_name, host,
database, pid, user, application_name, client_addr, client_hostname, client_port,
backend_start, xact_start, query_start, state_change, wait_event_type, wait_event,
state, backend_xid, backend_xmin, query, backend_type, blocking_pids, queryid, _rest)
VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)`,
		"pg_stat_statements": `INSERT INTO perf_diag.pg_stat_statements
(_timestamp, _partition, _offset, _idx, collect_time, cluster_id, host, user, database, queryid, calls,
total_time, min_time, max_time, mean_time, stddev_time, rows, shared_blks_hit, shared_blks_read,
shared_blks_dirtied, shared_blks_written, local_blks_hit, local_blks_read, local_blks_dirtied,
local_blks_written, temp_blks_read, temp_blks_written, blk_read_time, blk_write_time, reads, writes, user_time,
system_time, query, _rest)
VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)`,
	}

	for _, query := range createQueries {
		_, err = db.Exec(query)
		if err != nil {
			return err
		}
	}

	data := make(map[string][][]interface{})

	data["pg_stat_activity"] = genDataPGStatActivity()
	data["pg_stat_statements"] = genDataPGStatStatements()
	for table := range insertQueries {
		tx, _ := db.Begin()
		stmt, _ := tx.Prepare(insertQueries[table])
		for _, row := range data[table] {
			_, err = stmt.Exec(row...)
		}
		if err != nil {
			return err
		}

		if err = tx.Commit(); err != nil {
			return err
		}
	}
	return nil
}

func genDataPGStatActivity() [][]interface{} {
	PGStatActivityTestData := make([][]interface{}, 1)

	var (
		users      = []string{"user1", "user2"}
		cids       = []string{"cid1", "cid2"}
		clUser     = "cluster"
		dbname     = "business"
		host       = "host1"
		waitEvents = []string{"CPU", "SyncRep"}
	)

	ts := time.Unix(1595688814, 0)
	endTS := time.Unix(1595688850, 0)
	for ; ts.Unix() < endTS.Unix(); ts = ts.Add(1e9) {
		for _, user := range users {
			for _, cid := range cids {
				for _, waitEvent := range waitEvents {
					row := []interface{}{ts, "lb_part", 14, 1, ts, cid,
						clUser, host, dbname, 88, user, "app", "addr", "client_hostname", 228,
						ts, ts, ts,
						ts, "wait_event_type", waitEvent, "active", 14, 2, "SELECT 48",
						"client backend", "[1, 2, 3]", "1337", "{}"}
					PGStatActivityTestData = append(PGStatActivityTestData, row)
				}
			}
		}
	}
	return PGStatActivityTestData
}
func genDataPGStatStatements() [][]interface{} {

	PGStatStatementsTestData := make([][]interface{}, 1)

	var (
		users    = []string{"user1", "user2"}
		cids     = []string{"cid1", "cid2"}
		dbname   = "business"
		host     = "host1"
		queryIds = []string{"1", "2", "3", "4"}
		queries  = map[string]string{
			"1": "SELECT abalance FROM pgbench_accounts WHERE aid = $1",
			"2": "INSERT INTO pgbench_history (tid, bid, aid, delta, mtime) VALUES ($1, $2, $3, $4, CURRENT_TIMESTAMP)",
			"3": "UPDATE pgbench_tellers SET tbalance = tbalance + $1 WHERE tid = $2",
			"4": "UPDATE pgbench_branches SET bbalance = bbalance + $1 WHERE bid = $2",
		}
	)

	ts := time.Unix(1595688814, 0)
	endTS := time.Unix(1595688850, 0)
	i := 1
	f := 1.1
	for ; ts.Unix() < endTS.Unix(); ts = ts.Add(1e9) {
		for _, user := range users {
			for _, cid := range cids {
				for _, queryID := range queryIds {
					row := []interface{}{ts, "lb_part", 1, 1,
						ts, cid, host, user, dbname, queryID,
						i, f, f, f, f, f, i, i, i, i, i, i, i, i, i, i, i, f, f, i, i, f, f, queries[queryID], "{_rest}"}
					PGStatStatementsTestData = append(PGStatStatementsTestData, row)
					i++
					f++
				}
			}
		}
	}
	return PGStatStatementsTestData
}
