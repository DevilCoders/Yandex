package streamingcrypto

import (
	"bytes"
	"context"
	"encoding/base64"
	"fmt"
	"io"
	"math/rand"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
)

const (
	fakeKeyID = "fake-id"

	enableLog = false
)

type fakeKmsClient struct {
}

func (a *fakeKmsClient) Close() {}

func (a *fakeKmsClient) Encrypt(ctx context.Context, keyID string, plaintext []byte, aad []byte, _ *kmslbclient.CallCredentials) (ciphertext []byte, err error) {
	if keyID != fakeKeyID {
		return nil, fmt.Errorf("key id must be %s, is %s", fakeKeyID, keyID)
	}
	ret := []byte(keyID)[:]
	ret = append(ret, aad...)
	ret = append(ret, plaintext...)
	return ret, nil
}

func (a *fakeKmsClient) Decrypt(ctx context.Context, keyID string, ciphertext []byte, aad []byte, _ *kmslbclient.CallCredentials) (plaintext []byte, err error) {
	if keyID != fakeKeyID {
		return nil, fmt.Errorf("key id must be %s, is %s", fakeKeyID, keyID)
	}
	prefix := []byte(keyID)[:]
	prefix = append(prefix, aad...)
	if !bytes.HasPrefix(ciphertext, prefix) {
		return nil, fmt.Errorf("wrong ciphertext prefix: %v vs %v", prefix, ciphertext)
	}
	return ciphertext[len(prefix):], nil
}

func TestStreamingAes(t *testing.T) {
	if disableForExportFlag != "" {
		t.Skip("skipped due to export restrictions")
		return
	}

	defer clearTestBackup(testKeysStorageOptions)
	dks, err := newDefaultKeysStorageWithOptions(fakeKeystoreID, newFakeKeystoreClient(), fakeTokenProvider, testKeysStorageOptions)
	require.NoError(t, err)
	require.NotNil(t, dks)
	ImplTestStreamingAes(t, fakeKeyID, dks, &fakeKmsClient{})
}

func ImplTestStreamingAes(t *testing.T, keyID string, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient) {
	t.Logf("ImplTestStreamingAes with keyID %s", keyID)
	algorithms := []AEADAlgorithm{AesGcm128, AesGcm256, ChaCha20Poly1305}
	writeSizesForChunk17 := []int{1, 2, 3, 15, 16, 17, 18}
	for _, writeSize := range writeSizesForChunk17 {
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 17, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 17, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 17, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 17, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 17, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 17, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 17, 16, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 17, 16, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 17, 16, writeSize)
	}

	writeSizesForChunk24 := []int{1, 2, 3, 7, 8, 9, 23, 24, 25}
	for _, writeSize := range writeSizesForChunk24 {
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 24, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 24, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 24, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 24, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 24, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 24, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 24, 23, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 24, 23, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 24, 23, writeSize)
	}
	for _, algorithm := range algorithms {
		testStreamingAes(t, keyID, []byte("this is aad"), dks, kmsClient, algorithm, 0, 24, 1, 7)
		testStreamingAes(t, keyID, []byte("this is aad"), dks, kmsClient, algorithm, 1, 24, 1, 7)
		testStreamingAes(t, keyID, []byte("this is aad"), dks, kmsClient, algorithm, 10, 24, 1, 7)
	}

	writeSizesForChunk32 := []int{1, 2, 3, 15, 16, 17, 31, 32, 33}
	for _, writeSize := range writeSizesForChunk32 {
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 32, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 32, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 32, 0, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 32, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 32, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 32, 1, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 0, 32, 23, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 1, 32, 23, writeSize)
		testStreamingAes(t, keyID, nil, dks, kmsClient, AesGcm128, 10, 32, 23, writeSize)
	}
	for _, algorithm := range algorithms {
		testStreamingAes(t, keyID, []byte("this is aad"), dks, kmsClient, algorithm, 0, 32, 1, 15)
		testStreamingAes(t, keyID, []byte("this is aad"), dks, kmsClient, algorithm, 1, 32, 1, 15)
		testStreamingAes(t, keyID, []byte("this is aad"), dks, kmsClient, algorithm, 10, 32, 1, 15)
	}
}

type CloserBuffer struct {
	bytes.Buffer
	closed bool
}

func (b *CloserBuffer) Close() error {
	b.closed = true
	return nil
}

type SeekerBuffer struct {
	buf []byte
	off int
}

