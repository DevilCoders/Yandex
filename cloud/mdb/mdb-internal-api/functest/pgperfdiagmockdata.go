package functest

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
)

var pgPerfDiagSessionsStatResponse = []pgmodels.SessionsStats{
	{
		Timestamp:        optional.Time{Valid: true, Time: time.Unix(1, 0)},
		Dimensions:       map[string]string{"state": "active", "usename": "mxback", "wait_event": "SyncRep"},
		SessionsCount:    14,
		NextMessageToken: 87},
}

var pgPerfDiagSessionsAtTimeResponse = pgmodels.SessionsAtTime{
	Sessions: []pgmodels.SessionState{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "maildb",
			User:      "postgres",
			Query:     "autovacuum: VACUUM mail.messages",
		},
	},
	PreviousCollectTime: time.Unix(1, 0),
	NextCollectTime:     time.Unix(3, 0),
	NextMessageToken:    0,
}

var pgPerfDiagStatementsAtTimeResponse = pgmodels.StatementsAtTime{
	Statements: []pgmodels.Statements{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "maildb",
			User:      "postgres",
			Query:     "autovacuum: VACUUM mail.messages",
		},
	},
	PreviousCollectTime: time.Unix(1, 0),
	NextCollectTime:     time.Unix(3, 0),
	NextMessageToken:    0,
}

var pgPerfDiagStatementsDiff = pgmodels.DiffStatements{
	DiffStatements: []pgmodels.DiffStatement{
		{
			FirstIntervalTime: pgmodels.Interval{
				StartTimestamp: optional.Time{Time: time.Unix(1, 0), Valid: true},
				EndTimestamp:   optional.Time{Time: time.Unix(2, 0), Valid: true},
			},
			SecondIntervalTime: pgmodels.Interval{
				StartTimestamp: optional.Time{Time: time.Unix(3, 0), Valid: true},
				EndTimestamp:   optional.Time{Time: time.Unix(4, 0), Valid: true},
			},
			Host:        "host1",
			User:        "postgres",
			Database:    "maildb",
			Queryid:     "228",
			Query:       "autovacuum: VACUUM mail.messages",
			FirstCalls:  14,
			SecondCalls: 87,
			DiffCalls:   0,
		},
	},
	NextMessageToken: 0,
}

var pgPerfDiagStatementsIntervalResponse = pgmodels.StatementsInterval{
	Statements: []pgmodels.Statements{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "maildb",
			User:      "postgres",
			Query:     "autovacuum: VACUUM mail.messages",
		},
	},
	NextMessageToken: 0,
}

var pgPerfDiagStatementsStatsResponse = pgmodels.StatementsStats{
	Statements: []pgmodels.Statements{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "maildb",
			User:      "postgres",
		},
	},
	Query:            "autovacuum: VACUUM mail.messages",
	NextMessageToken: 0,
}
