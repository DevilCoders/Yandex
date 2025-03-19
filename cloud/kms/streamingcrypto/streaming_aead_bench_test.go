package streamingcrypto

import (
	"bytes"
	"fmt"
	"io"
	"math/rand"
	"testing"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
)

const (
	enableOneReadWrite = false
)

func BenchmarkStreamingAesEncrypt(b *testing.B) {
	ImplBenchmarkStreamingAesEncrypt(b, fakeKeyID, &fakeKmsClient{})
}

func ImplBenchmarkStreamingAesEncrypt(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient) {
	b.Logf("ImplBenchmarkStreamingAesEncrypt with keyID %s", keyID)
	chunkSizes := []int{1024, 4 * 1024, 8 * 1024, 16 * 1024, 64 * 1024}
	plaintextSizes := []int{16 * 1024, 128 * 1024, 1024 * 1024}
	algorithms := []AEADAlgorithm{AesGcm128, AesGcm256, ChaCha20Poly1305}
	for _, chunkSize := range chunkSizes {
		for _, plaintextSize := range plaintextSizes {
			for _, algorithm := range algorithms {
				name := benchName(algorithm, chunkSize, plaintextSize)
				// Benchmark the StreamingEncrypter.
				b.Run(name, func(b *testing.B) {
					benchmarkEncrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Write() with the whole plaintext.
					b.Run(name+"-oneWrite", func(b *testing.B) {
						benchmarkEncrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true)
					})
				}
				// Benchmark the StreamingEncrypter which is initialized only once (instead of re-initializing on
				// each iteration. Skips initializing the AEAD and generating various random byte arrays).
				b.Run(name+"-initOnce", func(b *testing.B) {
					benchmarkEncryptInitOnce(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Write() with the whole plaintext.
					b.Run(name+"-initOnce-oneWrite", func(b *testing.B) {
						benchmarkEncryptInitOnce(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true)
					})
				}
				// Benchmark the StreamingReadEncrypter.
				b.Run(name+"-read", func(b *testing.B) {
					benchmarkEncryptRead(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Read() with the whole ciphertext.
					b.Run(name+"-read-oneRead", func(b *testing.B) {
						benchmarkEncryptRead(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true)
					})
				}
				// Benchmark the StreamingReadEncrypter which is initialized only once (instead of re-initializing on
				// each iteration. Skips initializing the AEAD and generating various random byte arrays).
				b.Run(name+"-read-initOnce", func(b *testing.B) {
					benchmarkEncryptReadInitOnce(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Read() with the whole ciphertext.
					b.Run(name+"-read-initOnce-oneRead", func(b *testing.B) {
						benchmarkEncryptReadInitOnce(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true)
					})
				}
			}
		}
	}
}

func benchName(algorithm AEADAlgorithm, chunkSize int, plaintextSize int) string {
	pbAlg, _ := algorithmToPbAlgorithm(algorithm)
	return fmt.Sprintf("%s-chunk-%dkb-size-%dkb", pbAlg.String(), chunkSize/1024, plaintextSize/1024)
}

func benchmarkEncrypt(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm, chunkSize int, plaintextSize int, oneWrite bool) {
	plaintext := make([]byte, plaintextSize)
	rand.Read(plaintext)

	b.RunParallel(func(pb *testing.PB) {
		wbuf := CloserBuffer{}
		wbuf.Grow(plaintextSize * 2)
		for pb.Next() {
			wbuf.Reset()
			options := AeadEncrypterOptions{Algorithm: algorithm, ChunkSize: chunkSize}
			e, err := NewStreamingAeadEncrypterWithOptions(keyID, nil, kmsClient, &wbuf, options, nil)
			if err != nil {
				b.Fatalf("error in NewStreamingAeadEncrypterWithOptions: %v", err)
			}
			if oneWrite {
				n, err := e.Write(plaintext)
				if err != nil {
					b.Fatalf("error while d.Write: %v", err)
				}
				if n != plaintextSize {
					b.Fatalf("plaintextSize vs n mismatch: %d vs %d", plaintextSize, n)
				}
			} else {
				// NOTE: We actually split the plaintext between different chunks here: the chunk payload is 16 bytes smaller than chunkSize.
				err = writeInChunks(e, plaintext, chunkSize)
				if err != nil {
					b.Fatalf("error while writeInChunks: %v", err)
				}
			}
		}
	})
}

type VoidWriter struct {
}

func (v VoidWriter) Write(p []byte) (n int, err error) {
	return len(p), nil
}

func (v VoidWriter) Close() error {
	return nil
}

func benchmarkEncryptInitOnce(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm, chunkSize int, plaintextSize int, oneWrite bool) {
	plaintext := make([]byte, plaintextSize)
	rand.Read(plaintext)

	b.RunParallel(func(pb *testing.PB) {
		wbuf := VoidWriter{}
		options := AeadEncrypterOptions{Algorithm: algorithm, ChunkSize: chunkSize}
		e, err := NewStreamingAeadEncrypterWithOptions(keyID, nil, kmsClient, &wbuf, options, nil)
		if err != nil {
			b.Fatalf("error in NewStreamingAeadEncrypterWithOptions: %v", err)
		}
		for pb.Next() {
			if oneWrite {
				n, err := e.Write(plaintext)
				if err != nil {
					b.Fatalf("error while d.Write: %v", err)
				}
				if n != plaintextSize {
					b.Fatalf("plaintextSize vs n mismatch: %d vs %d", plaintextSize, n)
				}
			} else {
				// NOTE: We actually split the plaintext between different chunks here: the chunk payload is 16 bytes smaller than chunkSize.
				err = writeInChunks(e, plaintext, chunkSize)
				if err != nil {
					b.Fatalf("error while writeInChunks: %v", err)
				}
			}
		}
	})
}

func benchmarkEncryptRead(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm, chunkSize int, plaintextSize int, oneRead bool) {
	plaintext := make([]byte, plaintextSize)
	rand.Read(plaintext)

	b.RunParallel(func(pb *testing.PB) {
		var ciphertext []byte
		for pb.Next() {
			buf := CloserBuffer{Buffer: *bytes.NewBuffer(plaintext)}
			options := AeadEncrypterOptions{Algorithm: algorithm, ChunkSize: chunkSize}
			e, err := NewStreamingAeadReadEncrypterWithOptions(keyID, nil, kmsClient, &buf, options, nil)
			if err != nil {
				b.Fatalf("error in NewStreamingAeadReadEncrypterWithOptions: %v", err)
			}
			ciphertextSize := int(e.CiphertextSize(int64(plaintextSize)))
			if ciphertext == nil {
				ciphertext = make([]byte, ciphertextSize)
			}
			var n int
			if oneRead {
				n, err = e.Read(ciphertext)
			} else {
				n, err = readInChunksNoSeek(e, chunkSize, ciphertext)
			}
			if err != nil && err != io.EOF {
				b.Fatalf("error while e.Read: %v", err)
			}
			if n != ciphertextSize {
				b.Fatalf("ciphertextSize vs n mismatch: %d vs %d", ciphertextSize, n)
			}
		}
	})
}

type InfiniteReader struct{}

func (b *InfiniteReader) Read(p []byte) (int, error) {
	return len(p), nil
}

func (b *InfiniteReader) Close() error {
	return nil
}

func benchmarkEncryptReadInitOnce(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm, chunkSize int, plaintextSize int, oneRead bool) {
	b.RunParallel(func(pb *testing.PB) {
		buf := InfiniteReader{}
		options := AeadEncrypterOptions{Algorithm: algorithm, ChunkSize: chunkSize}
		e, err := NewStreamingAeadReadEncrypterWithOptions(keyID, nil, kmsClient, &buf, options, nil)
		if err != nil {
			b.Fatalf("error in NewStreamingAeadReadEncrypterWithOptions: %v", err)
		}
		ciphertextSize := int(e.CiphertextSize(int64(plaintextSize)))
		ciphertext := make([]byte, ciphertextSize)

		for pb.Next() {
			var n int
			// NOTE: We actually do different things here: if oneRead is true we fill the whole ciphertext buffer, in the other case
			// we only use the first chunkSize bytes of ciphertext as a scratch buffer. This matters quite a lot of ciphertext
			// does not fit cache.
			if oneRead {
				n, err = e.Read(ciphertext)
			} else {
				n, err = skipInChunks(e, chunkSize, ciphertextSize, ciphertext)
			}
			if err != nil && err != io.EOF {
				b.Fatalf("error while e.Read: %v", err)
			}
			if n != ciphertextSize {
				b.Fatalf("ciphertextSize vs n mismatch: %d vs %d", ciphertextSize, n)
			}
		}
	})
}

func skipInChunks(r io.Reader, readSize int, length int, scratch []byte) (int, error) {
	read := 0
	for read < length {
		toRead := readSize
		if length-read < toRead {
			toRead = length - read
		}
		n, err := r.Read(scratch[:toRead])
		if err != nil && err != io.EOF {
			return 0, fmt.Errorf("d.Read with length %d, readSize %d: %v", length, readSize, err)
		}
		if n != toRead && err != io.EOF {
			return 0, fmt.Errorf("mismatch n and toRead: %d vs %d", n, toRead)
		}
		read += n
		if err == io.EOF {
			break
		}
	}
	return read, nil
}

func BenchmarkStreamingAesDecrypt(b *testing.B) {
	ImplBenchmarkStreamingAesDecrypt(b, fakeKeyID, &fakeKmsClient{})
}

func ImplBenchmarkStreamingAesDecrypt(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient) {
	b.Logf("ImplBenchmarkStreamingAesDecrypt with keyID %s", keyID)
	chunkSizes := []int{1024, 4 * 1024, 8 * 1024, 16 * 1024, 64 * 1024}
	plaintextSizes := []int{16 * 1024, 128 * 1024, 1024 * 1024}
	algorithms := []AEADAlgorithm{AesGcm128, AesGcm256, ChaCha20Poly1305}
	for _, chunkSize := range chunkSizes {
		for _, plaintextSize := range plaintextSizes {
			for _, algorithm := range algorithms {
				name := benchName(algorithm, chunkSize, plaintextSize)
				// Benchmarks StreamingDecrypter.
				b.Run(name, func(b *testing.B) {
					benchmarkDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Read() with the whole plaintext.
					b.Run(name+"-oneRead", func(b *testing.B) {
						benchmarkDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false, true)
					})
				}
				// Benchmark the StreamingDecrypter which is initialized only once (instead of re-initializing on
				// each iteration. Skips initializing the AEAD and generating various random byte arrays).
				b.Run(name+"-initOnce", func(b *testing.B) {
					benchmarkDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Read() with the whole plaintext.
					b.Run(name+"-initOnce-oneRead", func(b *testing.B) {
						benchmarkDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true, true)
					})
				}
				// Benchmarks StreamingRangeDecrypter.
				b.Run(name+"-range", func(b *testing.B) {
					benchmarkRangeDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Read() with the whole plaintext.
					b.Run(name+"-range-oneRead", func(b *testing.B) {
						benchmarkRangeDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, false, true)
					})
				}
				// Benchmark the StreamingDecrypter which is initialized only once (instead of re-initializing on
				// each iteration. Skips initializing the AEAD and generating various random byte arrays).
				b.Run(name+"-range-initOnce", func(b *testing.B) {
					benchmarkRangeDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true, false)
				})
				if enableOneReadWrite {
					// Previous benchmark with one call to Read() with the whole plaintext.
					b.Run(name+"-range-initOnce-oneRead", func(b *testing.B) {
						benchmarkRangeDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintextSize, true, true)
					})
				}
			}
		}
	}
}

func prepareBenchmarkDecrypt(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm, chunkSize int, plaintext []byte) ([]byte, string) {
	wbuf := CloserBuffer{}
	options := AeadEncrypterOptions{Algorithm: algorithm, ChunkSize: chunkSize}
	e, err := NewStreamingAeadEncrypterWithOptions(keyID, nil, kmsClient, &wbuf, options, nil)
	if err != nil {
		b.Fatalf("error in NewStreamingAeadEncrypterWithOptions: %v", err)
	}
	wbuf.Grow(int(e.CiphertextSize(int64(len(plaintext)))))
	_, err = e.Write(plaintext)
	if err != nil {
		b.Fatalf("error while e.Write: %v", err)
	}
	err = e.Close()
	if err != nil {
		b.Fatalf("error while e.Close: %v", err)
	}
	metadata, err := e.Metadata()
	if err != nil {
		b.Fatalf("error in e.Metadata: %v", err)
	}
	return wbuf.Bytes(), metadata
}

func benchmarkDecrypt(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm, chunkSize int, plaintextSize int, initOnce bool, oneRead bool) {
	plaintext := make([]byte, plaintextSize)
	rand.Read(plaintext)

	ciphertext, metadata := prepareBenchmarkDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintext)

	b.RunParallel(func(pb *testing.PB) {
		var d StreamingDecrypter
		var err error
		if initOnce {
			d, err = NewStreamingAeadDecrypter(metadata, nil, kmsClient, &SeekerBuffer{buf: ciphertext}, nil)
			if err != nil {
				b.Fatalf("error in NewStreamingAeadDecrypter: %v", err)
			}
		}
		newPlaintext := make([]byte, plaintextSize)
		for pb.Next() {
			if !initOnce {
				d, err = NewStreamingAeadDecrypter(metadata, nil, kmsClient, &SeekerBuffer{buf: ciphertext}, nil)
				if err != nil {
					b.Fatalf("error in NewStreamingAeadDecrypter: %v", err)
				}
			}

			if oneRead {
				n, err := d.Read(newPlaintext)
				if err != nil && err != io.EOF {
					b.Fatalf("error in d.Read: %v", err)
				}
				if n != plaintextSize {
					b.Fatalf("plaintextSize vs n mismatch: %d vs %d", plaintextSize, n)
				}
			} else {
				n, err := readInChunks(d, 0, chunkSize, newPlaintext)
				if err != nil && err != io.EOF {
					b.Fatalf("error in readInChunks: %v", err)
				}
				if n != plaintextSize {
					b.Fatalf("plaintextSize vs n mismatch: %d vs %d", plaintextSize, n)
				}
			}
		}
	})
}

func benchmarkRangeDecrypt(b *testing.B, keyID string, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm, chunkSize int, plaintextSize int, initOnce bool, oneRead bool) {
	plaintext := make([]byte, plaintextSize)
	rand.Read(plaintext)

	ciphertext, metadata := prepareBenchmarkDecrypt(b, keyID, kmsClient, algorithm, chunkSize, plaintext)

	b.RunParallel(func(pb *testing.PB) {
		var d StreamingRangeDecrypter
		var err error
		if initOnce {
			d, err = NewStreamingAeadRangeDecrypter(metadata, nil, kmsClient, nil)
			if err != nil {
				b.Fatalf("error in NewStreamingAeadDecrypter: %v", err)
			}
		}
		newPlaintext := make([]byte, plaintextSize)
		for pb.Next() {
			if !initOnce {
				d, err = NewStreamingAeadRangeDecrypter(metadata, nil, kmsClient, nil)
				if err != nil {
					b.Fatalf("error in NewStreamingAeadDecrypter: %v", err)
				}
			}

			plainRange := Range{0, int64(plaintextSize)}
			reader, err := d.Plaintext(plainRange, &SeekerBuffer{buf: ciphertext})
			if err != nil {
				b.Fatalf("error in d.Plaintext: %v", err)
			}
			if oneRead {
				n, err := reader.Read(newPlaintext)
				if err != nil && err != io.EOF {
					b.Fatalf("error in d.Read: %v", err)
				}
				if n != plaintextSize {
					b.Fatalf("plaintextSize vs n mismatch: %d vs %d", plaintextSize, n)
				}
			} else {
				n, err := readInChunksNoSeek(reader, chunkSize, newPlaintext)
				if err != nil && err != io.EOF {
					b.Fatalf("error in readInChunksNoSeek: %v", err)
				}
				if n != plaintextSize {
					b.Fatalf("plaintextSize vs n mismatch: %d vs %d", plaintextSize, n)
				}
			}
		}
	})
}
