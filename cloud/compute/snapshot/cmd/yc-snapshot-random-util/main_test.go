package main

import (
	"bytes"
	"crypto/md5"
	"io"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/compute/snapshot/cmd/yc-snapshot-random-util/internal/rand"
)

func TestRandomBlockGenerator(t *testing.T) {
	// We need to ensure math/rand to generate same data on all future versions
	randomBlockGenerator := rand.New(rand.NewSource(Seed))
	testBlock := make([]byte, 4*1024*KB)
	_, _ = randomBlockGenerator.Read(testBlock) // no error in standard go random
	sum := md5.Sum(testBlock)
	assert.Equal(t,
		[16]byte{0xea, 0xb0, 0x45, 0xde, 0xed, 0x7f, 0xac, 0x26, 0x19, 0xdf, 0x76, 0x1b, 0xcd, 0xa, 0x17, 0x6c}, sum)
}

type writerAt struct {
	w io.Writer
}

func (fw writerAt) WriteAt(p []byte, _ int64) (int, error) {
	return fw.w.Write(p)
}

func TestWriteRandomBlocks(t *testing.T) {
	a := assert.New(t)
	size, blockSize := int64(900), int64(200)
	buff := bytes.Buffer{}
	err := writeBlocks(size, blockSize, writerAt{&buff}, newRandomOffsetShuffler())
	a.NoError(err)
	data := buff.Bytes()
	// ensure both whole blocks and appendix are non-empty
	a.Equal(size, int64(len(data)))
	empty := make([]byte, 800)
	a.NotEqual(empty, data[:800])
	empty = make([]byte, 100)
	a.NotEqual(empty, data[801:])
}
