package masksecret

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestPhone(t *testing.T) {
	testCases := []struct {
		name     string
		input    string
		expected string
	}{
		{
			"empty",
			"",
			"",
		},
		{
			"ultra_short_internal_number",
			"13-29",
			"xx-xx",
		},
		{
			"short_internal_number",
			"2-13-29",
			"2-xx-xx",
		},
		{
			"local_number",
			"123-45-67",
			"123-xx-xx",
		},
		{
			"local_number_numeric",
			"1234567",
			"123xxxx",
		},
		{
			"international",
			"+7 (495) 123-45-67",
			"+7 (495) 123-xx-xx",
		},
		{
			"international_numeric",
			"74951234567",
			"7495123xxxx",
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, Phone(tc.input))
		})
	}
}
