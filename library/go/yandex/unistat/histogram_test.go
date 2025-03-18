package unistat

import (
	"encoding/json"
	"sync"
	"testing"
)

var aggrHist = StructuredAggregation{
	AggregationType: Delta,
	Group:           Hgram,
	MetaGroup:       Hgram,
	Rollup:          Hgram,
}

func TestHistogram(t *testing.T) {
	h := NewHistogram("my_histogram", 10, aggrHist, []float64{0, 10, 100, 1000})
	for i := 1; i < 10; i++ {
		b := float64(i)
		h.Update(b + 0.1)
		h.Update(b*10 + 0.1)
		h.Update(b*100 + 0.1)
		h.Update(b*1000 + 0.1)
	}

	b, err := json.Marshal(h)
	if err != nil {
		t.Fatal("Marshal histogram:", err)
	}

	t.Logf("Histogram JSON: %s", b)

	jsonHist := [2]json.RawMessage{}
	if err := json.Unmarshal(b, &jsonHist); err != nil {
		t.Fatal("Unmarshal histogram:", err)
	}

	var name string
	if err := json.Unmarshal(jsonHist[0], &name); err != nil {
		t.Fatalf("Unmarshal histogram name %s: %s", jsonHist[0], err)
	}
	if name != "my_histogram_dhhh" {
		t.Error("Unexpected histogram name:", name)
	}

	buckets := [][2]float64{}
	if err := json.Unmarshal(jsonHist[1], &buckets); err != nil {
		t.Fatalf("Unmarshal buckets %s: %s", jsonHist[1], err)
	}

	for i := range buckets {
		if buckets[i][1] != 9 {
			t.Errorf("Unexpected bucket %f value: %f", buckets[i][0], buckets[i][1])
		}
	}
}

func TestHistogramParallel(t *testing.T) {
	m := NewHistogram("my_histogram", 10, aggrHist, []float64{0, 10, 100, 1000})

	numThreads := 1000
	var wg sync.WaitGroup
	wg.Add(numThreads)
	for tr := 0; tr < numThreads; tr++ {
		go func() {
			for i := 1; i < 10; i++ {
				b := float64(i)
				m.Update(b + 0.1)
				m.Update(b*10 + 0.1)
				m.Update(b*100 + 0.1)
				m.Update(b*1000 + 0.1)
			}
			wg.Done()
		}()
	}
	wg.Wait()

	for i := range m.weights {
		if m.weights[i] != int64(9*numThreads) {
			t.Errorf("Unexpected bucket %d value: %d", i, m.weights[i])
		}
	}
	if m.GetSize() != int64(4*9*numThreads) {
		t.Errorf("Unexpected histogram size: %d, expected: %d", m.GetSize(), 4*9*numThreads)
	}
}

func BenchmarkHistogramUpdate(b *testing.B) {
	intervals := []float64{}
	for i := 0; i < 50; i++ { // Max buckets count
		intervals = append(intervals, float64(10*i))
	}
	h := NewHistogram("my_histogram", 10, aggrHist, intervals)

	b.StartTimer()
	for i := 0; i < b.N; i++ {
		h.Update(0) // worst case
	}
}
