package valid_test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

func TestAlphanumeric(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrEmptyString},
		{"\n", valid.ErrInvalidCharacters},
		{"\r", valid.ErrInvalidCharacters},
		{"Ⅸ", valid.ErrInvalidCharacters},
		{"   fooo   ", valid.ErrInvalidCharacters},
		{"abc!!!", valid.ErrInvalidCharacters},
		{"abc〩", valid.ErrInvalidCharacters},
		{"소주", valid.ErrInvalidCharacters},
		{"소aBC", valid.ErrInvalidCharacters},
		{"소", valid.ErrInvalidCharacters},
		{"달기&Co.", valid.ErrInvalidCharacters},
		{"〩Hours", valid.ErrInvalidCharacters},
		{"\ufff0", valid.ErrInvalidCharacters},

		{"\u0026", valid.ErrInvalidCharacters}, // UTF-8(ASCII): &
		{"-00123", valid.ErrInvalidCharacters},
		{"-0", valid.ErrInvalidCharacters},
		{"123.123", valid.ErrInvalidCharacters},
		{" ", valid.ErrInvalidCharacters},
		{".", valid.ErrInvalidCharacters},
		{"-1¾", valid.ErrInvalidCharacters},
		{"1¾", valid.ErrInvalidCharacters},
		{"〥〩", valid.ErrInvalidCharacters},
		{"모자", valid.ErrInvalidCharacters},
		{"۳۵۶۰", valid.ErrInvalidCharacters},
		{"1--", valid.ErrInvalidCharacters},
		{"1-1", valid.ErrInvalidCharacters},
		{"-", valid.ErrInvalidCharacters},
		{"--", valid.ErrInvalidCharacters},
		{"1++", valid.ErrInvalidCharacters},
		{"1+1", valid.ErrInvalidCharacters},
		{"+", valid.ErrInvalidCharacters},
		{"++", valid.ErrInvalidCharacters},
		{"+1", valid.ErrInvalidCharacters},

		{"abc", nil},
		{"abc123", nil},
		{"ABC111", nil},
		{"abc1", nil},
		{"ABC", nil},
		{"FoObAr", nil},
		{"ix", nil},
		{"0", nil},
		{"\u0030", nil}, // UTF-8(ASCII): 0
		{"123", nil},
		{"0123", nil},
		{"\u0070", nil}, // UTF-8(ASCII): p
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.Alphanumeric(tc.param))
		})
	}
}

func TestAlpha(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrEmptyString},
		{"\n", valid.ErrInvalidCharacters},
		{"\r", valid.ErrInvalidCharacters},
		{"Ⅸ", valid.ErrInvalidCharacters},
		{"   fooo   ", valid.ErrInvalidCharacters},
		{"abc!!!", valid.ErrInvalidCharacters},
		{"abc1", valid.ErrInvalidCharacters},
		{"abc〩", valid.ErrInvalidCharacters},
		{"소주", valid.ErrInvalidCharacters},
		{"소aBC", valid.ErrInvalidCharacters},
		{"소", valid.ErrInvalidCharacters},
		{"달기&Co.", valid.ErrInvalidCharacters},
		{"〩Hours", valid.ErrInvalidCharacters},
		{"\ufff0", valid.ErrInvalidCharacters},
		{"\u0026", valid.ErrInvalidCharacters}, // UTF-8(ASCII): &
		{"\u0030", valid.ErrInvalidCharacters}, // UTF-8(ASCII): 0
		{"123", valid.ErrInvalidCharacters},
		{"0123", valid.ErrInvalidCharacters},
		{"-00123", valid.ErrInvalidCharacters},
		{"0", valid.ErrInvalidCharacters},
		{"-0", valid.ErrInvalidCharacters},
		{"123.123", valid.ErrInvalidCharacters},
		{" ", valid.ErrInvalidCharacters},
		{".", valid.ErrInvalidCharacters},
		{"-1¾", valid.ErrInvalidCharacters},
		{"1¾", valid.ErrInvalidCharacters},
		{"〥〩", valid.ErrInvalidCharacters},
		{"모자", valid.ErrInvalidCharacters},
		{"۳۵۶۰", valid.ErrInvalidCharacters},
		{"1--", valid.ErrInvalidCharacters},
		{"1-1", valid.ErrInvalidCharacters},
		{"-", valid.ErrInvalidCharacters},
		{"--", valid.ErrInvalidCharacters},
		{"1++", valid.ErrInvalidCharacters},
		{"1+1", valid.ErrInvalidCharacters},
		{"+", valid.ErrInvalidCharacters},
		{"++", valid.ErrInvalidCharacters},
		{"+1", valid.ErrInvalidCharacters},

		{"ix", nil},
		{"\u0070", nil}, // UTF-8(ASCII): p
		{"ABC", nil},
		{"FoObAr", nil},
		{"abc", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.Alpha(tc.param))
		})
	}
}

