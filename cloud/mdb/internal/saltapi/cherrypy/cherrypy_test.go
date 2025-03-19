package cherrypy

import (
	"encoding/json"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestCalcTokenRenewPeriod(t *testing.T) {
	for d := time.Second * 0; d <= time.Hour*24; d = nextStep(d) {
		now := time.Now()
		renew := calcTokenRenewPeriod(now, now.Add(d))

		if d <= time.Minute {
			assert.Equal(t, time.Second, renew, "d = %s", d)
		} else if d <= time.Hour {
			assert.Equal(t, d/2, renew, "d = %s", d)
		} else {
			assert.Equal(t, d-d/20, renew, "d = %s", d)
		}
	}
}

func nextStep(d time.Duration) time.Duration {
	// Check every second up to and including 1 minute 1 second
	if d <= time.Minute+time.Second {
		return d + time.Second
	}

	// Check every 30 seconds
	return d + time.Second*30
}

func TestParseMinionsList(t *testing.T) {
	var data = []struct {
		Name     string
		Input    json.RawMessage
		Expected map[string]json.RawMessage
		Fail     bool
	}{
		{
			Name:     "empty",
			Input:    []byte("{}"),
			Expected: map[string]json.RawMessage{},
		},
		{
			Name:     "boolean",
			Input:    []byte("{\"boolean\": false}"),
			Expected: map[string]json.RawMessage{},
		},
		{
			Name:  "value",
			Input: []byte("{\"value\": {\"key\": 123}}"),
			Expected: map[string]json.RawMessage{
				"value": []byte("{\"key\": 123}"),
			},
		},
		{
			Name:  "both",
			Input: []byte("{\"boolean\": false, \"value\": {\"key\": 123}}"),
			Expected: map[string]json.RawMessage{
				"value": []byte("{\"key\": 123}"),
			},
		},
		{
			Name:  "invalid",
			Input: []byte("[]"),
			Fail:  true,
		},
	}

	for _, d := range data {
		t.Run(d.Name, func(t *testing.T) {
			res, err := parseMinionsList(d.Input)
			if d.Fail {
				assert.Error(t, err)
				return
			}

			assert.NoError(t, err)
			assert.Equal(t, d.Expected, res)
		})
	}
}
