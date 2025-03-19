package qcow2

import (
	"bytes"
	"encoding/binary"
)

// MAGIC qemu QCow(2) qcow2Magic ("QFI\xfb").
var qcow2Magic = []byte{0x51, 0x46, 0x49, 0xFB}

type version uint32

const (
	// version2 qcow2 image format version2.
	version2 version = 2
	// version3 qcow2 image format version3.
	version3 version = 3
)

const (
	// version2HeaderSize is the image header at the beginning of the file.
	version2HeaderSize = 72
	// version3HeaderSize is directly following the v2 header, up to 104.
	version3HeaderSize = 104
)

// cryptMethod represents a whether encrypted qcow2 image.
// 0 for no enccyption
// 1 for AES encryption
type cryptMethod uint32

const (
	// cryptNone no encryption.
	cryptNone cryptMethod = iota
	// cryptAes AES encryption. Not supported.
	cryptAes

	maxCryptClusters = 32
	maxSnapshots     = 65536
)

var (
	l1OffsetMask = uint64(0x00fffffffffffe00)
	l2OffsetMask = uint64(0x00fffffffffffe00)

	qcow2CompressedSectorSize = uint64(512)
	qcowOflagCompressed       = uint64(1 << 62)
	qcowOflagZero             = uint64(1 << 0)
)

type qCow2ClusterType uint64

const (
	qcow2ClusterUnallocated qCow2ClusterType = iota
	qcow2ClusterZeroPlain
	qcow2ClusterZeroAlloc
	qcow2ClusterNormal
	qcow2ClusterCompressed
)

type qCow2SubclusterType uint64

const (
	qcow2SubclusterUnallocatedPlain qCow2SubclusterType = iota
	qcow2SubclusterUnallocatedAlloc
	qcow2SubclusterZeroPlain
	qcow2SubclusterZeroAlloc
	qcow2SubclusterNormal
	qcow2SubclusterCompressed
	qcow2SubclusterInvalid
)

type qlusterStatus uint64

const (
	clusterData qlusterStatus = iota
	clusterZero
	clusterUnallocated
)

func checkQCOW2Magic(magic uint32) bool {
	dst := [4]byte{}
	binary.BigEndian.PutUint32(dst[:], magic)

	return !bytes.Equal(dst[:], qcow2Magic)
}
