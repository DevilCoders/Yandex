package test

import (
	"io/ioutil"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/resource"
)

func TestResource(t *testing.T) {
	assert.Equal(t, []byte("hello world"), resource.Get("resfs/file/testdata/a.txt"))

	assert.Equal(t, []byte("library/go/core/resource/test-files/testdata/a.txt"), resource.Get("resfs/src/resfs/file/testdata/a.txt"))
	assert.Equal(t, []byte("library/go/core/resource/test-files/testdata/b.bin"), resource.Get("resfs/src/resfs/file/testdata/b.bin"))

	bindata, err := ioutil.ReadFile("testdata/b.bin")
	assert.NoError(t, err)
	assert.Equal(t, bindata, resource.Get("resfs/file/testdata/b.bin"))

	assert.Equal(t, []byte("handle this"), resource.Get("resfs/file/my_data/testdata/collision.txt"))

	assert.Equal(t, []byte("library/go/core/resource/test-files/testdata/collision.txt"), resource.Get("resfs/src/resfs/file/my_data/testdata/collision.txt"))
}
