package deployutils

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestDeployAPIURL(t *testing.T) {
	data := []struct {
		input    string
		expected string
	}{
		{"test.ru", "https://test.ru"},
		{" test.ru ", "https://test.ru"},
		{"", ""},
		{" ", ""},
	}
	for _, d := range data {
		t.Run(fmt.Sprintf("Host_%s", d.input), func(t *testing.T) {
			url, err := parseDeployAPIURL(d.input)
			if d.expected == "" {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, d.expected, url)
			}
		})
	}
}

func TestDeployVersion(t *testing.T) {
	data := []struct {
		input    string
		expected int
		fail     bool
	}{
		{"-1", -1, false},
		{"0", 0, false},
		{"1", 1, false},
		{"2", 2, false},
		{"-1\n", -1, false},
		{"0\n", 0, false},
		{"1\n", 1, false},
		{"2\n", 2, false},
		{" -1 ", -1, false},
		{" 0 ", 0, false},
		{" 1 ", 1, false},
		{" 2 ", 2, false},
		{"", 0, true},
		{"1a", 0, true},
		{"-1a", 0, true},
		{"a1", 0, true},
		{"a1", 0, true},
	}
	for _, d := range data {
		t.Run(fmt.Sprintf("Version_%s", d.input), func(t *testing.T) {
			v, err := parseDeployVersion(d.input)
			if d.fail {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, d.expected, v)
			}
		})
	}
}
