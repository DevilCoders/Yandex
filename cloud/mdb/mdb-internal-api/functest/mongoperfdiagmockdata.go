package functest

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
)

var mongoGetProfilerStatsResponse = []mongomodels.ProfilerStats{
	{
		Time:       time.Unix(1, 0),
		Dimensions: map[string]string{"form": "{a:1, b:1}"},
		Value:      10,
	},
}

var mongoGetProfilerRecsAtTimeResponse = []mongomodels.ProfilerRecs{
	{
		Time:              time.Unix(1, 0),
		RawRequest:        "{some_request: 'raw'}",
		RequestForm:       "{a:1, b:1}",
		Hostname:          "man-0.db.yanex.net",
		User:              "user1",
		Namespace:         "db.collention",
		Operation:         "query",
		Duration:          512,
		PlanSummary:       "COLLSCAN",
		ResponseLength:    128,
		KeysExamined:      102400,
		DocumentsExamined: 32,
		DocumentsReturned: 32,
	},
}

var mongoGetProfilerTopFormsByStat = []mongomodels.TopForms{
	{
		RequestForm:                          "{a:1, b:1}",
		PlanSummary:                          "COLLSCAN",
		QueriesCount:                         128,
		TotalQueriesDuration:                 102400,
		AVGQueryDuration:                     800,
		TotalResponseLength:                  65536,
		AVGResponseLength:                    512,
		TotalKeysExamined:                    10000000,
		TotalDocumentsExamined:               456,
		TotalDocumentsReturned:               300,
		KeysExaminedPerDocumentReturned:      33333,
		DocumentsExaminedPerDocumentReturned: 2,
	},
}
var mongoGetPossibleIndexes = []mongomodels.PossibleIndexes{
	{
		Database:     "db",
		Collection:   "collection",
		Index:        "{a:1, b:1}",
		RequestCount: 128,
	},
}
