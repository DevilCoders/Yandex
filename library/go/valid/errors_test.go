package valid_test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

func TestErrorsError(t *testing.T) {
	testCases := []struct {
		name     string
		errs     valid.Errors
		expected string
	}{
		{"no_errors", valid.Errors(nil), ""},
		{"one_error", valid.Errors{valid.ErrEmptyString}, "empty string given"},
		{"multiple_errors", valid.Errors{valid.ErrEmptyString, valid.ErrInvalidPrefix}, "empty string given; invalid prefix"},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, tc.errs.Error())
		})
	}
}

func TestErrorsString(t *testing.T) {
	testCases := []struct {
		name     string
		errs     valid.Errors
		expected string
	}{
		{"no_errors", valid.Errors(nil), ""},
		{"one_error", valid.Errors{valid.ErrEmptyString}, "empty string given"},
		{"multiple_errors", valid.Errors{valid.ErrEmptyString, valid.ErrInvalidPrefix}, "empty string given\ninvalid prefix"},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, tc.errs.String())
		})
	}
}

func TestErrorsHas(t *testing.T) {
	testCases := []struct {
		name     string
		errs     valid.Errors
		err      error
		expected bool
	}{
		{"empty_errors", valid.Errors(nil), valid.ErrEmptyString, false},
		{"has_ErrEmptyString", valid.Errors{valid.ErrEmptyString}, valid.ErrEmptyString, true},
		{"has_wrapped_ErrEmptyString", valid.Errors{valid.ErrValidation.Wrap(valid.ErrEmptyString)}, valid.ErrEmptyString, true},
		{"has_not_ErrEmptyString", valid.Errors{valid.ErrInvalidPrefix}, valid.ErrEmptyString, false},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, tc.errs.Has(tc.err))
		})
	}
}

func BenchmarkErrorsString(b *testing.B) {
	benchCases := []valid.Errors{
		valid.Errors(nil),
		{valid.ErrEmptyString},
		{valid.ErrEmptyString, valid.ErrInvalidPrefix},
		{valid.ErrEmptyString, valid.ErrInvalidPrefix, valid.ErrBadParams},
		{valid.ErrEmptyString, valid.ErrInvalidPrefix, valid.ErrBadParams, valid.ErrEmptyDataPart},
		{valid.ErrEmptyString, valid.ErrInvalidPrefix, valid.ErrBadParams, valid.ErrEmptyDataPart, valid.ErrInvalidISBN},
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = benchCases[i%len(benchCases)].String()
	}
}
