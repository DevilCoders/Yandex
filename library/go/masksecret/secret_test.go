package masksecret

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func Test_makePlaceholder(t *testing.T) {
	testCases := []struct {
		r        rune
		size     int
		expected string
	}{
		{'x', 1, "x"},
		{'x', 2, "xx"},
		{'x', 3, "xxx"},
		{'x', 4, "xxxx"},
		{'x', 5, "xxxxx"},

		{'*', 1, "*"},
		{'*', 2, "**"},
		{'*', 3, "***"},
		{'*', 4, "****"},
		{'*', 5, "*****"},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			assert.Equal(t, tc.expected, makePlaceholder(tc.r, tc.size))
		})
	}
}

func Test_makePlaceholderRunes(t *testing.T) {
	testCases := []struct {
		r        rune
		size     int
		expected []rune
	}{
		{'x', 1, []rune("x")},
		{'x', 2, []rune("xx")},
		{'x', 3, []rune("xxx")},
		{'x', 4, []rune("xxxx")},
		{'x', 5, []rune("xxxxx")},

		{'*', 1, []rune("*")},
		{'*', 2, []rune("**")},
		{'*', 3, []rune("***")},
		{'*', 4, []rune("****")},
		{'*', 5, []rune("*****")},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			assert.Equal(t, tc.expected, makePlaceholderRunes(tc.r, tc.size))
		})
	}
}