func (s *SeekerBuffer) Read(p []byte) (int, error) {
	remaining := len(s.buf) - s.off
	if remaining < len(p) {
		copy(p, s.buf[s.off:s.off+remaining])
		s.off += remaining
		return remaining, io.EOF
	}
	copy(p, s.buf[s.off:s.off+len(p)])
	s.off += len(p)
	return len(p), nil
}

func (s *SeekerBuffer) Seek(offset int64, whence int) (int64, error) {
	switch whence {
	case io.SeekStart:
		s.off = int(offset)
	case io.SeekCurrent:
		s.off += int(offset)
	case io.SeekEnd:
		s.off = len(s.buf) + int(offset)
	default:
		panic("unknown whence")
	}
	return int64(s.off), nil
}

func (s *SeekerBuffer) Close() error {
	return nil
}

func testStreamingAes(
	t *testing.T, keyID string, aad []byte, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient, algorithm AEADAlgorithm,
	numChunks int, chunkSize int, lastChunkSize int, readWriteSize int,
) {
	plaintext := preparePlaintext(t, chunkSize, numChunks, lastChunkSize)

	ciphertext, metadata := encryptPlaintextWithCustomKey(t, keyID, aad, kmsClient, plaintext, algorithm, chunkSize, readWriteSize)
	// decrypt with custom key should work without defaultKeysStorage
	testDecrypts(t, aad, plaintext, nil, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize, true)
	testRangeDecrypts(t, aad, plaintext, nil, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize)

	// encrypt without defaultKeyID
	ciphertext, metadata = encryptPlaintextWithDefaultKey(t, dks, "", aad, plaintext, algorithm, chunkSize, readWriteSize)
	testDecrypts(t, aad, plaintext, dks, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize, true)
	testRangeDecrypts(t, aad, plaintext, dks, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize)

	defaultKeyID, err := dks.GetPrimaryDefaultKeyID()
	require.NoErrorf(t, err, "error when get primary default key ID")

	// encrypt with specified defaultKeyID
	ciphertext, metadata = encryptPlaintextWithDefaultKey(t, dks, defaultKeyID, aad, plaintext, algorithm, chunkSize, readWriteSize)
	testDecrypts(t, aad, plaintext, dks, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize, true)
	testRangeDecrypts(t, aad, plaintext, dks, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize)

	ciphertext, metadata = encryptPlaintextViaReadWithCustomKey(t, keyID, aad, kmsClient, plaintext, algorithm, chunkSize, readWriteSize)
	testDecrypts(t, aad, plaintext, nil, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize, true)

	// encrypt without defaultKeyID
	ciphertext, metadata = encryptPlaintextViaReadWithDefaultKey(t, aad, dks, "", plaintext, algorithm, chunkSize, readWriteSize)
	testDecrypts(t, aad, plaintext, dks, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize, true)
}

func preparePlaintext(t *testing.T, chunkSize int, numChunks int, lastChunkSize int) []byte {
	const tagSize = 16
	plaintextSize := (chunkSize-tagSize)*numChunks + lastChunkSize
	plaintext := make([]byte, plaintextSize)
	n, err := rand.Read(plaintext)
	require.EqualValues(t, n, plaintextSize)
	require.NoErrorf(t, err, "rand.Read")
	return plaintext
}

func encryptPlaintextWithCustomKey(t *testing.T, keyID string, aad []byte, kmsClient kmslbclient.KMSClient, plaintext []byte, algorithm AEADAlgorithm, chunkSize int, readWriteSize int) ([]byte, string) {
	var buf CloserBuffer
	options := AeadEncrypterOptions{
		Algorithm: algorithm,
		ChunkSize: chunkSize,
	}
	e, err := NewStreamingAeadEncrypterWithOptions(keyID, aad, kmsClient, &buf, options, nil)
	require.NoErrorf(t, err, "error when NewStreamingAeadEncrypterWithOptions")

	targetSize := e.CiphertextSize(int64(len(plaintext)))
	writeInChunksNoError(t, e, plaintext, readWriteSize)
	require.NoErrorf(t, e.Close(), "error when e.Close")
	require.EqualValues(t, targetSize, int64(buf.Len()))
	require.True(t, buf.closed)

	metadata, err := e.Metadata()
	require.NoErrorf(t, err, "error when e.Metadata")

	return buf.Bytes(), metadata
}

