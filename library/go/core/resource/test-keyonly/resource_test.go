package test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/resource"
)

func TestResource(t *testing.T) {
	assert.Equal(t, []byte("bar"), resource.Get("foo"))
	assert.Equal(t, []byte("baz"), resource.Get("bar"))
}
