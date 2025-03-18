package rule

import (
	"fmt"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestUnique(t *testing.T) {
	testCases := []struct {
		name        string
		value       interface{}
		keyFunc     func(v *inspection.Inspected) string
		expectedErr error
	}{
		{
			name:  "valid_slice",
			value: []string{"1", "2", "3"},
			keyFunc: func(v *inspection.Inspected) string {
				return v.Interface.(string)
			},
			expectedErr: nil,
		},
		{
			name:  "invalid_slice",
			value: []string{"1", "2", "2"},
			keyFunc: func(v *inspection.Inspected) string {
				return v.Interface.(string)
			},
			expectedErr: Errors{
				fmt.Errorf("2: %w", ErrDuplicateValue),
			},
		},
		{
			name:  "valid_array",
			value: [3]string{"1", "2", "3"},
			keyFunc: func(v *inspection.Inspected) string {
				return v.Interface.(string)
			},
			expectedErr: nil,
		},
		{
			name:  "invalid_array",
			value: [3]string{"1", "4", "4"},
			keyFunc: func(v *inspection.Inspected) string {
				return v.Interface.(string)
			},
			expectedErr: Errors{
				fmt.Errorf("4: %w", ErrDuplicateValue),
			},
		},
		{
			name:  "valid_map",
			value: map[string]string{"1": "xx", "2": "yy", "3": "zz"},
			keyFunc: func(v *inspection.Inspected) string {
				return v.Interface.(string)
			},
			expectedErr: nil,
		},
		{
			name:  "invalid_slice",
			value: map[string]string{"1": "xx", "2": "yy", "3": "zz"},
			keyFunc: func(v *inspection.Inspected) string {
				return strings.Replace(v.Interface.(string), "y", "z", -1)

			},
			expectedErr: Errors{
				fmt.Errorf("zz: %w", ErrDuplicateValue),
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			err := Unique(tc.keyFunc)(v)
			assert.Equal(t, tc.expectedErr, err)
		})
	}
}
