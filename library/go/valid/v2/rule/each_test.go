package rule

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestEach(t *testing.T) {
	testCases := []struct {
		name        string
		value       interface{}
		expectedErr error
	}{
		{"invalid_type", int64(42), nil},
		{"valid_slice", []string{"1", "2", "3"}, nil},
		{"invalid_slice", []string{"1", "ololo", "3"}, Errors{ErrInvalidCharacters}},
		{"valid_array", [3]string{"1", "2", "3"}, nil},
		{"invalid_array", [3]string{"1", "ololo", "3"}, Errors{ErrInvalidCharacters}},
		{"valid_map", map[string]string{"1": "1", "2": "2", "3": "3"}, nil},
		{"invalid_map", map[string]string{"1": "1", "2": "ololo", "3": "3"}, Errors{ErrInvalidCharacters}},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expectedErr, Each(IsNumeric)(v))
		})
	}
}
