package decimal

import (
	"math/big"
	"strings"
	"sync"
)

var bigIntPool = sync.Pool{
	New: func() interface{} {
		x := &big.Int{}
		// Warm up allocated Int
		return x.SetBits([]big.Word{0, 0, 0, 0, 0, 0, 0, 0})
	},
}

func getInt() *big.Int {
	return bigIntPool.Get().(*big.Int)
}

func putInt(v *big.Int) {
	v.SetInt64(0)
	bigIntPool.Put(v)
}

var bigRatPool = sync.Pool{
	New: func() interface{} {
		return &big.Rat{}
	},
}

func getRat() *big.Rat {
	return bigRatPool.Get().(*big.Rat)
}

func putRat(v *big.Rat) {
	v.SetInt64(0)
	bigRatPool.Put(v)
}

var stringsReaderPool = sync.Pool{
	New: func() interface{} {
		return &strings.Reader{}
	},
}

func getStringsReader() *strings.Reader {
	return stringsReaderPool.Get().(*strings.Reader)
}

func putStringsReader(v *strings.Reader) {
	stringsReaderPool.Put(v)
}
