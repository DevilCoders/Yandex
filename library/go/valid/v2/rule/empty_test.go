package rule

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestNotEmpty(t *testing.T) {
	intVal := 1

	testCases := []struct {
		name     string
		value    interface{}
		expected error
	}{
		{"string", "shimba", nil},
		{"pointer", &intVal, nil},
		{"int8", int8(1), nil},
		{"interface", interface{}(42), nil},
		{"slice", []string{"test"}, nil},
		{"map", map[string]string{"test": "test"}, nil},

		{"empty_string", "", ErrEmptyValue},
		{"empty_pointer", (*int)(nil), ErrEmptyValue},
		{"empty_int8", int8(0), ErrEmptyValue},
		{"empty_interface", interface{}(nil), ErrEmptyValue},
		{"nil_slice", []string(nil), ErrEmptyValue},
		{"nil_map", map[string]string(nil), ErrEmptyValue},
		{"empty_slice", []string{}, ErrEmptyValue},
		{"empty_map", map[string]string{}, ErrEmptyValue},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expected, NotEmpty(v))
		})
	}
}

func TestOmitEmpty(t *testing.T) {
	testCases := []struct {
		name     string
		value    interface{}
		expected error
	}{
		{"empty", "", nil},
		{"non_empty", "ololo", Errors{ErrInvalidCharacters}},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expected, OmitEmpty(IsNumeric)(v))
		})
	}
}

func BenchmarkOmitEmpty(b *testing.B) {
	testCases := []*inspection.Inspected{
		inspection.Inspect(""),
		inspection.Inspect(([]string)(nil)),
		inspection.Inspect((map[string]string)(nil)),
		inspection.Inspect(0),
		inspection.Inspect(0.0),
		inspection.Inspect("ololo"),
		inspection.Inspect([]string{}),
		inspection.Inspect(map[string]string{}),
		inspection.Inspect(42),
		inspection.Inspect(4.2),
	}

	r := OmitEmpty(IsNumeric)

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = r(testCases[i%len(testCases)])
	}
}
