package exporter

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/generator"
)

type testMetric struct {
	Data string `json:"data"`
}

func (tm testMetric) Marshal() ([]byte, error) {
	return json.Marshal(tm)
}

func TestYCMetricsBatcher_Marshal(t *testing.T) {
	t.Run("test", func(t *testing.T) {
		mb, err := NewYCMetricsBatcher(generator.NewSequentialGenerator(1))
		require.NoError(t, err)
		mb.Add(testMetric{Data: "val1"})
		mb.Add(testMetric{Data: "val2"})

		bytes, err := mb.Marshal()
		require.NoError(t, err)
		expected := []byte(`{"data":"val1"}
{"data":"val2"}
`)
		require.Equal(t, expected, bytes)
		require.Equal(t, mb.ID(), "1")

		mb.Add(testMetric{Data: "val3"})
		bytes, err = mb.Marshal()
		require.NoError(t, err)
		expected = []byte(`{"data":"val1"}
{"data":"val2"}
{"data":"val3"}
`)
		require.Equal(t, expected, bytes)
		require.Equal(t, mb.ID(), "1")

		require.NoError(t, mb.Reset())

		mb.Add(testMetric{Data: "val10"})
		mb.Add(testMetric{Data: "val11"})
		bytes, err = mb.Marshal()
		require.NoError(t, err)
		expected = []byte(`{"data":"val10"}
{"data":"val11"}
`)
		require.Equal(t, expected, bytes)
		require.Equal(t, mb.ID(), "2")
	})

}
