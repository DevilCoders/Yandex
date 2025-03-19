package functest

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
)

var myPerfDiagSessionsStatResponse = []mymodels.SessionsStats{
	{
		Timestamp:        optional.Time{Valid: true, Time: time.Unix(1, 0)},
		Dimensions:       map[string]string{"stage": "stage/sql/Sending data", "user": "user1"},
		SessionsCount:    14,
		NextMessageToken: 87,
	},
}

var myPerfDiagSessionsAtTimeResponse = mymodels.SessionsAtTime{
	Sessions: []mymodels.SessionState{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "db1",
			User:      "user1",
			Query:     "SELECT * FROM t1 WHERE id = 42",
		},
	},
	PreviousCollectTime: time.Unix(1, 0),
	NextCollectTime:     time.Unix(3, 0),
	NextMessageToken:    0,
}

var myPerfDiagStatementsAtTimeResponse = mymodels.StatementsAtTime{
	Statements: []mymodels.Statements{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "db1",
			Query:     "SELECT * FROM t1 WHERE id = ?",
		},
	},
	PreviousCollectTime: time.Unix(1, 0),
	NextCollectTime:     time.Unix(3, 0),
	NextMessageToken:    0,
}

var myPerfDiagStatementsDiff = mymodels.DiffStatements{
	DiffStatements: []mymodels.DiffStatement{
		{
			FirstIntervalTime: mymodels.Interval{
				StartTimestamp: optional.Time{Time: time.Unix(1, 0), Valid: true},
				EndTimestamp:   optional.Time{Time: time.Unix(2, 0), Valid: true},
			},
			SecondIntervalTime: mymodels.Interval{
				StartTimestamp: optional.Time{Time: time.Unix(3, 0), Valid: true},
				EndTimestamp:   optional.Time{Time: time.Unix(4, 0), Valid: true},
			},
			DiffStatementBase: mymodels.DiffStatementBase{
				Host:        "host1",
				Database:    "db1",
				Digest:      "deadbeaf1",
				Query:       "SELECT * FROM t1 WHERE id = ?",
				FirstCalls:  14,
				SecondCalls: 87,
				DiffCalls:   0,
			},
		},
	},
	NextMessageToken: 0,
}

var myPerfDiagStatementsIntervalResponse = mymodels.StatementsInterval{
	Statements: []mymodels.Statements{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "db1",
			Digest:    "deadbeaf1",
			Query:     "SELECT * FROM t1 WHERE id = ?",
		},
	},
	NextMessageToken: 0,
}

var myPerfDiagStatementsStatsResponse = mymodels.StatementsStats{
	Statements: []mymodels.Statements{
		{
			Timestamp: time.Unix(2, 0),
			Database:  "db1",
		},
	},
	NextMessageToken: 0,
}
