package directreader

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

func TestSequenceURLImageReader_CanReadDirect(t *testing.T) {
	var r *SequenceURLImageReader
	a := assert.New(t)

	r = NewSequenceURLImageReader(BlocksMap{items: []imageMapItem{}}, 4*misc.MB, nil)
	a.True(r.CanReadDirect())

	p := func(v int64) *int64 {
		return &v
	}

	r = NewSequenceURLImageReader(BlocksMap{items: []imageMapItem{
		{},
	}}, 4*misc.MB, nil)
	a.True(r.CanReadDirect())

	r = NewSequenceURLImageReader(BlocksMap{items: []imageMapItem{
		{Zero: true},
	}}, 4*misc.MB, nil)
	a.True(r.CanReadDirect())

	r = NewSequenceURLImageReader(BlocksMap{items: []imageMapItem{
		{Data: true, RawImageOffset: p(1)},
	}}, 4*misc.MB, nil)
	a.True(r.CanReadDirect())

	r = NewSequenceURLImageReader(BlocksMap{items: []imageMapItem{
		{Data: true, Zero: true, RawImageOffset: nil},
	}}, 4*misc.MB, nil)
	a.True(r.CanReadDirect())

	r = NewSequenceURLImageReader(BlocksMap{items: []imageMapItem{
		{Data: true, RawImageOffset: nil},
	}}, 4*misc.MB, nil)
	a.False(r.CanReadDirect())
}
