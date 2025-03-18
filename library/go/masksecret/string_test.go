package masksecret

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestString(t *testing.T) {
	testCases := []struct {
		name     string
		input    string
		expected string
	}{
		{
			"short_string",
			"olol",
			"xxxx",
		},
		{
			"long_string",
			"OLOLO_TROLOLO",
			"OxxxxxxxxxxxO",
		},
		{
			"ascii_string",
			"SHIMBA_BOOMBA",
			"SxxxxxxxxxxxA",
		},
		{
			"utf_string",
			"Здравствуйте дорогой Мартин Алексеич",
			"Зxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxч",
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, String(tc.input))
		})
	}
}
