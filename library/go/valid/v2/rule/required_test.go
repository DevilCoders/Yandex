package rule

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestRequired(t *testing.T) {
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

		{"empty_string", "", ErrRequired},
		{"empty_pointer", (*int)(nil), ErrRequired},
		{"empty_int8", int8(0), ErrRequired},
		{"empty_interface", interface{}(nil), ErrRequired},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expected, Required(v))
		})
	}
}
