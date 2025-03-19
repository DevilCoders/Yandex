package agent

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

const jsonHbase = `{
    "regions": 2,
    "requests": 5,
    "averageLoad": 2,
    "LiveNodes": [
        {
            "name": "hdp-d0.internal:16020",
            "startCode": 1552474989367,
            "requests": 5,
            "heapSizeMB": 22,
            "maxHeapSizeMB": 1221
        }
    ],
    "DeadNodes": [
		{
            "name": "hdp-d1.internal:16020",
            "startCode": 1552474989367,
            "requests": 6,
            "heapSizeMB": 21,
            "maxHeapSizeMB": 1231
        }
	]
}`

func Test_parseHbaseResponse(t *testing.T) {
	want := models.HbaseInfo{
		Available:   true,
		Regions:     2,
		Requests:    5,
		AverageLoad: 2,
		LiveNodes: []models.HbaseNodeInfo{
			{Name: "hdp-d0.internal", Requests: 5, HeapSizeMB: 22, MaxHeapSizeMB: 1221},
		},
		DeadNodes: []models.HbaseNodeInfo{
			{Name: "hdp-d1.internal", Requests: 6, HeapSizeMB: 21, MaxHeapSizeMB: 1231},
		},
	}

	got, err := parseHbaseResponse([]byte(jsonHbase))
	require.NoError(t, err)
	require.Equal(t, want, got)
}
