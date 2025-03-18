package rule

import (
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

func TestIsAbs(t *testing.T) {
	sep := string(filepath.Separator)

	testCases := []struct {
		value       string
		expectedErr error
	}{
		{sep + "test", nil},
		{sep + "test" + sep, nil},
		{"|test", ErrPatternMismatch},
		{"test" + sep, ErrPatternMismatch},
		{"test", ErrPatternMismatch},
	}

	for _, tc := range testCases {
		t.Run(tc.value, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			err := IsAbs(v)
			assert.Equal(t, tc.expectedErr, err)
		})
	}
}

func TestIsAbsDir(t *testing.T) {
	sep := string(filepath.Separator)

	testCases := []struct {
		value       string
		expectedErr error
	}{
		{sep + "test", ErrPatternMismatch},
		{sep + "test" + sep, nil},
		{"|test", ErrPatternMismatch},
		{"test" + sep, ErrPatternMismatch},
		{"test", ErrPatternMismatch},
	}

	for _, tc := range testCases {
		t.Run(tc.value, func(t *testing.T) {
			v := inspection.Inspect(tc.value)
			err := IsAbsDir(v)
			assert.Equal(t, tc.expectedErr, err)
		})
	}
}

func BenchmarkPath(b *testing.B) {
	sep := string(filepath.Separator)

	testCases := []*inspection.Inspected{
		inspection.Inspect(sep + "test"),
		inspection.Inspect(sep + "test" + sep),
		inspection.Inspect("|test"),
		inspection.Inspect("|test|"),
		inspection.Inspect("test" + sep),
		inspection.Inspect("test"),
	}

	b.Run("IsAbs", func(b *testing.B) {
		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = IsAbs(testCases[i%len(testCases)])
		}
	})

	b.Run("IsAbsDir", func(b *testing.B) {
		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			_ = IsAbsDir(testCases[i%len(testCases)])
		}
	})
}
