package rule

import (
	"fmt"
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestLen(t *testing.T) {
	testCases := []struct {
		name      string
		param     interface{}
		min, max  int
		expectErr error
	}{
		{"invalid_params", "ololo", 4, 2, ErrBadParams},
		{"invalid_type", int64(42), 0, 100, fmt.Errorf("%v: %w", reflect.Int64, ErrInvalidType)},

		{"valid_string", "ololo", 0, 10, nil},
		{"overflow_string", "ololo", 0, 3, ErrInvalidLength},
		{"endless_string", "shimba", 0, -1, nil},
		{"underflow_string", "shimba", 20, 40, ErrInvalidLength},

		{"valid_slice", []string{"123"}, 0, 10, nil},
		{"overflow_slice", []string{"1", "2", "3", "4"}, 0, 3, ErrInvalidLength},
		{"endless_slice", []string{"1", "2", "3", "4"}, 0, -1, nil},
		{"underflow_slice", []string{"1", "2", "3", "4"}, 20, 40, ErrInvalidLength},

		{"valid_array", [1]string{"123"}, 0, 10, nil},
		{"overflow_array", [4]string{"1", "2", "3", "4"}, 0, 3, ErrInvalidLength},
		{"endless_array", [4]string{"1", "2", "3", "4"}, 0, -1, nil},
		{"underflow_array", [4]string{"1", "2", "3", "4"}, 20, 40, ErrInvalidLength},

		{"valid_map", map[string]string{"123": "456"}, 0, 10, nil},
		{"overflow_map", map[string]string{"1": "1", "2": "2", "3": "3", "4": "4"}, 0, 3, ErrInvalidLength},
		{"endless_map", map[string]string{"1": "1", "2": "2", "3": "3", "4": "4"}, 0, -1, nil},
		{"underflow_map", map[string]string{"1": "1", "2": "2", "3": "3", "4": "4"}, 20, 40, ErrInvalidLength},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.param)
			assert.Equal(t, tc.expectErr, Len(tc.min, tc.max)(v))
		})
	}
}

func BenchmarkLen(b *testing.B) {
	testCases := []*inspection.Inspected{
		inspection.Inspect("ololo"),
		inspection.Inspect(int64(42)),
		inspection.Inspect("ololo"),
		inspection.Inspect("shimba"),
		inspection.Inspect([]string{"123"}),
		inspection.Inspect([]string{"1", "2", "3", "4"}),
		inspection.Inspect([1]string{"123"}),
		inspection.Inspect([4]string{"1", "2", "3", "4"}),
		inspection.Inspect(map[string]string{"123": "456"}),
		inspection.Inspect(map[string]string{"1": "1", "2": "2", "3": "3", "4": "4"}),
	}

	r := Len(2, 5)

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = r(testCases[i%len(testCases)])
	}
}
