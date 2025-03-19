package chunker

import (
	"bytes"
	"crypto/sha512"
	"encoding/hex"
	"fmt"
	"hash"
	"hash/crc32"
	"io"
	"sync"

	"golang.org/x/crypto/blake2b"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

// HashConstructor builds a Hasher.
type HashConstructor func() Hasher

// Hasher is a chunk hashing abstraction.
type Hasher struct {
	hash.Hash
	Name string
}

// Sha512Hasher returns a sha512 hasher.
func Sha512Hasher() Hasher {
	return Hasher{
		Hash: sha512.New(),
		Name: misc.SHA512Hash,
	}
}

func CRC32Hasher() Hasher {
	return Hasher{
		Hash: crc32.NewIEEE(),
		Name: misc.CRC32Hash,
	}
}

func Blake2Hasher() Hasher {
	h, err := blake2b.New256(nil)
	if err != nil {
		panic(fmt.Sprintf("failed to create blake2 hasher: %v", err))
	}
	return Hasher{
		Hash: h,
		Name: misc.Blake2Hash,
	}
}

// BuildHasher returns a hash constructor by hash name.
func BuildHasher(name string) (HashConstructor, error) {
	switch name {
	case misc.SHA512Hash:
		return Sha512Hasher, nil
	case misc.CRC32Hash:
		return CRC32Hasher, nil
	case misc.Blake2Hash:
		return Blake2Hasher, nil
	default:
		return nil, fmt.Errorf("unknown hash: %s", name)
	}
}

// MustBuildHasher same as BuildHasher but panic if failed (for tests)
func MustBuildHasher(name string) HashConstructor {
	h, err := BuildHasher(name)
	if err != nil {
		panic(err)
	}
	return h
}

type zeroHashEntry struct {
	Name      string
	ChunkSize int64
}

type zeroHashMap struct {
	m    map[zeroHashEntry]string
	lock sync.Mutex
}

func (m *zeroHashMap) Get(chunkSize int64, h Hasher) string {
	m.lock.Lock()
	defer m.lock.Unlock()

	if v, ok := m.m[zeroHashEntry{h.Name, chunkSize}]; ok {
		return v
	}

	// Fill cache
	buf := bytes.NewReader(make([]byte, chunkSize))
	nn, err := io.CopyN(h, buf, chunkSize)
	if err != nil || nn != chunkSize {
		return ""
	}
	chunkHash := h.Name + ":" + hex.EncodeToString(h.Sum(nil))
	m.m[zeroHashEntry{h.Name, chunkSize}] = chunkHash
	return chunkHash
}

var (
	zero     = make([]byte, 1024*1024)
	zeroHash = zeroHashMap{
		m: make(map[zeroHashEntry]string),
	}
)

// NotZero is a all-zero check hasher.
// The false value means all writes were zero.
type NotZero bool

func (h *NotZero) Write(b []byte) (int, error) {
	h.write(b)
	return len(b), nil
}

func (h *NotZero) write(b []byte) {
	for pos := 0; !*h && pos < len(b); pos += len(zero) {
		if len(b)-pos >= len(zero) {
			*h = *h || NotZero(!bytes.Equal(b[pos:pos+len(zero)], zero))
		} else {
			*h = *h || NotZero(!bytes.Equal(b[pos:], zero[:len(b)-pos]))
		}
	}
}

func isZero(data []byte) bool {
	var nz NotZero
	_, _ = nz.Write(data)
	return !bool(nz)
}
