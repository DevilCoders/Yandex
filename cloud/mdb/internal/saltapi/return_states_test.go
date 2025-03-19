package saltapi

import (
	"encoding/json"
	"strings"
	"testing"
	"unicode"

	"github.com/stretchr/testify/assert"
)

func TestAbs(t *testing.T) {

	var obj ReturnStatesMap = make(map[string]*StateReturn)
	obj["A"] = &StateReturn{
		RunNum: 20,
		Name:   "AAA",
	}
	obj["B"] = &StateReturn{
		RunNum: 10,
		Name:   "BBB",
	}
	obj["C"] = &StateReturn{
		RunNum: 2,
		Name:   "CCC",
	}

	expected := stripSpaces(`
	{
		"C":{
			"name":"CCC",
			"id":"",
			"result":false,
			"sls":"",
			"changes":null,
			"comment":"",
			"duration":"0s",
			"start_ts":"0001-01-01T00:00:00Z",
			"finish_ts":"0001-01-01T00:00:00Z",
			"start_time":"",
			"run_num":2
		},
		"B":{
			"name":"BBB",
			"id":"",
			"result":false,
			"sls":"",
			"changes":null,
			"comment":"",
			"duration":"0s",
			"start_ts":"0001-01-01T00:00:00Z",
			"finish_ts":"0001-01-01T00:00:00Z",
			"start_time":"",
			"run_num":10
		},
		"A":{
			"name":"AAA",
			"id":"",
			"result":false,
			"sls":"",
			"changes":null,
			"comment":"",
			"duration":"0s",
			"start_ts":"0001-01-01T00:00:00Z",
			"finish_ts":"0001-01-01T00:00:00Z",
			"start_time":"",
			"run_num":20
		}
	}`)

	data, err := json.Marshal(obj)
	if err != nil {
		t.Errorf("Failed to encode ReturnStatesMap: %v", err)
		return
	}
	str := string(data)
	assert.Equal(t, expected, str)

}

// https://stackoverflow.com/a/32082217
func stripSpaces(str string) string {
	return strings.Map(func(r rune) rune {
		if unicode.IsSpace(r) {
			// if the character is a space, drop it
			return -1
		}
		// else keep it in the string
		return r
	}, str)
}
