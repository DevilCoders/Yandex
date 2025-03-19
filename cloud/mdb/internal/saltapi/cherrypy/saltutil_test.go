package cherrypy

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
)

func TestParseRunningFuncResponse(t *testing.T) {
	var data = []struct {
		Name     string
		Input    map[string]json.RawMessage
		Expected map[string][]saltapi.RunningFunc
		Error    bool
	}{
		{
			Name:     "empty",
			Input:    map[string]json.RawMessage{},
			Expected: map[string][]saltapi.RunningFunc{},
		},
		{
			Name: "value",
			Input: map[string]json.RawMessage{
				"value": []byte("[{\"fun\": \"state.highstate\"}]"),
			},
			Expected: map[string][]saltapi.RunningFunc{
				"value": {
					{Function: "state.highstate"},
				},
			},
		},
		{
			Name: "empty_slice",
			Input: map[string]json.RawMessage{
				"value": []byte("[]"),
			},
			Expected: map[string][]saltapi.RunningFunc{
				"value": {},
			},
		},
		{
			Name: "invalid",
			Input: map[string]json.RawMessage{
				"value": []byte("{}"),
			},
			Error: true,
		},
	}

	for _, d := range data {
		t.Run(d.Name, func(t *testing.T) {
			res, err := parseRunningFuncResponse(d.Input)
			if d.Error {
				assert.Error(t, err)
				return
			}

			assert.NoError(t, err)
			assert.Equal(t, d.Expected, res)
		})
	}
}
