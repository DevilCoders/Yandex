package rule

import (
	"fmt"
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestIsKind(t *testing.T) {
	testCases := []struct {
		name        string
		value       interface{}
		kinds       []reflect.Kind
		expectedErr error
	}{
		{"is_string", "ololo", []reflect.Kind{reflect.String}, nil},
		{"one_is_string", "ololo", []reflect.Kind{reflect.String, reflect.Float64}, nil},
		{"not_string", 42, []reflect.Kind{reflect.String}, fmt.Errorf("%v: %w", reflect.Int, ErrInvalidType)},
		{"not_anything", "ololo", []reflect.Kind{reflect.Int, reflect.Float64}, fmt.Errorf("%v: %w", reflect.String, ErrInvalidType)},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expectedErr, IsKind(tc.kinds...)(v))
		})
	}
}
