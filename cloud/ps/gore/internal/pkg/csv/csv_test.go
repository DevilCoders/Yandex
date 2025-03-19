package csv

import (
	"errors"
	"io"
	"testing"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
)

type testCase struct {
	Name    string
	Input   string
	Service service.Service
	Output  []shift.Shift
	Err     error
}

var testService = service.Service{
	Service:  "Test",
	Timezone: "Europe/Moscow",
	Schedule: &service.Schedule{
		Method: "file:bitbucket",
		Order: map[string]int{
			"primary": 0,
			"backup":  1,
		},
		KwArgs: map[string]string{
			"format":     "date:02/01/2006,staff:primary,staff:backup",
			"has_header": "true",
			"time":       "12:00",
		},
	},
}

var testCases = []testCase{
	{
		Name:    "One day long shifts",
		Service: testService,
		Err:     nil,
		Input: `
Date,Primary,Backup
01/12/2020,staff:novikoff,staff:skipor
02/12/2020,staff:baranovich,staff:novikoff
03/12/2020,staff:ascheglov,staff:baranovich
04/12/2020,staff:skipor,staff:ascheglov
05/12/2020,staff:kozhapenko,staff:skipor
06/12/2020,staff:manykey,staff:kozhapenko
07/12/2020,staff:akhaustov,staff:manykey
08/12/2020,staff:seukyaso,staff:akhaustov
09/12/2020,staff:iceman,staff:seukyaso
10/12/2020,staff:elemir90,staff:iceman
11/12/2020,staff:phmx,staff:elemir90
12/12/2020,staff:novikoff,staff:phmx
13/12/2020,staff:akhaustov,staff:novikoff
14/12/2020,staff:ascheglov,staff:akhaustov
15/12/2020,staff:skipor,staff:ascheglov
16/12/2020,staff:kozhapenko,staff:skipor`,
		Output: []shift.Shift{
			{ServiceID: "000000000000000000000000", DateStart: 1606813200, DateEnd: 1606899600, Resp: shift.Resp{Order: 1, Username: "skipor"}},
			{ServiceID: "000000000000000000000000", DateStart: 1606813200, DateEnd: 1606899600, Resp: shift.Resp{Order: 0, Username: "novikoff"}},
			{ServiceID: "000000000000000000000000", DateStart: 1606899600, DateEnd: 1606986000, Resp: shift.Resp{Order: 1, Username: "novikoff"}},
			{ServiceID: "000000000000000000000000", DateStart: 1606899600, DateEnd: 1606986000, Resp: shift.Resp{Order: 0, Username: "baranovich"}},
			{ServiceID: "000000000000000000000000", DateStart: 1606986000, DateEnd: 1607072400, Resp: shift.Resp{Order: 0, Username: "ascheglov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1606986000, DateEnd: 1607072400, Resp: shift.Resp{Order: 1, Username: "baranovich"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607072400, DateEnd: 1607158800, Resp: shift.Resp{Order: 0, Username: "skipor"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607072400, DateEnd: 1607158800, Resp: shift.Resp{Order: 1, Username: "ascheglov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607158800, DateEnd: 1607245200, Resp: shift.Resp{Order: 1, Username: "skipor"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607158800, DateEnd: 1607245200, Resp: shift.Resp{Order: 0, Username: "kozhapenko"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607245200, DateEnd: 1607331600, Resp: shift.Resp{Order: 0, Username: "manykey"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607245200, DateEnd: 1607331600, Resp: shift.Resp{Order: 1, Username: "kozhapenko"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607331600, DateEnd: 1607418000, Resp: shift.Resp{Order: 0, Username: "akhaustov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607331600, DateEnd: 1607418000, Resp: shift.Resp{Order: 1, Username: "manykey"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607418000, DateEnd: 1607504400, Resp: shift.Resp{Order: 0, Username: "seukyaso"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607418000, DateEnd: 1607504400, Resp: shift.Resp{Order: 1, Username: "akhaustov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607504400, DateEnd: 1607590800, Resp: shift.Resp{Order: 1, Username: "seukyaso"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607504400, DateEnd: 1607590800, Resp: shift.Resp{Order: 0, Username: "iceman"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607590800, DateEnd: 1607677200, Resp: shift.Resp{Order: 0, Username: "elemir90"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607590800, DateEnd: 1607677200, Resp: shift.Resp{Order: 1, Username: "iceman"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607677200, DateEnd: 1607763600, Resp: shift.Resp{Order: 0, Username: "phmx"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607677200, DateEnd: 1607763600, Resp: shift.Resp{Order: 1, Username: "elemir90"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607763600, DateEnd: 1607850000, Resp: shift.Resp{Order: 0, Username: "novikoff"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607763600, DateEnd: 1607850000, Resp: shift.Resp{Order: 1, Username: "phmx"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607850000, DateEnd: 1607936400, Resp: shift.Resp{Order: 0, Username: "akhaustov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607850000, DateEnd: 1607936400, Resp: shift.Resp{Order: 1, Username: "novikoff"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607936400, DateEnd: 1608022800, Resp: shift.Resp{Order: 0, Username: "ascheglov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1607936400, DateEnd: 1608022800, Resp: shift.Resp{Order: 1, Username: "akhaustov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1608022800, DateEnd: 1608109200, Resp: shift.Resp{Order: 0, Username: "skipor"}},
			{ServiceID: "000000000000000000000000", DateStart: 1608022800, DateEnd: 1608109200, Resp: shift.Resp{Order: 1, Username: "ascheglov"}},
			{ServiceID: "000000000000000000000000", DateStart: 1608109200, DateEnd: 1608195600, Resp: shift.Resp{Order: 0, Username: "kozhapenko"}},
			{ServiceID: "000000000000000000000000", DateStart: 1608109200, DateEnd: 1608195600, Resp: shift.Resp{Order: 1, Username: "skipor"}},
		},
	},
	{
		Name:    "Few days long shifts",
		Service: testService,
		Err:     nil,
		Input: `
Date,Primary,Backup
04/03/2019,staff:staerist,staff:nuraev
05/03/2019,staff:staerist,staff:nuraev
06/03/2019,staff:nuraev,staff:ekilimchuk
07/03/2019,staff:nuraev,staff:ekilimchuk
08/03/2019,staff:ekilimchuk,staff:staerist
09/03/2019,staff:ekilimchuk,staff:staerist
10/03/2019,staff:staerist,staff:nuraev
11/03/2019,staff:staerist,staff:nuraev
12/03/2019,staff:nuraev,staff:ekilimchuk
13/03/2019,staff:nuraev,staff:ekilimchuk
14/03/2019,staff:ekilimchuk,staff:staerist
15/03/2019,staff:ekilimchuk,staff:staerist
16/03/2019,staff:staerist,staff:nuraev
17/03/2019,staff:staerist,staff:nuraev
18/03/2019,staff:ekilimchuk,staff:staerist
19/03/2019,staff:ekilimchuk,staff:staerist
20/03/2019,staff:ekilimchuk,staff:staerist
21/03/2019,staff:ekilimchuk,staff:staerist
22/03/2019,staff:ekilimchuk,staff:staerist
23/03/2019,staff:ekilimchuk,staff:staerist
24/03/2019,staff:ekilimchuk,staff:staerist`,
		Output: []shift.Shift{
			{ServiceID: "000000000000000000000000", DateStart: 1551690000, DateEnd: 1551862800, Resp: shift.Resp{Order: 1, Username: "nuraev"}},
			{ServiceID: "000000000000000000000000", DateStart: 1551690000, DateEnd: 1551862800, Resp: shift.Resp{Order: 0, Username: "staerist"}},
			{ServiceID: "000000000000000000000000", DateStart: 1551862800, DateEnd: 1552035600, Resp: shift.Resp{Order: 0, Username: "nuraev"}},
			{ServiceID: "000000000000000000000000", DateStart: 1551862800, DateEnd: 1552035600, Resp: shift.Resp{Order: 1, Username: "ekilimchuk"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552035600, DateEnd: 1552208400, Resp: shift.Resp{Order: 0, Username: "ekilimchuk"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552035600, DateEnd: 1552208400, Resp: shift.Resp{Order: 1, Username: "staerist"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552208400, DateEnd: 1552381200, Resp: shift.Resp{Order: 0, Username: "staerist"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552208400, DateEnd: 1552381200, Resp: shift.Resp{Order: 1, Username: "nuraev"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552381200, DateEnd: 1552554000, Resp: shift.Resp{Order: 0, Username: "nuraev"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552381200, DateEnd: 1552554000, Resp: shift.Resp{Order: 1, Username: "ekilimchuk"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552554000, DateEnd: 1552726800, Resp: shift.Resp{Order: 0, Username: "ekilimchuk"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552554000, DateEnd: 1552726800, Resp: shift.Resp{Order: 1, Username: "staerist"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552726800, DateEnd: 1552899600, Resp: shift.Resp{Order: 0, Username: "staerist"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552726800, DateEnd: 1552899600, Resp: shift.Resp{Order: 1, Username: "nuraev"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552899600, DateEnd: 1553504400, Resp: shift.Resp{Order: 0, Username: "ekilimchuk"}},
			{ServiceID: "000000000000000000000000", DateStart: 1552899600, DateEnd: 1553504400, Resp: shift.Resp{Order: 1, Username: "staerist"}},
		},
	},
	{
		Name:    "Broken data and comments",
		Service: testService,
		Input: `
Date,Primary,Backup
06/01/2021,staff:megaeee,staff:uplink
9/01/2021,staff:cayde
10/01/2021,,staff:cayde
11/01/2021,staff:uplink,
12/01/2021,staff:apereshein,
13/01/21,staff:apereshein,
// here comest he normal data
14/01/2021,staff:apereshein,staff:kozhemyash
15/01/2021,staff:kozhemyash,staff:apereshein
16/01/2021,staff:kozhemyash,staff:apereshein
17/01/2021,staff:kozhemyash,staff:apereshein`,
		Output: []shift.Shift{
			{ServiceID: "000000000000000000000000", DateStart: 1609923600, DateEnd: 1610355600, Resp: shift.Resp{Order: 0, Username: "megaeee"}},
			{ServiceID: "000000000000000000000000", DateStart: 1609923600, DateEnd: 1610269200, Resp: shift.Resp{Order: 1, Username: "uplink"}},
			{ServiceID: "000000000000000000000000", DateStart: 1610269200, DateEnd: 1610614800, Resp: shift.Resp{Order: 1, Username: "cayde"}},
			{ServiceID: "000000000000000000000000", DateStart: 1610355600, DateEnd: 1610442000, Resp: shift.Resp{Order: 0, Username: "uplink"}},
			{ServiceID: "000000000000000000000000", DateStart: 1610442000, DateEnd: 1610701200, Resp: shift.Resp{Order: 0, Username: "apereshein"}},
			{ServiceID: "000000000000000000000000", DateStart: 1610614800, DateEnd: 1610701200, Resp: shift.Resp{Order: 1, Username: "kozhemyash"}},
			{ServiceID: "000000000000000000000000", DateStart: 1610701200, DateEnd: 1610960400, Resp: shift.Resp{Order: 0, Username: "kozhemyash"}},
			{ServiceID: "000000000000000000000000", DateStart: 1610701200, DateEnd: 1610960400, Resp: shift.Resp{Order: 1, Username: "apereshein"}},
		},
	},
	{
		Name:    "Empty data",
		Service: testService,
		Input:   "", // `some crap`,
		Err:     io.EOF,
	},
	{
		Name: "Broken service: format - date",
		Service: service.Service{
			Service:  "Test",
			Timezone: "Europe/Moscow",
			Schedule: &service.Schedule{
				KwArgs: map[string]string{
					"format":     "staff:primary,staff:backup",
					"has_header": "true",
					"time":       "12:00",
				},
			},
		},
		Input: `some crap`,
		Err:   ErrNoDateField,
	},
	{
		Name: "Broken service: format - fubar",
		Service: service.Service{
			Service:  "Test",
			Timezone: "Europe/Moscow",
			Schedule: &service.Schedule{
				KwArgs: map[string]string{
					"format":     "staff:primary",
					"has_header": "true",
					"time":       "12:00",
				},
			},
		},
		Input: `some crap`,
		Err:   ErrInvalidSchedule,
	},
}

func TestGetRespsCSV(t *testing.T) {
	gc := DefaultConfig()
	gc.Log, _ = zap.NewQloudLogger(log.DebugLevel)
	for _, test := range testCases {
		shs, err := gc.GetRespsCSV(test.Input, &test.Service)
		if !errors.Is(err, test.Err) {
			t.Errorf("Test %s. Unexpected result: %v, error %v", test.Name, shs, err)
			return
		}

		var found bool
		for i := range test.Output {
			found = false
			for _, sh := range shs {
				if sh == test.Output[i] {
					found = true
					break
				}
			}

			if !found {
				t.Errorf("Test %s. Unexpected result: %v at position %d", test.Name, shs[i], i)
				return
			}
		}
	}
}
