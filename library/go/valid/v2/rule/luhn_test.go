package rule

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestLuhn(t *testing.T) {
	var testCases = []struct {
		value     string
		expectErr error
	}{
		{"1111111111", ErrInvalidChecksum},
		{"7992739871", ErrInvalidChecksum},
		{"4222222222222222", ErrInvalidChecksum},
		{"49927398717", ErrInvalidChecksum},
		{"1234567812345678", ErrInvalidChecksum},

		{"4276380091945522", nil},
		{"356938035643809", nil},
		{"49927398716", nil},
		{"1111111116", nil},
		{"12345674", nil},
		{"5515805738324655", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.value, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			assert.Equal(t, tc.expectErr, Luhn(v))
		})
	}
}

func BenchmarkLuhn(b *testing.B) {
	benchCases := []*inspection.Inspected{
		inspection.Inspect("1111111111"),
		inspection.Inspect("7992739871"),
		inspection.Inspect("4222222222222222"),
		inspection.Inspect("49927398717"),
		inspection.Inspect("1234567812345678"),
		inspection.Inspect("4276380091945522"),
		inspection.Inspect("356938035643809"),
		inspection.Inspect("49927398716"),
		inspection.Inspect("1111111116"),
		inspection.Inspect("12345674"),
		inspection.Inspect("5515805738324655"),
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = Luhn(benchCases[i%len(benchCases)])
	}
}
