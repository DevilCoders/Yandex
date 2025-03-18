package rule

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestSemver(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"1.0", ErrStringTooShort},
		{"v1.0", ErrStringTooShort},
		{"1", ErrStringTooShort},
		{"v1", ErrStringTooShort},
		{"1.2.beta", ErrInvalidCharsSequence},
		{"v1.2.beta", ErrInvalidCharsSequence},
		{"foo", ErrStringTooShort},
		{"1.2-5", ErrInvalidCharsSequence},
		{"v1.2-5", ErrInvalidCharsSequence},
		{"1.2-beta.5", ErrInvalidCharsSequence},
		{"v1.2-beta.5", ErrInvalidCharsSequence},
		{"\n1.2", ErrStringTooShort},
		{"\nv1.2", ErrInvalidCharsSequence},
		{"1.2.3.4", ErrInvalidCharsSequence},
		{"v1.2.3.4", ErrInvalidCharsSequence},
		{"01.2.3", ErrInvalidCharsSequence},
		{"1.02.3", ErrInvalidCharsSequence},
		{"1.2.03", ErrInvalidCharsSequence},
		{"1.2.03+meta-test", ErrInvalidCharsSequence},
		{"1.2.03+-pre", ErrInvalidCharsSequence},
		{"1.2.03-+meta", ErrInvalidCharsSequence},
		{"1.2.0-01.1.0+metadata", ErrInvalidCharsSequence},
		{"1.2.0-1.01.0+metadata", ErrInvalidCharsSequence},
		{"1.2.0-1.1.05+metadata", ErrInvalidCharsSequence},

		{"1.0.3", nil},
		{"1.2.0", nil},
		{"1.2.0-1.1.5+metadata", nil},
		{"1.2.0-x.Y.0+metadata", nil},
		{"0.2.3", nil},
		{"v1.2.0-x.Y.0+metadata", nil},
		{"v1.2.3", nil},
		{"1.2.0-x.Y.0+metadata-width-hypen", nil},
		{"1.2.3", nil},
		{"v1.2.0-x.Y.0+metadata-width-hypen", nil},
		{"1.2.3-rc1-with-hypen", nil},
		{"v1.2.3-rc1-with-hypen", nil},
		{"1.2.2147483648", nil},
		{"1.2147483648.3", nil},
		{"2147483648.3.0", nil},
	}

	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			v := inspection.Inspect(tc.param)
			assert.Equal(t, tc.expectErr, Semver(v))
		})
	}
}

func BenchmarkSemver(b *testing.B) {
	benchCases := []*inspection.Inspected{
		inspection.Inspect("1.2.3"),
		inspection.Inspect("v1.2.3"),
		inspection.Inspect("1.0"),
		inspection.Inspect("v1.0"),
		inspection.Inspect("1"),
		inspection.Inspect("v1"),
		inspection.Inspect("1.2.beta"),
		inspection.Inspect("v1.2.beta"),
		inspection.Inspect("foo"),
		inspection.Inspect("1.2-5"),
		inspection.Inspect("v1.2-5"),
		inspection.Inspect("1.2-beta.5"),
		inspection.Inspect("v1.2-beta.5"),
		inspection.Inspect("\n1.2"),
		inspection.Inspect("\nv1.2"),
		inspection.Inspect("1.2.0-x.Y.0+metadata"),
		inspection.Inspect("v1.2.0-x.Y.0+metadata"),
		inspection.Inspect("1.2.0-x.Y.0+metadata-width-hypen"),
		inspection.Inspect("v1.2.0-x.Y.0+metadata-width-hypen"),
		inspection.Inspect("1.2.3-rc1-with-hypen"),
		inspection.Inspect("v1.2.3-rc1-with-hypen"),
		inspection.Inspect("1.2.3.4"),
		inspection.Inspect("v1.2.3.4"),
		inspection.Inspect("1.2.2147483648"),
		inspection.Inspect("1.2147483648.3"),
		inspection.Inspect("2147483648.3.0"),
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = Semver(benchCases[i%len(benchCases)])
	}
}