func TestNumeric(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrEmptyString},
		{"\n", valid.ErrInvalidCharacters},
		{"\r", valid.ErrInvalidCharacters},
		{"Ⅸ", valid.ErrInvalidCharacters},
		{"   fooo   ", valid.ErrInvalidCharacters},
		{"abc!!!", valid.ErrInvalidCharacters},
		{"abc1", valid.ErrInvalidCharacters},
		{"abc〩", valid.ErrInvalidCharacters},
		{"abc", valid.ErrInvalidCharacters},
		{"소주", valid.ErrInvalidCharacters},
		{"ABC", valid.ErrInvalidCharacters},
		{"FoObAr", valid.ErrInvalidCharacters},
		{"소aBC", valid.ErrInvalidCharacters},
		{"소", valid.ErrInvalidCharacters},
		{"달기&Co.", valid.ErrInvalidCharacters},
		{"〩Hours", valid.ErrInvalidCharacters},
		{"\ufff0", valid.ErrInvalidCharacters},
		{"\u0070", valid.ErrInvalidCharacters}, // UTF-8(ASCII): p
		{"\u0026", valid.ErrInvalidCharacters}, // UTF-8(ASCII): &
		{"\u0030", nil},                        // UTF-8(ASCII): 0
		{"-00123", valid.ErrInvalidCharacters},
		{"+00123", valid.ErrInvalidCharacters},
		{"-0", valid.ErrInvalidCharacters},
		{"123.123", valid.ErrInvalidCharacters},
		{" ", valid.ErrInvalidCharacters},
		{".", valid.ErrInvalidCharacters},
		{"12𐅪3", valid.ErrInvalidCharacters},
		{"-1¾", valid.ErrInvalidCharacters},
		{"1¾", valid.ErrInvalidCharacters},
		{"〥〩", valid.ErrInvalidCharacters},
		{"모자", valid.ErrInvalidCharacters},
		{"ix", valid.ErrInvalidCharacters},
		{"۳۵۶۰", valid.ErrInvalidCharacters},
		{"1--", valid.ErrInvalidCharacters},
		{"1-1", valid.ErrInvalidCharacters},
		{"-", valid.ErrInvalidCharacters},
		{"--", valid.ErrInvalidCharacters},
		{"1++", valid.ErrInvalidCharacters},
		{"1+1", valid.ErrInvalidCharacters},
		{"+", valid.ErrInvalidCharacters},
		{"++", valid.ErrInvalidCharacters},
		{"+1", valid.ErrInvalidCharacters},

		{"0", nil},
		{"123", nil},
		{"0123", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.Numeric(tc.param))
		})
	}
}

func TestDouble(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrEmptyString},
		{".", valid.ErrBadFormat},
		{"ololo", valid.ErrInvalidCharacters},
		{".0", valid.ErrBadFormat},
		{"01234.0", valid.ErrBadFormat},

		{"0", nil},
		{"1234", nil},
		{"0.0001", nil},
		{"1234.00", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.Double(tc.param))
		})
	}
}

func TestHexColor(t *testing.T) {
	testCases := []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrStringTooShort},
		{"#ff", valid.ErrInvalidStringLength},
		{"fff0", valid.ErrInvalidStringLength},
		{"#ff12FG", valid.ErrInvalidCharacters},

		{"CCccCC", nil},
		{"fff", nil},
		{"#f00", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.HexColor(tc.param))
		})
	}
}

func TestStringLen(t *testing.T) {
	testCases := []struct {
		str       string
		min       int
		max       int
		expectErr error
	}{
		{"anything", 5, 2, valid.ErrBadParams},
		{"", 2, 5, valid.ErrStringTooShort},
		{"a", 2, 5, valid.ErrStringTooShort},
		{"ab", 2, 5, nil},
		{"abc", 2, 5, nil},
		{"abcd", 2, 5, nil},
		{"abcde", 2, 5, nil},
		{"abcdef", 2, 5, valid.ErrStringTooLong},
		{"abcdefg", 2, 5, valid.ErrStringTooLong},

		{"just_min", 9, 0, valid.ErrStringTooShort},
		{"just_min", 5, 0, nil},

		{"just_max", 0, 5, valid.ErrStringTooLong},
		{"just_min", 0, 10, nil},
	}
	for _, tc := range testCases {
		t.Run(tc.str, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.StringLen(tc.str, tc.min, tc.max))
		})
	}
}
