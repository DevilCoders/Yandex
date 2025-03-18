package rule

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestInSlice(t *testing.T) {
	intVal := 42

	testCases := []struct {
		name        string
		slice       interface{}
		value       interface{}
		expectedErr error
	}{
		{"non_slice", 42, "shimba", fmt.Errorf("slice of string expected: %w", ErrInvalidType)},
		{"mismatch_slice_type", []string{"shimba"}, 42, ErrUnexpected},
		{"not_in", []string{"shimba"}, "boomba", ErrUnexpected},
		{"in", []string{"shimba"}, "shimba", nil},
		{"ptr_in", []int{intVal}, &intVal, nil},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			err := InSlice(tc.slice)(v)
			assert.Equal(t, tc.expectedErr, err)
		})
	}
}

func TestNotInSlice(t *testing.T) {
	intVal := 42

	testCases := []struct {
		name        string
		slice       interface{}
		value       interface{}
		expectedErr error
	}{
		{"non_slice", 42, "shimba", fmt.Errorf("slice of string expected: %w", ErrInvalidType)},
		{"mismatch_slice_type", []string{"shimba"}, 42, nil},
		{"not_in", []string{"shimba"}, "boomba", nil},
		{"in", []string{"shimba"}, "shimba", ErrExpected},
		{"ptr_in", []int{intVal}, &intVal, ErrExpected},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			err := NotInSlice(tc.slice)(v)
			assert.Equal(t, tc.expectedErr, err)
		})
	}
}

func BenchmarkSlice(b *testing.B) {
	testCases := []*inspection.Inspected{
		inspection.Inspect("shimba"),
		inspection.Inspect("boomba"),
		inspection.Inspect(42),
		inspection.Inspect(true),
	}

	b.Run("InSlice", func(b *testing.B) {
		slice := []string{"shimba", "chicken"}
		r := InSlice(slice)

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})

	b.Run("NotInSlice", func(b *testing.B) {
		slice := []string{"shimba", "chicken"}
		r := NotInSlice(slice)

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})
}
