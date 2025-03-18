package compress

import (
	"crypto/rand"
)

// testBufferGen returns slice filled with random bytes
func testBufferGen(size int) []byte {
	buf := make([]byte, size)
	_, _ = rand.Read(buf)
	return buf
}

// testBufferFill returns slice filled with given byte
func testBufferFill(b []byte, size int) []byte {
	buf := make([]byte, size)
	for i := 0; i < size; i++ {
		buf = append(buf, b...)
	}
	return buf[:size]
}
