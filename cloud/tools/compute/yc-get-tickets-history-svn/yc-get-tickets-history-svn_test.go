package main

import (
	"testing"

	"github.com/magiconair/properties/assert"
)

func TestGetRevisions(t *testing.T) {
	var first, last, err = getRevisions("1..2")
	assert.Equal(t, first, 1)
	assert.Equal(t, last, 2)
	assert.Equal(t, err, nil)

	first, last, err = getRevisions("1..")
	assert.Equal(t, first, 1)
	assert.Equal(t, last, 0)
	assert.Equal(t, err, nil)

	first, last, err = getRevisions("..123")
	assert.Equal(t, first, 0)
	assert.Equal(t, last, 123)
	assert.Equal(t, err, nil)
}
