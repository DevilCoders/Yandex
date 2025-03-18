package rule

import (
	"fmt"
	"reflect"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestInRange(t *testing.T) {
	testCases := []struct {
		name        string
		value       interface{}
		min, max    float64
		expectedErr error
	}{
		{"valid_int", int(42), 0, 100, nil},
		{"valid_int8", int8(42), 0, 100, nil},
		{"valid_int16", int16(42), 0, 100, nil},
		{"valid_int32", int32(42), 0, 100, nil},
		{"valid_int64", int64(42), 0, 100, nil},
		{"valid_uint", uint(42), 0, 100, nil},
		{"valid_uint8", uint8(42), 0, 100, nil},
		{"valid_uint16", uint16(42), 0, 100, nil},
		{"valid_uint32", uint32(42), 0, 100, nil},
		{"valid_uint64", uint64(42), 0, 100, nil},
		{"valid_float32", float32(4.2), 0, 10, nil},
		{"valid_float64", float64(4.2), 0, 10, nil},

		{"invalid_int", int(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_int8", int8(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_int16", int16(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_int32", int32(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_int64", int64(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_uint", uint(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_uint8", uint8(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_uint16", uint16(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_uint32", uint32(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_uint64", uint64(42), 0, 10, fmt.Errorf("value must be between 0 and 10: %w", ErrOutOfRange)},
		{"invalid_float32", float32(4.2), 0.55, 0.7, fmt.Errorf("value must be between 0.55 and 0.7: %w", ErrOutOfRange)},
		{"invalid_float64", float64(4.2), 0.55, 0.7, fmt.Errorf("value must be between 0.55 and 0.7: %w", ErrOutOfRange)},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expectedErr, InRange(tc.min, tc.max)(v))
		})
	}
}

func TestLesserOrEqual(t *testing.T) {
	testCases := []struct {
		name        string
		value       interface{}
		lim         float64
		expectedErr error
	}{
		{"lesser", 42, 100, nil},
		{"equal", 4.2, 4.2, nil},
		{"greater", 100, 4.2, fmt.Errorf("value must be between -Inf and 4.2: %w", ErrOutOfRange)},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expectedErr, LesserOrEqual(tc.lim)(v))
		})
	}
}

func TestGreaterOrEqual(t *testing.T) {
	testCases := []struct {
		name        string
		value       interface{}
		lim         float64
		expectedErr error
	}{
		{"greater", 100, 4.2, nil},
		{"equal", 4.2, 4.2, nil},
		{"lesser", 42, 100, fmt.Errorf("value must be between 100 and +Inf: %w", ErrOutOfRange)},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expectedErr, GreaterOrEqual(tc.lim)(v))
		})
	}
}

func TestIsPositive(t *testing.T) {
	testCases := []struct {
		name        string
		value       interface{}
		expectedErr error
	}{
		{"positive_int", int(42), nil},
		{"positive_int8", int8(42), nil},
		{"positive_int16", int16(42), nil},
		{"positive_int32", int32(42), nil},
		{"positive_int64", int64(42), nil},
		{"positive_uint", uint(42), nil},
		{"positive_uint8", uint8(42), nil},
		{"positive_uint16", uint16(42), nil},
		{"positive_uint32", uint32(42), nil},
		{"positive_uint64", uint64(42), nil},
		{"positive_float32", float32(42), nil},
		{"positive_float64", float64(42), nil},
		{"positive_bool", true, nil},
		{"positive_duration", 42 * time.Second, nil},

		{"negative_int", int(-42), ErrNegativeValue},
		{"negative_int8", int8(-42), ErrNegativeValue},
		{"negative_int16", int16(-42), ErrNegativeValue},
		{"negative_int32", int32(-42), ErrNegativeValue},
		{"negative_int64", int64(-42), ErrNegativeValue},
		{"negative_float32", float32(-42), ErrNegativeValue},
		{"negative_float64", float64(-42), ErrNegativeValue},
		{"negative_bool", false, ErrNegativeValue},
		{"positive_duration", -42 * time.Second, ErrNegativeValue},

		{"invalid_type", "ololo", fmt.Errorf("%v: %w", reflect.String, ErrInvalidType)},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expectedErr, IsPositive(v))
		})
	}

}

func BenchmarkIsPositive(b *testing.B) {
	testCases := []*inspection.Inspected{
		inspection.Inspect(int(42)),
		inspection.Inspect(int8(42)),
		inspection.Inspect(int16(42)),
		inspection.Inspect(int32(42)),
		inspection.Inspect(int64(42)),
		inspection.Inspect(uint(42)),
		inspection.Inspect(uint8(42)),
		inspection.Inspect(uint16(42)),
		inspection.Inspect(uint32(42)),
		inspection.Inspect(uint64(42)),
		inspection.Inspect(float32(42)),
		inspection.Inspect(float64(42)),
		inspection.Inspect(true),
		inspection.Inspect(42 * time.Second),
		inspection.Inspect(int(-42)),
		inspection.Inspect(int8(-42)),
		inspection.Inspect(int16(-42)),
		inspection.Inspect(int32(-42)),
		inspection.Inspect(int64(-42)),
		inspection.Inspect(float32(-42)),
		inspection.Inspect(float64(-42)),
		inspection.Inspect(false),
		inspection.Inspect(-42 * time.Second),
		inspection.Inspect("ololo"),
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = IsPositive(testCases[i%len(testCases)])
	}
}

func BenchmarkRange(b *testing.B) {
	testCases := []*inspection.Inspected{
		inspection.Inspect(int(42)),
		inspection.Inspect(int8(42)),
		inspection.Inspect(int16(42)),
		inspection.Inspect(int32(42)),
		inspection.Inspect(int64(42)),
		inspection.Inspect(uint(42)),
		inspection.Inspect(uint8(42)),
		inspection.Inspect(uint16(42)),
		inspection.Inspect(uint32(42)),
		inspection.Inspect(uint64(42)),
		inspection.Inspect(float32(42)),
		inspection.Inspect(float64(42)),
		inspection.Inspect(true),
		inspection.Inspect(42 * time.Second),
		inspection.Inspect(int(-42)),
		inspection.Inspect(int8(-42)),
		inspection.Inspect(int16(-42)),
		inspection.Inspect(int32(-42)),
		inspection.Inspect(int64(-42)),
		inspection.Inspect(float32(-42)),
		inspection.Inspect(float64(-42)),
		inspection.Inspect(false),
		inspection.Inspect(-42 * time.Second),
		inspection.Inspect("ololo"),
	}

	b.Run("in_range", func(b *testing.B) {
		r := InRange(0, 100)

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})

	b.Run("lesser_or_equal", func(b *testing.B) {
		r := LesserOrEqual(10)

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})

	b.Run("greater_or_equal", func(b *testing.B) {
		r := LesserOrEqual(10)

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})
}
