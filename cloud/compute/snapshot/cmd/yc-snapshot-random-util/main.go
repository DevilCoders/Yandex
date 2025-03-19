package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"time"

	"a.yandex-team.ru/cloud/compute/snapshot/cmd/yc-snapshot-random-util/internal/rand"
)

const (
	KB   = 1024
	Seed = 2331125
)

func main() {
	var path string
	var blockSize, size int64
	flag.StringVar(&path, "path", "", "path to file")
	flag.Int64Var(&blockSize, "blksize", 4*KB, "block size of one block in file")
	flag.Int64Var(&size, "size", 0, "size of file")

	flag.Parse()
	if path == "" {
		log.Println("required --path=<path-to-file-to-fill>")
		return
	}
	file, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE, os.FileMode(0666))
	if err != nil {
		log.Fatalf("Error during file opening: %v", err)
	}
	if size == 0 {
		info, err := file.Stat()
		if err != nil {
			log.Fatalf("Error during file.Stat(): %v", err)
		}
		size = info.Size()
	}

	if err = writeBlocks(size, blockSize, file, newRandomOffsetShuffler()); err != nil {
		log.Fatalf("Error during filling file with random data: %v", err)
	}
}

type offsetShuffler interface {
	shuffle([]int64)
}

type randomOffsetShuffler struct {
	r *rand.Rand
}

func newRandomOffsetShuffler() *randomOffsetShuffler {
	return &randomOffsetShuffler{r: rand.New(rand.NewSource(Seed))}
}

func (off *randomOffsetShuffler) shuffle(data []int64) {
	for i := 0; i < len(data); i++ {
		j := off.r.Intn(len(data))
		data[i], data[j] = data[j], data[i]
	}
}

func writeBlocks(size, blockSize int64, output io.WriterAt, shuffler offsetShuffler) error {
	log.Println("Start generate offsets")
	// ceil int64 division (90 / 20 100 / 20 = 5)
	blocksCount := 1 + (size-1)/blockSize
	offsets := make([]int64, blocksCount)
	for i := int64(0); i < blocksCount; i++ {
		offsets[i] = i * blockSize
	}

	log.Println("Start shuffle offsets")
	shuffler.shuffle(offsets)

	log.Println("Start write data")
	const progressInterval = time.Second
	lastProgressPrint := time.Now()
	lastProgressBlocks := 0
	for index, offset := range offsets {
		thisBlockSize := blockSize
		if offset+blockSize > size {
			thisBlockSize = size - offset
		}
		buffer := make([]byte, thisBlockSize)
		_, err := rand.New(rand.NewSource(offset)).Read(buffer)
		if err != nil {
			return err
		}
		n, err := output.WriteAt(buffer, offset)
		if err != nil {
			return err
		}
		if int64(n) != thisBlockSize {
			return fmt.Errorf("readed %v instead of %v", n, thisBlockSize)
		}
		now := time.Now()
		if now.Sub(lastProgressPrint) > progressInterval {
			count := index + 1
			blocksSinceLast := count - lastProgressBlocks
			speedBlocks := float64(blocksSinceLast) / now.Sub(lastProgressPrint).Seconds()
			speedBytes := speedBlocks * float64(blockSize)
			writedBytes := int64(count) * blockSize
			writedPercent := float64(count) / float64(blocksCount) * 100
			log.Printf("Speed blocks:\t%0.1f/sec, Speed bytes:\t%0.0f/sec. Writed blocks\t%v/%v, bytes\t%v/%v (%0.2f%%).\n",
				speedBlocks, speedBytes, count, blocksCount, writedBytes, size, writedPercent)

			lastProgressPrint = time.Now()
			lastProgressBlocks = count
		}
	}
	return nil
}
