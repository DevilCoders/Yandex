package rule

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestHasKey(t *testing.T) {
	testCases := []struct {
		name        string
		val         interface{}
		keys        []interface{}
		expectedErr error
	}{
		{
			name:        "has_key",
			val:         map[string]string{"shimba": "boomba"},
			keys:        []interface{}{"shimba"},
			expectedErr: nil,
		},
		{
			name:        "has_keys",
			val:         map[string]string{"shimba": "boomba"},
			keys:        []interface{}{"looken", "shimba"},
			expectedErr: nil,
		},
		{
			name:        "has_no_keys",
			val:         map[string]string{"shimba": "boomba"},
			keys:        []interface{}{"looken", "chicken"},
			expectedErr: ErrUnexpected,
		},
		{
			name:        "non_map",
			val:         []string{"shimba"},
			keys:        []interface{}{"shimba"},
			expectedErr: fmt.Errorf("slice: %w", ErrInvalidType),
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			v := inspection.Inspect(tc.val)
			err := HasKey(tc.keys...)(v)
			assert.Equal(t, tc.expectedErr, err)
		})
	}
}

func TestHasKey_Panics(t *testing.T) {
	assert.Panics(t, func() {
		v := inspection.Inspect(map[string]string{"shimba": "boomba"})
		_ = HasKey(42)(v)
	})
}

func BenchmarkHasKey(b *testing.B) {
	testCases := []*inspection.Inspected{
		inspection.Inspect(map[string]string{"shimba": "boomba"}),
		inspection.Inspect(map[string]string{"looken": "tooken"}),
		inspection.Inspect(map[string]string{"chicken": "cooken"}),
	}

	b.Run("one_key", func(b *testing.B) {
		r := HasKey("shimba")

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})

	b.Run("two_keys", func(b *testing.B) {
		r := HasKey("chicken", "shimba")

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})

	b.Run("three_keys", func(b *testing.B) {
		r := HasKey("chicken", "ololo", "shimba")

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = r(testCases[i%len(testCases)])
		}
	})
}
