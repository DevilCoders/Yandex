package directreader

import (
	"errors"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

func TestMap(t *testing.T) {
	a := assert.New(t)
	goodMap := `Offset          Length          Mapped to       File
0               0x100000        0x200000        not used
0x100000        0x200000        0x600000        not used
0x800000        0x200000        0x300000        not used
0xc00000        0x300000        0x800000        not used
0x1000000       0x100000        0x500000        not used
`
	created, err := FromHuman([]byte(goodMap), "some map")
	a.NoError(err)
	offsets := []int64{
		2097152, 6291456, 3145728, 8388608, 5242880,
	}
	a.Equal(BlocksMap{items: []imageMapItem{
		{
			Start:          0,
			Length:         1048576,
			Data:           true,
			RawImageOffset: &offsets[0],
		},
		{
			Start:          1048576,
			Length:         2097152,
			Data:           true,
			RawImageOffset: &offsets[1],
		},
		{
			Start:          8388608,
			Length:         2097152,
			Data:           true,
			RawImageOffset: &offsets[2],
		},
		{
			Start:          12582912,
			Length:         3145728,
			Data:           true,
			RawImageOffset: &offsets[3],
		},
		{
			Start:          16777216,
			Length:         1048576,
			Data:           true,
			RawImageOffset: &offsets[4],
		},
	}, url: "some map"}, created)
}

func TestMapNotCreated(t *testing.T) {
	badMap := `Offset          Length          Mapped to       File
qemu-img: File contains external, encrypted or compressed clusters.
`
	a := assert.New(t)
	_, err := FromHuman([]byte(badMap), "")
	a.True(errors.Is(err, misc.ErrIncorrectMap), err)
}

func TestMakeCompact(t *testing.T) {
	a := assert.New(t)
	offsets := []int64{
		100, 110, 111,
	}
	m := BlocksMap{items: []imageMapItem{
		{Data: true, Zero: false, Start: 0, Length: 10, RawImageOffset: &offsets[0]},
		{Data: true, Zero: false, Start: 10, Length: 10, RawImageOffset: &offsets[1]},
		{Data: true, Zero: true, Start: 20, Length: 10},
		{Data: true, Zero: true, Start: 30, Length: 10},
		{Data: true, Zero: true, Start: 40, Length: 10},
		{Data: false, Zero: true, Start: 50, Length: 10},
		{Data: true, Zero: true, Start: 60, Length: 10},
		{Data: true, Zero: true, Start: 80, Length: 10},
		{Data: true, Zero: false, Start: 90, Length: 10, RawImageOffset: &offsets[0]},
		{Data: true, Zero: false, Start: 100, Length: 10, RawImageOffset: &offsets[2]},
	}}

	res := BlocksMap{items: []imageMapItem{
		{Data: true, Zero: false, Start: 0, Length: 20, RawImageOffset: &offsets[0]},
		{Data: true, Zero: true, Start: 20, Length: 30},
		{Data: false, Zero: true, Start: 50, Length: 10},
		{Data: true, Zero: true, Start: 60, Length: 10},
		{Data: true, Zero: true, Start: 80, Length: 10},
		{Data: true, Zero: false, Start: 90, Length: 10, RawImageOffset: &offsets[0]},
		{Data: true, Zero: false, Start: 100, Length: 10, RawImageOffset: &offsets[2]},
	}}
	m.MergeAdjacentBlocks()
	a.Equal(res, m)
}
