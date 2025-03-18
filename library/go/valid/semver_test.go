package valid_test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

func TestSemver(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"1.2.3", nil},
		{"v1.2.3", nil},
		{"1.0", valid.ErrStringTooShort},
		{"v1.0", valid.ErrStringTooShort},
		{"1", valid.ErrStringTooShort},
		{"v1", valid.ErrStringTooShort},
		{"1.2.beta", valid.ErrNonNumericVersion},
		{"v1.2.beta", valid.ErrNonNumericVersion},
		{"foo", valid.ErrStringTooShort},
		{"1.2-5", valid.ErrTooFewSemverParts},
		{"v1.2-5", valid.ErrTooFewSemverParts},
		{"1.2-beta.5", valid.ErrNonNumericVersion},
		{"v1.2-beta.5", valid.ErrNonNumericVersion},
		{"\n1.2", valid.ErrStringTooShort},
		{"\nv1.2", valid.ErrTooFewSemverParts},
		{"1.2.3.4", valid.ErrNonNumericVersion},
		{"v1.2.3.4", valid.ErrNonNumericVersion},
		{"1.2.0-x.Y.0+metadata", nil},
		{"v1.2.0-x.Y.0+metadata", nil},
		{"1.2.0-x.Y.0+metadata-width-hypen", nil},
		{"v1.2.0-x.Y.0+metadata-width-hypen", nil},
		{"1.2.3-rc1-with-hypen", nil},
		{"v1.2.3-rc1-with-hypen", nil},
		{"1.2.2147483648", nil},
		{"1.2147483648.3", nil},
		{"2147483648.3.0", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.Semver(tc.param))
		})
	}
}

func BenchmarkSemver(b *testing.B) {
	benchCases := []string{
		"1.2.3",
		"v1.2.3",
		"1.0",
		"v1.0",
		"1",
		"v1",
		"1.2.beta",
		"v1.2.beta",
		"foo",
		"1.2-5",
		"v1.2-5",
		"1.2-beta.5",
		"v1.2-beta.5",
		"\n1.2",
		"\nv1.2",
		"1.2.0-x.Y.0+metadata",
		"v1.2.0-x.Y.0+metadata",
		"1.2.0-x.Y.0+metadata-width-hypen",
		"v1.2.0-x.Y.0+metadata-width-hypen",
		"1.2.3-rc1-with-hypen",
		"v1.2.3-rc1-with-hypen",
		"1.2.3.4",
		"v1.2.3.4",
		"1.2.2147483648",
		"1.2147483648.3",
		"2147483648.3.0",
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = valid.Semver(benchCases[i%len(benchCases)])
	}
}
