package unistat

import (
	"encoding/json"
	"fmt"
	"math/rand"
	"sort"
	"strings"
	"sync"
	"testing"
)

func TestMarshaling(t *testing.T) {
	registry := NewRegistry()
	registry.Register(&testMetric{name: "a", priority: 10})
	registry.Register(&testMetric{name: "b", priority: 100})
	registry.Register(&testMetric{name: "d", priority: 1})
	registry.Register(&testMetric{name: "c", priority: 1})

	b, err := json.Marshal(registry)
	if err != nil {
		t.Fatal("Marshal error:", err)
	}

	t.Logf("JSON unistat report: %s", b)

	report := []string{}
	if err := json.Unmarshal(b, &report); err != nil {
		t.Fatal("Unmarshal error:", err)
	}

	if len(report) != 4 {
		t.Fatal("Unexpected report:", report)
	}
	if strings.Join(report, ",") != strings.Join([]string{"b", "a", "c", "d"}, ",") {
		t.Errorf("Unexpected metrics order: %s", report)
	}

}

func TestDefaultRegistry(t *testing.T) {
	m := &testMetric{name: "test"}
	Register(m)

	bytes, err := MarshalJSON()
	if err != nil {
		t.Fatal("Marshal:", err)
	}

	v := []string{}
	if err = json.Unmarshal(bytes, &v); err != nil {
		t.Fatal("Unmarshal error:", err)
	}
	if len(v) != 1 || v[0] != "test" {
		t.Fatalf("Unexpected report: %s", bytes)
	}
}

func reverseInts(ints []int) {
	for i := 0; i < len(ints)/2; i++ {
		j := len(ints) - i - 1
		ints[i], ints[j] = ints[j], ints[i]
	}
}

func isEqualInts(a, b []int) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if a[i] != b[i] {
			return false
		}
	}

	return true
}

func TestReverse(t *testing.T) {
	a := []int{}
	reverseInts(a)

	a = []int{1, 2, 3}
	reverseInts(a)
	if !isEqualInts(a, []int{3, 2, 1}) {
		t.Error("Unexpected reverse:", a)
	}

	a = []int{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
	reverseInts(a)
	reverseInts(a)
	if isEqualInts(a, []int{1, 2, 3, 4, 5, 6, 7, 8, 9}) {
		t.Error("Unexpected reverse:", a)
	}
}

func TestPriorityOrder(t *testing.T) {
	reg := NewRegistry()
	for i := 0; i < 10; i++ {
		m := &testMetric{name: fmt.Sprintf("%d", i), priority: Priority(rand.Int())}
		reg.Register(m)
	}

	if _, err := reg.MarshalJSON(); err != nil {
		t.Fatal("Marshal:", err)
	}

	regImpl := reg.(*registry)
	var ints []int
	for i := range regImpl.metrics {
		ints = append(ints, int(regImpl.metrics[i].Priority()))
	}

	reverseInts(ints)
	if !sort.IntsAreSorted(ints) {
		t.Error("Expected sorted desc by priority:", ints)
	}
}

func TestPriorityOrderByName(t *testing.T) {
	reg := NewRegistry()
	for i := 0; i < 10; i++ {
		m := &testMetric{name: fmt.Sprintf("%d-%d", rand.Int63(), i), priority: 1}
		reg.Register(m)
	}

	if _, err := reg.MarshalJSON(); err != nil {
		t.Fatal("Marshal:", err)
	}

	regImpl := reg.(*registry)
	var strings []string
	for i := range regImpl.metrics {
		strings = append(strings, regImpl.metrics[i].Name())
	}

	if !sort.StringsAreSorted(strings) {
		t.Error("Expected sorted by name:", strings)
	}
}

func TestNameCollision(t *testing.T) {
	defer func() {
		err := recover()
		if err != ErrDuplicate {
			t.Fatal("Expected duplicate error:", err)
		}
	}()

	registry := NewRegistry()
	registry.Register(&testMetric{name: "aba"})
	registry.Register(&testMetric{name: "aba"})
}

func TestRegisterMarshalParallel(t *testing.T) {
	reg := NewRegistry()

	var wg sync.WaitGroup
	wg.Add(1000)
	for i := 0; i < 1000; i++ {
		go func(i int) {
			m := &testMetric{name: fmt.Sprintf("%d", i)}
			reg.Register(m)
			if _, err := reg.MarshalJSON(); err != nil {
				t.Log("Marshal:", err)
				t.Fail()
			}
			wg.Done()
		}(i)
	}
	wg.Wait()

	regImpl := reg.(*registry)
	if len(regImpl.metrics) != 1000 {
		t.Error("Unexpected metrics count:", len(regImpl.metrics))
	}
}

func TestEmptyRegistry(t *testing.T) {
	reg := NewRegistry()
	b, err := reg.MarshalJSON()
	if err != nil {
		t.Fatal("Marshal:", err)
	}

	var v []string
	if err := json.Unmarshal(b, &v); err != nil {
		t.Fatal("Unmarshal:", err)
	}

	if len(v) != 0 || v == nil {
		t.Fatalf("Unexpected report %s (%s)", v, b)
	}
}

func BenchmarkRegister(b *testing.B) {
	registry := NewRegistry()
	for i := 0; i < b.N; i++ {
		m := &testMetric{name: fmt.Sprintf("%d", i), priority: Priority(rand.Int())}
		registry.Register(m)
	}
}

func BenchmarkMarshal(b *testing.B) {
	registry := NewRegistry()
	for i := 0; i < 100; i++ {
		m := &testMetric{name: fmt.Sprintf("%d", i), priority: Priority(rand.Int())}
		registry.Register(m)
	}

	b.StartTimer()
	for i := 0; i < b.N; i++ {
		if _, err := registry.MarshalJSON(); err != nil {
			b.Fatal("Marshal:", err)
		}
	}
}
