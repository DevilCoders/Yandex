package decimal

import (
	"encoding/binary"
	"math/big"
)

// get uint128 value of internal decimal buffer like [16]byte
func (d *Decimal128) bytes() (buf [byteLen]byte) {
	putUintsToBytesBigEndian(d.abs[:], buf[:])
	return
}

// set internal decimal buffer from [16]byte
func (d *Decimal128) setBytes(buf [byteLen]byte) {
	putBytesToUintsBigEndian(buf[:], d.abs[:])
}

// big endian copy uints to bytes
func putUintsToBytesBigEndian(u []uint64, b []byte) {
	binary.BigEndian.PutUint64(b[0:8], u[1])
	binary.BigEndian.PutUint64(b[8:16], u[0])
}

// little endian copy uints to bytes
func putUintsToBytesLittleIndian(u []uint64, b []byte) {
	binary.LittleEndian.PutUint64(b[0:8], u[0])
	binary.LittleEndian.PutUint64(b[8:16], u[1])
}

// big endian copy bytes to uints
func putBytesToUintsBigEndian(b []byte, u []uint64) {
	u[1] = binary.BigEndian.Uint64(b[0:8])
	u[0] = binary.BigEndian.Uint64(b[8:16])
}

// little endian copy bytes to uints
func putBytesToUintsLittleEndian(b []byte, u []uint64) {
	u[0] = binary.LittleEndian.Uint64(b[0:8])
	u[1] = binary.LittleEndian.Uint64(b[8:16])
}

// special case - put unscaled decimal buffer to big.Int and let it work for us
func (d Decimal128) fillBigInt(i *big.Int) *big.Int {
	buf := d.bytes()
	i.SetBytes(buf[:])
	if d.neg {
		i = i.Neg(i)
	}
	return i
}
