package unistat

import (
	"encoding/json"
	"sync"
	"testing"
)

var aggrSignle = StructuredAggregation{
	AggregationType: Delta,
	Group:           Sum,
	MetaGroup:       Sum,
	Rollup:          Sum,
}

func TestNumeric(t *testing.T) {
	counter := NewNumeric("my_counter", 1, aggrSignle, Sum)
	for i := 0; i < 1000; i++ {
		counter.Update(1)
	}

	b, err := json.Marshal(counter)
	if err != nil {
		t.Fatal("Marshal single:", err)
	}

	t.Logf("Single JSON: %s", b)

	jsonMetric := [2]json.RawMessage{}
	if err := json.Unmarshal(b, &jsonMetric); err != nil {
		t.Fatal("Unmarshal single:", err)
	}

	var name string
	if err := json.Unmarshal(jsonMetric[0], &name); err != nil {
		t.Fatalf("Unmarshal single name %s: %s", jsonMetric[0], err)
	}
	if name != "my_counter_dmmm" {
		t.Error("Unexpected metric name:", name)
	}

	var value float64
	if err := json.Unmarshal(jsonMetric[1], &value); err != nil {
		t.Fatalf("Unmarshal value %s: %s", jsonMetric[1], err)
	}

	if value != 1000 {
		t.Error("Unexpected value:", value)
	}
}

func TestNumericParallel(t *testing.T) {
	counter := NewNumeric("my_counter", 1, aggrSignle, Sum)

	numThreads := 1000
	var wg sync.WaitGroup
	wg.Add(numThreads)
	for tr := 0; tr < numThreads; tr++ {
		go func() {
			for i := 0; i < 10; i++ {
				counter.Update(1)
			}
			wg.Done()
		}()
	}
	wg.Wait()

	if counter.GetValue() != float64(10*numThreads) {
		t.Error("Unexpected value:", counter.GetValue())
	}
}
