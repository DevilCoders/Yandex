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
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb/clickhouse"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var (
	tsZero        = time.Unix(0, 0)
	tsStart       = time.Unix(1595688814, 0)
	tsBeforeStart = tsStart.Add(-34 * time.Second)
	tsEnd         = time.Unix(1595688850, 0)
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
		GroupBy      []mymodels.MySessionsColumn
		OrderBy      []mymodels.OrderBy
		Res          []mymodels.SessionsStats
		More         bool
		Err          error
	}{
		{
			Name:         "NoFilter",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsUser,
				mymodels.MySessionsTime,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsUser, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp: optional.NewTime(tsBeforeStart),
					Dimensions: map[string]string{
						"user": "user1",
					},
					SessionsCount:    52,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "GroupByDigest",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDigest,
				mymodels.MySessionsTime,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsDigest, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp: optional.NewTime(tsBeforeStart),
					Dimensions: map[string]string{
						"digest": "deadbeaf1",
						"query":  "SELECT * FROM t1 WHERE id = 42",
					},
					SessionsCount:    104,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "Filter",
			Filter:       "stats.user='user1'",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDatabase,
				mymodels.MySessionsTime,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp:        optional.NewTime(tsBeforeStart),
					Dimensions:       map[string]string{"database": "business", "stage": "stage/sql/Sending data"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "FilterIn",
			Filter:       "stats.user in ('user1')",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDatabase,
				mymodels.MySessionsTime,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp:        optional.NewTime(tsBeforeStart),
					Dimensions:       map[string]string{"database": "business", "stage": "stage/sql/Sending data"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "FilterNotIn",
			Filter:       "stats.user NOT IN ('user2', 'user3')",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDatabase,
				mymodels.MySessionsTime,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp:        optional.NewTime(tsBeforeStart),
					Dimensions:       map[string]string{"database": "business", "stage": "stage/sql/Sending data"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "FilterNotEqual",
			Filter:       "stats.user!='user2'",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDatabase,
				mymodels.MySessionsTime,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp:        optional.NewTime(tsBeforeStart),
					Dimensions:       map[string]string{"database": "business", "stage": "stage/sql/Sending data"},
					SessionsCount:    26,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "Order",
			Filter:       "stats.user='user1'",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDatabase,
				mymodels.MySessionsTime,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByDesc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByDesc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp:        optional.NewTime(tsEnd.Add(-10 * time.Second)),
					Dimensions:       map[string]string{"database": "business", "stage": "stage/sql/statistics"},
					SessionsCount:    10,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "NoFilter",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDatabase,
				mymodels.MySessionsTime,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp:        optional.Time{Valid: true, Time: tsBeforeStart},
					Dimensions:       map[string]string{"database": "business", "stage": "stage/sql/Sending data"},
					SessionsCount:    52,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "NoRollup",
			Limit:        1,
			RollupPeriod: 1,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsDatabase,
				mymodels.MySessionsTime,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Timestamp:        optional.NewTime(tsStart),
					Dimensions:       map[string]string{"database": "business", "stage": "stage/sql/Sending data"},
					SessionsCount:    2,
					NextMessageToken: 1,
				},
			},
			More: true,
		},
		{
			Name:         "NoTime",
			Limit:        4,
			RollupPeriod: 1,
			GroupBy: []mymodels.MySessionsColumn{
				mymodels.MySessionsUser,
				mymodels.MySessionsStage,
			},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsUser, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res: []mymodels.SessionsStats{
				{
					Dimensions:       map[string]string{"user": "user1", "stage": "stage/sql/Sending data"},
					SessionsCount:    36,
					NextMessageToken: 1,
				},
				{
					Dimensions:       map[string]string{"user": "user1", "stage": "stage/sql/statistics"},
					SessionsCount:    36,
					NextMessageToken: 2,
				},
				{
					Dimensions:       map[string]string{"user": "user2", "stage": "stage/sql/Sending data"},
					SessionsCount:    36,
					NextMessageToken: 3,
				},
				{
					Dimensions:       map[string]string{"user": "user2", "stage": "stage/sql/statistics"},
					SessionsCount:    36,
					NextMessageToken: 4,
				},
			},
			More: false,
		},
		{
			Name:         "NoGroupBy",
			Limit:        1,
			RollupPeriod: 60,
			GroupBy:      []mymodels.MySessionsColumn{},
			OrderBy: []mymodels.OrderBy{
				{Field: mymodels.MySessionsTime, SortOrder: mymodels.OrderByAsc},
				{Field: mymodels.MySessionsStage, SortOrder: mymodels.OrderByAsc},
			},
			Res:  nil,
			More: false,
			Err:  clickhouse.ErrGroupByRequired,
		},
	}
	// SessionStats runs
	for _, input := range sessionsStatsInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.SessionsStats(сtx, "cid1", input.Limit, 0, tsStart,
				time.Unix(1595688874, 0), input.RollupPeriod, input.GroupBy, input.OrderBy, f)
			if input.Err != nil {
				require.ErrorIs(t, err, input.Err)
			} else {
				require.NoError(t, err)
			}
			require.Equal(t, input.More, more)
			require.Equal(t, msg, input.Res)
		})
	}

	sessionsAtTimeInputs := []struct {
		Name         string
		Filter       string
		Limit        int64
		OrderBy      []mymodels.OrderBySessionsAtTime
		ColumnFilter []mymodels.MySessionsColumn
		Time         time.Time
		Res          mymodels.SessionsAtTime
		More         bool
		Err          error
	}{
		{
			Name:  "sessionsWithNulls",
			Limit: 1,
			ColumnFilter: []mymodels.MySessionsColumn{
				"user",
				"collect_time",
				"current_wait",
				"wait_object",
				"wait_latency",
				"query",
			},
			OrderBy: []mymodels.OrderBySessionsAtTime{
				{
					Field:     mymodels.MySessionsTime,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			Time: tsBeforeStart,
			Res: mymodels.SessionsAtTime{
				Sessions: []mymodels.SessionState{
					{
						Timestamp:   tsStart,
						User:        "user1",
						Query:       "SELECT * FROM t1 WHERE id = 42",
						CurrentWait: "",
						WaitObject:  "",
						WaitLatency: 0,
					},
				},
				PreviousCollectTime: tsStart,
				NextCollectTime:     tsStart.Add(time.Second),
				NextMessageToken:    1,
			},
			More: true,
		},
		{
			Name:         "sessionsAtTimeNoExistsDateLeft",
			Limit:        1,
			ColumnFilter: []mymodels.MySessionsColumn{"stage", "user", "collect_time", "query"},
			OrderBy: []mymodels.OrderBySessionsAtTime{
				{
					Field:     mymodels.MySessionsTime,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			Time: tsBeforeStart,
			Res: mymodels.SessionsAtTime{
				Sessions: []mymodels.SessionState{
					{
						Timestamp: tsStart,
						User:      "user1",
						Stage:     "stage/sql/statistics",
						Query:     "SELECT * FROM t1 WHERE id = 42",
					},
				},
				PreviousCollectTime: tsStart,
				NextCollectTime:     tsStart.Add(time.Second),
				NextMessageToken:    1,
			},
			More: true,
		},
		{
			Name:         "sessionsAtTimeNoExistsDateRight",
			Limit:        1,
			ColumnFilter: []mymodels.MySessionsColumn{"stage", "user", "collect_time"},
			OrderBy: []mymodels.OrderBySessionsAtTime{
				{
					Field:     mymodels.MySessionsTime,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			Time: tsEnd.Add(100500 * time.Hour),
			Res: mymodels.SessionsAtTime{
				Sessions: []mymodels.SessionState{
					{
						Timestamp: tsEnd.Add(-1 * time.Second),
						User:      "user1",
						Stage:     "stage/sql/statistics",
					},
				},
				PreviousCollectTime: tsEnd.Add(-2 * time.Second),
				NextCollectTime:     tsEnd.Add(-1 * time.Second),
				NextMessageToken:    1,
			},
			More: true,
		},
		{
			Name:         "sessionsAtTimeExistsDate",
			Limit:        2,
			Filter:       "stats.user!='user1'",
			ColumnFilter: []mymodels.MySessionsColumn{"stage", "user", "collect_time"},
			OrderBy: []mymodels.OrderBySessionsAtTime{
				{
					Field:     mymodels.MySessionsStage,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			Time: tsEnd.Add(-10 * time.Second),
			Res: mymodels.SessionsAtTime{
				Sessions: []mymodels.SessionState{
					{
						Timestamp: tsEnd.Add(-10 * time.Second),
						User:      "user2",
						Stage:     "stage/sql/Sending data",
					},
					{
						Timestamp: tsEnd.Add(-10 * time.Second),
						User:      "user2",
						Stage:     "stage/sql/statistics",
					},
				},
				PreviousCollectTime: tsEnd.Add(-11 * time.Second),
				NextCollectTime:     tsEnd.Add(-9 * time.Second),
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

	statementsAtTimeInputs := []struct {
		Name         string
		Filter       string
		Limit        int64
		OrderBy      []mymodels.OrderByStatementsAtTime
		ColumnFilter []mymodels.MyStatementsColumn
		Time         time.Time
		Res          mymodels.StatementsAtTime
		More         bool
		Err          error
	}{
		{
			Name:         "NoExistsDateLeftStatementsAtTime",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{"query", "digest", "collect_time", "calls"},
			OrderBy: []mymodels.OrderByStatementsAtTime{
				{
					Field:     mymodels.MyStatementsTime,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			Time: tsZero,
			Res: mymodels.StatementsAtTime{
				Statements: []mymodels.Statements{
					{
						Timestamp: tsStart,
						Digest:    "deadbeaf1",
						Query:     "SELECT * FROM t1 WHERE id = ?",
						Calls:     1,
					},
				},
				PreviousCollectTime: tsStart,
				NextCollectTime:     tsStart.Add(time.Second),
				NextMessageToken:    1,
			},
			More: true,
		},
		{
			Name:         "NoExistsDateRightStatementsAtTime",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{"query", "database", "digest", "collect_time", "calls"},
			OrderBy: []mymodels.OrderByStatementsAtTime{
				{
					Field:     mymodels.MyStatementsTime,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			Time: tsEnd.Add(100500 * time.Hour),
			Res: mymodels.StatementsAtTime{
				Statements: []mymodels.Statements{
					{
						Timestamp: tsEnd.Add(-1 * time.Second),
						Database:  "db1",
						Digest:    "deadbeaf1",
						Query:     "SELECT * FROM t1 WHERE id = ?",
						Calls:     561,
					},
				},
				PreviousCollectTime: tsEnd.Add(-2 * time.Second),
				NextCollectTime:     tsEnd.Add(-1 * time.Second),
				NextMessageToken:    1,
			},
			More: true,
		},
		{
			Name:         "StatementsAtTimeExists",
			Limit:        2,
			Filter:       "stats.database='db2'",
			ColumnFilter: []mymodels.MyStatementsColumn{"query", "database", "digest", "collect_time", "calls"},
			OrderBy: []mymodels.OrderByStatementsAtTime{
				{
					Field:     mymodels.MyStatementsTime,
					SortOrder: mymodels.OrderByAsc,
				},
				{
					Field:     mymodels.MyStatementsDigest,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			Time: tsEnd.Add(-10 * time.Second),
			Res: mymodels.StatementsAtTime{
				Statements: []mymodels.Statements{
					{
						Timestamp: tsEnd.Add(-10 * time.Second),
						Database:  "db2",
						Digest:    "deadbeaf1",
						Query:     "SELECT * FROM t1 WHERE id = ?",
						Calls:     425,
					},
					{
						Timestamp: tsEnd.Add(-10 * time.Second),
						Database:  "db2",
						Digest:    "deadbeaf2",
						Query:     "SELECT * FROM t1 WHERE id = ?",
						Calls:     426,
					},
				},
				PreviousCollectTime: tsEnd.Add(-11 * time.Second),
				NextCollectTime:     tsEnd.Add(-9 * time.Second),
				NextMessageToken:    2,
			},
			More: true,
		},
	}

	// StatementsDiff runs
	for _, input := range statementsAtTimeInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.StatementsAtTime(сtx, "cid1", input.Limit, 0, input.Time, input.ColumnFilter, f, input.OrderBy)
			// sqlx put some timezone info in time struct
			for i, f := range msg.Statements {
				msg.Statements[i].Timestamp = time.Unix(f.Timestamp.Unix(), 0)
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
		OrderBy        []mymodels.OrderByStatementsAtTime
		ColumnFilter   []mymodels.MyStatementsColumn
		FirstInterval  time.Time
		SecondInterval time.Time
		Duration       int64
		Res            mymodels.DiffStatements
		More           bool
		Err            error
	}{
		{
			Name:         "StatementsDiff",
			Limit:        1,
			Duration:     5,
			ColumnFilter: []mymodels.MyStatementsColumn{"query", "collect_time", "database", "digest", "calls"},
			OrderBy: []mymodels.OrderByStatementsAtTime{
				{
					Field:     mymodels.MyStatementsDigest,
					SortOrder: mymodels.OrderByAsc,
				},
				{
					Field:     mymodels.MyStatementsDatabase,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			FirstInterval:  tsStart,
			SecondInterval: tsStart.Add(5 * time.Second),
			Res: mymodels.DiffStatements{
				DiffStatements: []mymodels.DiffStatement{
					{
						FirstIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: tsStart, Valid: true},
							EndTimestamp:   optional.Time{Time: tsStart.Add(5 * time.Second), Valid: true},
						},
						SecondIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: tsStart.Add(5 * time.Second), Valid: true},
							EndTimestamp:   optional.Time{Time: tsStart.Add(10 * time.Second), Valid: true},
						},
						DiffStatementBase: mymodels.DiffStatementBase{
							Database:    "db1",
							Digest:      "deadbeaf1",
							Query:       "SELECT * FROM t1 WHERE id = ?",
							FirstCalls:  246,
							SecondCalls: 726,
							DiffCalls:   195.121,
						},
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
			ColumnFilter: []mymodels.MyStatementsColumn{"query", "collect_time", "database", "digest", "calls"},
			OrderBy: []mymodels.OrderByStatementsAtTime{
				{
					Field:     mymodels.MyStatementsDigest,
					SortOrder: mymodels.OrderByAsc,
				},
				{
					Field:     mymodels.MyStatementsDatabase,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			FirstInterval:  tsZero,
			SecondInterval: tsStart.Add(5 * time.Second),
			Res: mymodels.DiffStatements{
				DiffStatements: []mymodels.DiffStatement{
					{
						FirstIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
							EndTimestamp:   optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
						},
						SecondIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: tsStart.Add(5 * time.Second), Valid: true},
							EndTimestamp:   optional.Time{Time: tsStart.Add(10 * time.Second), Valid: true},
						},
						DiffStatementBase: mymodels.DiffStatementBase{
							Database:    "db1",
							Digest:      "deadbeaf1",
							Query:       "SELECT * FROM t1 WHERE id = ?",
							FirstCalls:  0,
							SecondCalls: 726,
							DiffCalls:   0,
						},
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
			ColumnFilter: []mymodels.MyStatementsColumn{"query", "collect_time", "database", "digest", "calls"},
			OrderBy: []mymodels.OrderByStatementsAtTime{
				{
					Field:     mymodels.MyStatementsDigest,
					SortOrder: mymodels.OrderByAsc,
				},
				{
					Field:     mymodels.MyStatementsDatabase,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			SecondInterval: tsZero,
			FirstInterval:  tsStart,
			Res: mymodels.DiffStatements{
				DiffStatements: []mymodels.DiffStatement{
					{
						FirstIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: tsStart, Valid: true},
							EndTimestamp:   optional.Time{Time: tsStart.Add(5 * time.Second), Valid: true},
						},
						SecondIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
							EndTimestamp:   optional.Time{Time: time.Unix(time.Time{}.Unix(), 0), Valid: false},
						},
						DiffStatementBase: mymodels.DiffStatementBase{
							Database:   "db1",
							Digest:     "deadbeaf1",
							Query:      "SELECT * FROM t1 WHERE id = ?",
							FirstCalls: 246,
							DiffCalls:  -100,
						},
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
			ColumnFilter: []mymodels.MyStatementsColumn{"query", "collect_time", "database", "digest", "calls"},
			OrderBy: []mymodels.OrderByStatementsAtTime{
				{
					Field:     mymodels.MyStatementsDigest,
					SortOrder: mymodels.OrderByAsc,
				},
				{
					Field:     mymodels.MyStatementsDatabase,
					SortOrder: mymodels.OrderByAsc,
				},
			},
			SecondInterval: tsStart.Add(5 * time.Second),
			FirstInterval:  tsStart.Add(5 * time.Second),

			Res: mymodels.DiffStatements{
				DiffStatements: []mymodels.DiffStatement{
					{
						FirstIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: tsStart.Add(5 * time.Second), Valid: true},
							EndTimestamp:   optional.Time{Time: tsStart.Add(10 * time.Second), Valid: true},
						},
						SecondIntervalTime: mymodels.Interval{
							StartTimestamp: optional.Time{Time: tsStart.Add(5 * time.Second), Valid: true},
							EndTimestamp:   optional.Time{Time: tsStart.Add(10 * time.Second), Valid: true},
						},
						DiffStatementBase: mymodels.DiffStatementBase{
							Database:    "db1",
							Digest:      "deadbeaf1",
							Query:       "SELECT * FROM t1 WHERE id = ?",
							FirstCalls:  726,
							SecondCalls: 726,
							DiffCalls:   0,
						},
					},
				},
				NextMessageToken: 1,
			},
			More: true,
		},
	}

	// StatementsAtTime runs
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
		OrderBy      []mymodels.OrderByStatementsAtTime
		ColumnFilter []mymodels.MyStatementsColumn
		FromTS, ToTS time.Time
		Res          mymodels.StatementsInterval
		More         bool
		Err          error
	}{
		{
			Name:         "NonExistsDate",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{},
			OrderBy:      []mymodels.OrderByStatementsAtTime{},
			FromTS:       tsZero,
			ToTS:         tsZero,
			Res: mymodels.StatementsInterval{
				Statements:       nil,
				NextMessageToken: 0,
			},
			More: false,
		},
		{
			Name:         "ExistsDate",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{},
			OrderBy:      []mymodels.OrderByStatementsAtTime{},
			FromTS:       tsStart.Add(5 * time.Second),
			ToTS:         tsStart.Add(5 * time.Second),
			Res: mymodels.StatementsInterval{
				Statements: []mymodels.Statements{{
					Timestamp:           tsStart.Add(5 * time.Second),
					Host:                "host1",
					Database:            "db1",
					Digest:              "deadbeaf1",
					Query:               "SELECT * FROM t1 WHERE id = ?",
					TotalQueryLatency:   100,
					TotalLockLatency:    10,
					AvgQueryLatency:     100,
					AvgLockLatency:      10,
					Calls:               81,
					Errors:              0,
					Warnings:            0,
					RowsExamined:        81,
					RowsSent:            81,
					RowsAffected:        81,
					TmpTables:           81,
					TmpDiskTables:       81,
					SelectFullJoin:      81,
					SelectFullRangeJoin: 81,
					SelectRange:         81,
					SelectRangeCheck:    81,
					SelectScan:          81,
					SortMergePasses:     81,
					SortRange:           81,
					SortRows:            81,
					SortScan:            81,
					NoIndexUsed:         81,
					NoGoodIndexUsed:     81,
				}},
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
		ColumnFilter []mymodels.MyStatementsColumn
		FromTS, ToTS time.Time
		Res          mymodels.StatementsStats
		GroupBy      []mymodels.StatementsStatsGroupBy
		More         bool
		Err          error
	}{
		{
			Name:         "NonExistsDate",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{},
			FromTS:       tsZero,
			ToTS:         tsZero,
			Filter:       "stats.digest='deadbeaf1'",
			GroupBy: []mymodels.StatementsStatsGroupBy{
				mymodels.StatementsStatsGroupByHost,
			},
			Res: mymodels.StatementsStats{
				Statements:       nil,
				NextMessageToken: 0,
			},
			More: false,
		},
		{
			Name:         "ExistsDate",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{},
			FromTS:       tsStart.Add(5 * time.Second),
			ToTS:         tsStart.Add(5 * time.Second),
			GroupBy: []mymodels.StatementsStatsGroupBy{
				mymodels.StatementsStatsGroupByDatabase,
			},
			Filter: "stats.digest='deadbeaf3'",
			Res: mymodels.StatementsStats{
				Statements: []mymodels.Statements{{
					Timestamp:           time.Unix(tsStart.Unix()-tsStart.Unix()%10, 0),
					Host:                "",
					Database:            "db2",
					Digest:              "",
					Query:               "",
					TotalQueryLatency:   100,
					TotalLockLatency:    10,
					AvgQueryLatency:     100,
					AvgLockLatency:      10,
					Calls:               91,
					Errors:              0,
					Warnings:            0,
					RowsExamined:        91,
					RowsSent:            91,
					RowsAffected:        91,
					TmpTables:           91,
					TmpDiskTables:       91,
					SelectFullJoin:      91,
					SelectFullRangeJoin: 91,
					SelectRange:         91,
					SelectRangeCheck:    91,
					SelectScan:          91,
					SortMergePasses:     91,
					SortRange:           91,
					SortRows:            91,
					SortScan:            91,
					NoIndexUsed:         91,
					NoGoodIndexUsed:     91,
				}},
				NextMessageToken: 1,
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
	//time.Sleep(time.Hour)

	statementStatsInputs := []struct {
		Name         string
		Digest       string
		Filter       string
		Limit        int64
		ColumnFilter []mymodels.MyStatementsColumn
		FromTS, ToTS time.Time
		Res          mymodels.StatementStats
		GroupBy      []mymodels.StatementsStatsGroupBy
		More         bool
		Err          error
	}{
		{
			Name:         "NonExistsDate",
			Digest:       "deadbeaf1",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{},
			FromTS:       tsZero,
			ToTS:         tsZero,
			GroupBy: []mymodels.StatementsStatsGroupBy{
				mymodels.StatementsStatsGroupByHost,
			},
			Res: mymodels.StatementStats{
				Query:            "",
				Statements:       nil,
				NextMessageToken: 0,
			},
			More: false,
		},
		{
			Name:         "SimpleQuery",
			Digest:       "deadbeaf1",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{},
			FromTS:       tsStart.Add(5 * time.Second),
			ToTS:         tsEnd,
			GroupBy:      []mymodels.StatementsStatsGroupBy{},
			Res: mymodels.StatementStats{
				Query: "SELECT * FROM t1 WHERE id = ?",
				Statements: []mymodels.Statements{{
					Timestamp:           time.Unix(tsStart.Unix()-tsStart.Unix()%10, 0),
					Host:                "",
					Database:            "",
					Digest:              "deadbeaf1",
					Query:               "",
					TotalQueryLatency:   100,
					TotalLockLatency:    10,
					AvgQueryLatency:     100,
					AvgLockLatency:      10,
					Calls:               85,
					RowsExamined:        85,
					RowsSent:            85,
					RowsAffected:        85,
					TmpTables:           85,
					TmpDiskTables:       85,
					SelectFullJoin:      85,
					SelectFullRangeJoin: 85,
					SelectRange:         85,
					SelectRangeCheck:    85,
					SelectScan:          85,
					SortMergePasses:     85,
					SortRange:           85,
					SortRows:            85,
					SortScan:            85,
					NoIndexUsed:         85,
					NoGoodIndexUsed:     85,
				}},
				NextMessageToken: 1,
			},
			More: true,
		},
		{
			Name:         "SimpleQueryWithGroupBy",
			Digest:       "deadbeaf1",
			Limit:        1,
			ColumnFilter: []mymodels.MyStatementsColumn{},
			FromTS:       tsStart.Add(5 * time.Second),
			ToTS:         tsEnd,
			GroupBy: []mymodels.StatementsStatsGroupBy{
				mymodels.StatementsStatsGroupByDatabase,
			},
			Res: mymodels.StatementStats{
				Query: "SELECT * FROM t1 WHERE id = ?",
				Statements: []mymodels.Statements{{
					Timestamp:           time.Unix(tsStart.Unix()-tsStart.Unix()%10, 0),
					Host:                "",
					Database:            "db1",
					Digest:              "deadbeaf1",
					Query:               "",
					TotalQueryLatency:   100,
					TotalLockLatency:    10,
					AvgQueryLatency:     100,
					AvgLockLatency:      10,
					Calls:               81,
					Errors:              0,
					Warnings:            0,
					RowsExamined:        81,
					RowsSent:            81,
					RowsAffected:        81,
					TmpTables:           81,
					TmpDiskTables:       81,
					SelectFullJoin:      81,
					SelectFullRangeJoin: 81,
					SelectRange:         81,
					SelectRangeCheck:    81,
					SelectScan:          81,
					SortMergePasses:     81,
					SortRange:           81,
					SortRows:            81,
					SortScan:            81,
					NoIndexUsed:         81,
					NoGoodIndexUsed:     81,
				}},
				NextMessageToken: 1,
			},
			More: true,
		},
	}

	// StatementsStats runs
	for _, input := range statementStatsInputs {
		t.Run(input.Name, func(t *testing.T) {
			f, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			msg, more, err := back.StatementStats(сtx, "cid1", input.Digest, input.Limit, 0, input.FromTS, input.ToTS, input.GroupBy, f, input.ColumnFilter)
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
		"my_sessions": `
CREATE TABLE perf_diag.my_sessions (_timestamp DateTime, _partition String, _offset UInt64, _idx UInt32, host String,
cluster_id String, collect_time DateTime, database Nullable(String), user Nullable(String),
thd_id Nullable(UInt32), conn_id Nullable(UInt32), command Nullable(String), query Nullable(String),
digest Nullable(String), query_latency Nullable(Float64), lock_latency Nullable(Float64), stage Nullable(String),
stage_latency Nullable(Float64), current_wait Nullable(String), wait_object Nullable(String),
wait_latency Nullable(Float64), trx_latency Nullable(Float64), current_memory Nullable(UInt64),
client_addr Nullable(String), client_hostname Nullable(String), client_port Nullable(UInt32),
_rest Nullable(String)) ENGINE = Memory;
`,
		"my_statements": `
CREATE TABLE perf_diag.my_statements (_timestamp DateTime, _partition String, _offset UInt64, _idx UInt32,
digest String, database String, host String, cluster_id String, collect_time DateTime, rows_affected Nullable(UInt64),
total_query_latency Nullable(Float64), total_lock_latency Nullable(Float64), avg_query_latency Nullable(Float64),
avg_lock_latency Nullable(Float64), calls Nullable(UInt64), errors Nullable(UInt64), warnings Nullable(UInt64),
rows_examined Nullable(UInt64), rows_sent Nullable(UInt64), query Nullable(String), tmp_tables Nullable(UInt64),
tmp_disk_tables Nullable(UInt64), select_full_join Nullable(UInt64), select_full_range_join Nullable(UInt64),
select_range Nullable(UInt64), select_range_check Nullable(UInt64), select_scan Nullable(UInt64),
sort_merge_passes Nullable(UInt64), sort_range Nullable(UInt64), sort_rows Nullable(UInt64),
sort_scan Nullable(UInt64), no_index_used Nullable(UInt64), no_good_index_used Nullable(UInt64),
_rest Nullable(String)) ENGINE = Memory;
`,
	}
	insertQueries := map[string]string{
		"my_sessions": `
INSERT INTO perf_diag.my_sessions
(_timestamp , _partition , _offset , _idx , host ,
cluster_id , collect_time , database , user ,
thd_id , conn_id , command , query ,
digest , query_latency , lock_latency , stage ,
stage_latency , current_wait , wait_object ,
wait_latency , trx_latency , current_memory ,
client_addr , client_hostname , client_port , _rest)
VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
`,
		"my_statements": `
INSERT INTO perf_diag.my_statements
(_timestamp , _partition , _offset , _idx ,
digest , database , host , cluster_id , collect_time , rows_affected ,
total_query_latency , total_lock_latency , avg_query_latency ,
avg_lock_latency , calls , errors , warnings ,
rows_examined , rows_sent , query , tmp_tables ,
tmp_disk_tables , select_full_join , select_full_range_join ,
select_range , select_range_check , select_scan ,
sort_merge_passes , sort_range , sort_rows ,
sort_scan , no_index_used , no_good_index_used , _rest)
VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
`,
	}

	for _, query := range createQueries {
		_, err = db.Exec(query)
		if err != nil {
			return err
		}
	}

	data := make(map[string][][]interface{})

	data["my_sessions"] = genDataMySessions()
	data["my_statements"] = genDataMyStatements()
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

func genDataMySessions() [][]interface{} {
	mySessionsTestData := make([][]interface{}, 1)

	var (
		users       = []string{"user1", "user2"}
		cids        = []string{"cid1", "cid2"}
		dbname      = "business"
		host        = "host1"
		stages      = []string{"stage/sql/statistics", "stage/sql/Sending data"}
		query       = "SELECT * FROM t1 WHERE id = 42"
		digest      = "deadbeaf1"
		currentWait = interface{}("wait/io/file/innodb/innodb_data_file")
		waitObject  = interface{}("/var/lib/mysql/db1/t1.ibd")
		waitLatency = interface{}(1.0)
	)

	for ts := tsStart; ts.Unix() < tsEnd.Unix(); ts = ts.Add(time.Second) {
		for _, user := range users {
			for _, cid := range cids {
				for _, stage := range stages {
					thdID := len(mySessionsTestData)
					connID := thdID
					if connID%2 != 0 {
						currentWait = nil
						waitObject = nil
						waitLatency = nil
					}
					row := []interface{}{ts, "lb_part", 123, 1, host,
						cid, ts, dbname, user,
						thdID, connID, "Execute", query,
						digest, float64(100), float64(1), stage,
						float64(10), currentWait, waitObject,
						waitLatency, float64(100), 1024,
						"10.0.5.5", "app1.domain1", 32451, "{}"}
					mySessionsTestData = append(mySessionsTestData, row)
				}
			}
		}
	}
	return mySessionsTestData
}

func genDataMyStatements() [][]interface{} {
	MyStatementsTestData := make([][]interface{}, 1)

	var (
		databases = []string{"db1", "db2"}
		cids      = []string{"cid1", "cid2"}
		host      = "host1"
		digests   = []string{"deadbeaf1", "deadbeaf2", "deadbeaf3", "deadbeaf4"}
		query     = "SELECT * FROM t1 WHERE id = ?"
	)

	i := 1
	for ts := tsStart; ts.Unix() < tsEnd.Unix(); ts = ts.Add(time.Second) {
		for _, dbname := range databases {
			for _, cid := range cids {
				for _, digest := range digests {
					row := []interface{}{ts, "lb_part", 123, 1,
						digest, dbname, host, cid, ts, i,
						float64(100), float64(10), float64(100),
						float64(10), i, 0, 0,
						i, i, query, i,
						i, i, i,
						i, i, i,
						i, i, i,
						i, i, i, "{}"}
					MyStatementsTestData = append(MyStatementsTestData, row)
					i++
				}
			}
		}
	}
	return MyStatementsTestData
}
