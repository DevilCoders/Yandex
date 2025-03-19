package shift

import (
	"fmt"
	"testing"
	"time"
)

type testAddUser struct {
	Input  []string
	Pos    int
	User   string
	Output []string
	Err    error
}

var testVaules = []testAddUser{
	{
		Input:  []string{"uplink", "megaeee"},
		Pos:    3,
		User:   "syndicut",
		Output: []string{"uplink", "megaeee", "syndicut"},
		Err:    nil,
	},
	{
		Input:  []string{"uplink", "megaeee"},
		Pos:    30,
		User:   "syndicut",
		Output: []string{"uplink", "megaeee", "syndicut"},
		Err:    nil,
	},
	{
		Input:  []string{"uplink", "megaeee"},
		Pos:    -1,
		User:   "syndicut",
		Output: []string{"syndicut", "uplink", "megaeee"},
		Err:    nil,
	},
	{
		Input:  []string{"uplink", "megaeee"},
		Pos:    0,
		User:   "syndicut",
		Output: []string{"syndicut", "uplink", "megaeee"},
		Err:    nil,
	},
	{
		Input:  []string{"uplink", "megaeee", "mryzh"},
		Pos:    2,
		User:   "syndicut",
		Output: []string{"uplink", "megaeee", "syndicut", "mryzh"},
		Err:    nil,
	},
	{
		Input:  []string{},
		Pos:    2,
		User:   "syndicut",
		Output: []string{"syndicut"},
		Err:    nil,
	},
	{
		Input:  []string{"uplink", "megaeee"},
		Pos:    2,
		User:   "",
		Output: []string{"uplink", "megaeee", ""},
		Err:    fmt.Errorf("nil"),
	},
	{
		Input:  []string{"uplink", "megaeee", "syndicut"},
		Pos:    1,
		User:   "syndicut",
		Output: []string{"uplink", "syndicut", "megaeee"},
		Err:    nil,
	},
	{
		Input:  []string{"syndicut", "uplink", "megaeee"},
		Pos:    2,
		User:   "syndicut",
		Output: []string{"syndicut", "uplink", "megaeee"},
		Err:    nil,
	},
}

func TestAddUserToFlattenResps(t *testing.T) {
	for i, testIter := range testVaules {
		result := AddUserToFlattenResps(testIter.Input, testIter.User, testIter.Pos)
		if len(result) != len(testIter.Output) {
			t.Errorf("Unexpected result for test %d: %v, expected %v", i, result, testIter.Output)
			continue
		}

		for j, r := range result {
			if r != testIter.Output[j] {
				t.Errorf("Unexpected result for test %d: %v, expected %v", i, result, testIter.Output)
				break
			}
		}
	}
}

var now = time.Now().Unix()
var shs = []Shift{
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 86400,
		DateEnd:   now + 86400,
		Resp: Resp{
			Order:    1,
			Username: "uplink",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 83600,
		DateEnd:   now + 83600,
		Resp: Resp{
			Order:    0,
			Username: "megaeee",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 83600,
		DateEnd:   now - 82400,
		Resp: Resp{
			Order:    1,
			Username: "megaeee",
		},
	},
}

func TestFlattenShift(t *testing.T) {
	result := FlattenShift(shs)
	if (result[0] != "uplink") || (result[1] != "megaeee") {
		t.Errorf("Unexpected result: %v", result)
	}

	result = FlattenShift([]Shift{})
	if len(result) > 0 {
		t.Errorf("Unexpected result: %v", result)
	}
}

func TestFilterToCurrent(t *testing.T) {
	result := FilterToCurrent(shs)
	if result[0].DateStart != now {
		t.Errorf("Unexpected result: %v", result)
	}

	if result[0].DateEnd != now+86400 {
		t.Errorf("Unexpected result: %v", result)
	}

	if result[1].DateStart != now {
		t.Errorf("Unexpected result: %v", result)
	}

	if result[1].DateEnd != now+83600 {
		t.Errorf("Unexpected result: %v", result)
	}
}

var sh = Shift{
	ServiceID: "5d90c2192e96c977b76822ee",
	DateStart: now - 86400,
	DateEnd:   now + 86400,
	Resp: Resp{
		Order:    0,
		Username: "uplink",
	},
}

var srsIntelapce = []Shift{
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 83600,
		DateEnd:   now - 43200,
		Resp: Resp{
			Order:    1,
			Username: "andgein",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 39600,
		DateEnd:   now + 7200,
		Resp: Resp{
			Order:    0,
			Username: "megaeee",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now,
		DateEnd:   now + 83600,
		Resp: Resp{
			Order:    1,
			Username: "andgein",
		},
	},
}

var expectedInterlapse = []Shift{
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 86400,
		DateEnd:   now - 83600,
		Resp: Resp{
			Order:    0,
			Username: "uplink",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 83600,
		DateEnd:   now - 43200,
		Resp: Resp{
			Order:    0,
			Username: "andgein",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 43200,
		DateEnd:   now - 39600,
		Resp: Resp{
			Order:    0,
			Username: "uplink",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now - 39600,
		DateEnd:   now,
		Resp: Resp{
			Order:    0,
			Username: "megaeee",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now,
		DateEnd:   now + 83600,
		Resp: Resp{
			Order:    0,
			Username: "andgein",
		},
	},
	{
		ServiceID: "5d90c2192e96c977b76822ee",
		DateStart: now + 83600,
		DateEnd:   now + 86400,
		Resp: Resp{
			Order:    0,
			Username: "uplink",
		},
	},
}

func TestApplyReplaces(t *testing.T) {
	result := ApplyReplaces(&sh, srsIntelapce)
	for i := range result {
		if result[i] != expectedInterlapse[i] {
			t.Errorf("Unexpected result: %v", result)
		}
	}
}
