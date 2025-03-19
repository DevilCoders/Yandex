package agent

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

const jmxHive = `{
	"beans" : [ {
	  "name" : "metrics:name=hs2_active_sessions",
	  "Value" : 3
	}, {
	  "name" : "metrics:name=hs2_succeeded_queries",
	  "Count" : 111
	}, {
	  "name" : "metrics:name=hs2_failed_queries",
	  "Count" : 1
	}, {
	  "name" : "metrics:name=hs2_open_sessions",
	  "Value" : NaN
	} ]
}`

func Test_parseHiveResponse(t *testing.T) {
	want := models.HiveInfo{
		Available:        true,
		QueriesSucceeded: 111,
		QueriesFailed:    1,
		SessionsActive:   3,
		SessionsOpen:     0,
	}

	got, err := parseHiveResponse([]byte(jmxHive))
	require.NoError(t, err)
	require.Equal(t, want, got)
}
