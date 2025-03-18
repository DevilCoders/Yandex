package unistat

import (
	"encoding/json"
	"testing"
	"time"
)

type testMetric struct {
	name     string
	priority Priority
	value    float64
}

func (m *testMetric) Name() string {
	return m.name
}

func (m *testMetric) Priority() Priority {
	return m.priority
}

func (m *testMetric) Update(v float64) {
	m.value = v
}

func (m *testMetric) Aggregation() Aggregation { return StructuredAggregation{} }

func (m *testMetric) MarshalJSON() ([]byte, error) {
	return json.Marshal(m.name)
}

func TestMeasureMicrosecondsSince(t *testing.T) {
	sinceFunc := func(time.Time) time.Duration {
		return time.Millisecond
	}

	m := &testMetric{}
	measureMicrosecondsSince(sinceFunc, m, time.Time{})
	if m.value != 1000 {
		t.Error("Unexpected duration:", m.value)
	}
}