func encryptPlaintextWithDefaultKey(t *testing.T, dks DefaultKeysStorage, defaultKeyID string, aad []byte, plaintext []byte, algorithm AEADAlgorithm, chunkSize int, readWriteSize int) ([]byte, string) {
	var buf CloserBuffer
	options := AeadEncrypterOptions{
		Algorithm: algorithm,
		ChunkSize: chunkSize,
	}
	e, err := NewStreamingAeadDefaultEncrypterWithOptions(dks, defaultKeyID, aad, &buf, options)
	require.NoErrorf(t, err, "error when NewStreamingAeadDefaultEncrypterWithOptions")

	targetSize := e.CiphertextSize(int64(len(plaintext)))
	writeInChunksNoError(t, e, plaintext, readWriteSize)
	require.NoErrorf(t, e.Close(), "error when e.Close")
	require.EqualValues(t, targetSize, int64(buf.Len()))
	require.True(t, buf.closed)

	metadata, err := e.Metadata()
	require.NoErrorf(t, err, "error when e.Metadata")

	return buf.Bytes(), metadata
}

func encryptPlaintextViaReadWithCustomKey(t *testing.T, keyID string, aad []byte, kmsClient kmslbclient.KMSClient, plaintext []byte, algorithm AEADAlgorithm, chunkSize int, readWriteSize int) ([]byte, string) {
	options := AeadEncrypterOptions{
		Algorithm: algorithm,
		ChunkSize: chunkSize,
	}
	buf := CloserBuffer{Buffer: *bytes.NewBuffer(plaintext)}
	e, err := NewStreamingAeadReadEncrypterWithOptions(keyID, aad, kmsClient, &buf, options, nil)
	require.NoErrorf(t, err, "error when NewStreamingAeadReadEncrypterWithOptions")

	targetSize := e.CiphertextSize(int64(len(plaintext)))
	ciphertext := make([]byte, targetSize*2)
	read, err := readInChunksNoSeek(e, readWriteSize, ciphertext)
	require.NoErrorf(t, err, "readInChunksNoSeek")
	ciphertext = ciphertext[:read]
	require.NoErrorf(t, e.Close(), "error when e.Close")
	require.EqualValues(t, targetSize, int64(len(ciphertext)))
	require.True(t, buf.closed)

	metadata, err := e.Metadata()
	require.NoErrorf(t, err, "error when e.Metadata")

	return ciphertext, metadata
}

func encryptPlaintextViaReadWithDefaultKey(t *testing.T, aad []byte, dks DefaultKeysStorage, defaultKeyID string, plaintext []byte, algorithm AEADAlgorithm, chunkSize int, readWriteSize int) ([]byte, string) {
	options := AeadEncrypterOptions{
		Algorithm: algorithm,
		ChunkSize: chunkSize,
	}
	buf := CloserBuffer{Buffer: *bytes.NewBuffer(plaintext)}
	e, err := NewStreamingAeadDefaultReadEncrypterWithOptions(dks, defaultKeyID, aad, &buf, options)
	require.NoErrorf(t, err, "error when NewStreamingAeadDefaultReadEncrypterWithOptions")

	targetSize := e.CiphertextSize(int64(len(plaintext)))
	ciphertext := make([]byte, targetSize*2)
	read, err := readInChunksNoSeek(e, readWriteSize, ciphertext)
	require.NoErrorf(t, err, "readInChunksNoSeek")
	ciphertext = ciphertext[:read]
	require.NoErrorf(t, e.Close(), "error when e.Close")
	require.EqualValues(t, targetSize, int64(len(ciphertext)))
	require.True(t, buf.closed)

	metadata, err := e.Metadata()
	require.NoErrorf(t, err, "error when e.Metadata")

	return ciphertext, metadata
}

func testDecrypts(t *testing.T, aad []byte, plaintext []byte, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient, ciphertext []byte, metadata string, numChunks int, chunkSize int, lastChunkSize int, readWriteSize int, failOnOldMetadata bool) {
	sbuf := SeekerBuffer{buf: ciphertext}
	d, err := NewStreamingAeadDecrypterWithMetadataFlagAndDefaultKey(metadata, aad, dks, kmsClient, &sbuf, nil, failOnOldMetadata)
	require.NoErrorf(t, err, "error when NewStreamingAeadDecrypter")

	lengths := []int{0, 1, 2, chunkSize - 1, chunkSize, chunkSize + 1, 2*chunkSize - 1, 2 * chunkSize, 2*chunkSize + 1,
		len(plaintext) - chunkSize, len(plaintext) - chunkSize - 1, len(plaintext) - 1, len(plaintext)}
	offsets := []int{0, 1, 2, chunkSize - 1, chunkSize, chunkSize + 1}
	if lastChunkSize > 1 {
		offsets = append(offsets, chunkSize*numChunks-1, chunkSize*numChunks, chunkSize*numChunks+1)
	}
	for _, l := range lengths {
		if l < 0 {
			continue
		}
		for _, off := range offsets {
			if off < 0 {
				continue
			}
			if enableLog {
				t.Logf("aad == %#v && numChunks == %d && chunkSize == %d && lastChunkSize == %d && readWriteSize == %d && l == %d && off == %d", aad, numChunks, chunkSize, lastChunkSize, readWriteSize, l, off)
			}
			read := readInChunksNoError(t, d, l, off, readWriteSize)
			require.True(t, bytes.Equal(safeSlice(plaintext, off, off+l), read))
		}
	}

	testEOF(t, d, len(plaintext))
}

