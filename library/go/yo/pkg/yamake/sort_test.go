package yamake

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestSortRecurse(t *testing.T) {
	yaMake := &YaMake{Recurse: []string{
		"one",
		"# comment for two",
		"two",
		"# first comment for three",
		"# second comment for three",
		"three",
		"# last comment",
	}}
	SortRecurse(yaMake)
	assert.Equal(t, []string{
		"one",
		"# first comment for three",
		"# second comment for three",
		"three",
		"# comment for two",
		"two",
		"# last comment",
	}, yaMake.Recurse)
}