func testRangeDecrypts(t *testing.T, aad []byte, plaintext []byte, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient, ciphertext []byte, metadata string, numChunks int, chunkSize int, lastChunkSize int, readWriteSize int) {
	d, err := NewStreamingAeadRangeDecrypterWithMetadataFlagAndDefaultKey(metadata, aad, dks, kmsClient, nil, true)
	require.NoErrorf(t, err, "error when NewStreamingAeadDecrypter")

	lengths := []int{0, 1, 2, chunkSize - 1, chunkSize, chunkSize + 1, 2*chunkSize - 1, 2 * chunkSize, 2*chunkSize + 1,
		len(plaintext) - chunkSize, len(plaintext) - chunkSize - 1, len(plaintext) - 1, len(plaintext)}
	offsets := []int{0, 1, 2, chunkSize - 1, chunkSize, chunkSize + 1}
	if lastChunkSize > 1 {
		offsets = append(offsets, chunkSize*numChunks-1, chunkSize*numChunks, chunkSize*numChunks+1)
	}
	for _, l := range lengths {
		if l < 0 {
			continue
		}
		for _, off := range offsets {
			if off < 0 {
				continue
			}
			if enableLog {
				t.Logf("range: aad == %#v && numChunks == %d && chunkSize == %d && lastChunkSize == %d && readWriteSize == %d && l == %d && off == %d", aad, numChunks, chunkSize, lastChunkSize, readWriteSize, l, off)
			}
			read := readRangeInChunksNoError(t, ciphertext, d, l, off, readWriteSize)
			require.True(t, bytes.Equal(safeSlice(plaintext, off, off+l), read))
		}
	}

	testRangeEOF(t, ciphertext, d, len(plaintext))
}

func safeSlice(b []byte, l int, r int) []byte {
	if l > len(b) {
		l = len(b)
	}
	if r > len(b) {
		r = len(b)
	}
	return b[l:r]
}

func writeInChunksNoError(t *testing.T, w io.Writer, plaintext []byte, writeSize int) {
	require.NoErrorf(t, writeInChunks(w, plaintext, writeSize), "writeInChunks")
}

func writeInChunks(w io.Writer, plaintext []byte, writeSize int) error {
	pos := 0
	for {
		l := len(plaintext) - pos
		if l > writeSize {
			l = writeSize
		}
		n, err := w.Write(plaintext[pos : pos+l])
		if err != nil {
			return err
		}
		if n != l {
			return fmt.Errorf("mismatch n and l: %d vs %d", n, l)
		}
		pos += l
		// Check the condition here so that we test the zero-length writes as well.
		if pos == len(plaintext) {
			break
		}
	}
	return nil
}

func readInChunksNoError(t *testing.T, r io.ReadSeeker, length int, offset int, readSize int) []byte {
	ret := make([]byte, length)
	n, err := readInChunks(r, offset, readSize, ret)
	require.NoErrorf(t, err, "readInChunks")
	return ret[:n]
}

func readInChunks(r io.ReadSeeker, offset int, readSize int, dst []byte) (int, error) {
	off, err := r.Seek(int64(offset), io.SeekStart)
	if err != nil {
		return 0, fmt.Errorf("d.Seek with length %d, offset %d, readSize %d: %v", len(dst), offset, readSize, err)
	}
	if int64(offset) != off {
		return 0, fmt.Errorf("mismatch offset and off: %d vs %d", offset, off)
	}

	return readInChunksNoSeek(r, readSize, dst)
}

func readInChunksNoSeek(r io.Reader, readSize int, dst []byte) (int, error) {
	read := 0
	for read < len(dst) {
		toRead := readSize
		if len(dst)-read < toRead {
			toRead = len(dst) - read
		}
		n, err := r.Read(dst[read : read+toRead])
		if err != nil && err != io.EOF {
			return 0, fmt.Errorf("d.Read with length %d, readSize %d: %v", len(dst), readSize, err)
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

func testEOF(t *testing.T, d StreamingDecrypter, plaintextSize int) {
	_, err := d.Seek(int64(plaintextSize), io.SeekStart)
	require.NoErrorf(t, err, "d.Seek EOF")
	tmp := make([]byte, 1)
	n, err := d.Read(tmp)
	require.EqualValues(t, io.EOF, err)
	require.EqualValues(t, 0, n)
}

func readRangeInChunksNoError(t *testing.T, ciphertext []byte, d StreamingRangeDecrypter, length int, offset int, readSize int) []byte {
	plainRange := Range{Start: int64(offset), Stop: int64(offset + length)}
	cStart, cStop, err := d.CiphertextRange(plainRange.Start, plainRange.Stop)
	cipherRange := Range{Start: cStart, Stop: cStop}
	require.NoErrorf(t, err, "d.CiphertextRange")
	cipherReader := CloserBuffer{Buffer: *bytes.NewBuffer(ciphertext[cipherRange.Start:cipherRange.Stop])}
	plainReader, err := d.Plaintext(plainRange, &cipherReader)
	require.NoErrorf(t, err, "d.Plaintext")
	plaintext := make([]byte, length)
	n, err := readInChunksNoSeek(plainReader, readSize, plaintext)
	require.NoErrorf(t, err, "readInChunksNoSeek")
	require.True(t, n <= length)
	return plaintext[:n]
}

func testRangeEOF(t *testing.T, ciphertext []byte, d StreamingRangeDecrypter, plaintextSize int) {
	plaintext := readRangeInChunksNoError(t, ciphertext, d, 1, plaintextSize, 1)
	require.EqualValues(t, 0, len(plaintext))
}

func TestWithOldMetadata(t *testing.T) {
	if disableForExportFlag != "" {
		t.Skip("skipped due to export restrictions")
		return
	}

	kmsClient := &fakeKmsClient{}
	aad, _ := base64.StdEncoding.DecodeString("dGhpcyBpcyBhYWQ=")
	plaintext, _ := base64.StdEncoding.DecodeString("aUaQOEWZ0Rb40v2Tsq7VW31EtbBU8/OOeI5P3zblkVaMQdEFLK0Py2jKTEv1CQ1X35228Nkd2LEbgE8zGtt++wh6VgTp4itNVNtAvLxuJy/1")
	ciphertext, _ := base64.StdEncoding.DecodeString("kNz9Z4+wjAcuwY1kCJZ2a/wzs7KYug6W8l0YQTaGoP8mY/5Jqma2Q4FwA9QWoKYYHHpD1SinGSS3JISySGPYVRd2D0zy84qzRYfCXxSG29hMcfB/z3nNOAMNQhfLP1QBjED1AkDUA+8EjRe5ws1XR4CVoqVSSV8L9CcS40Uo4mgRF5INHdqfBiuAEzIrlw/VdVrxlj80kMd5LI8QQL72yMSUmRFezkw5aWvFkEC9QmwAX0I4EM866oTecNwzIYpgiIFUjfuaKBnsWMNQp70j4gqCUJMDOeSdCkWbxdUL8ba5Nn1zRUA5nIVRhvRquZSKhVckh8WnQo8jXFYXeB3ocv4=")
	metadata := `{"algorithm":"AES256-GCM","chunkSize":"24","eikm":"ZmFrZS1pZHRoaXMgaXMgYWFkABPEnJ4cQ+kvwBc/KB+fZwtUorCyCNmMZW18sKZj51M=","hkdf":"SHA-256","keyUri":"fake-id","noncePrefix":"ri9XQMebxg==","plaintext":"81","salt":"YB7/Sk5oYBE55OuJAQtIxuX0UZtHdEUynWobXSJtp0Q=","tagSize":"16"}`
	numChunks := 10
	chunkSize := 24
	lastChunkSize := 1
	readWriteSize := 7

	// old metadata only with custom key
	testDecrypts(t, aad, plaintext, nil, kmsClient, ciphertext, metadata, numChunks, chunkSize, lastChunkSize, readWriteSize, false)

	sbuf := SeekerBuffer{buf: ciphertext}
	_, err := NewStreamingAeadDecrypterWithMetadataFlag(metadata, aad, kmsClient, &sbuf, nil, true)
	require.EqualError(t, err, "old json metadata format not supported")

	_, err = NewStreamingAeadRangeDecrypterWithMetadataFlag(metadata, aad, kmsClient, nil, true)
	require.EqualError(t, err, "old json metadata format not supported")
}
