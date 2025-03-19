package streamingcrypto

import (
	"errors"
	"fmt"
	"io"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
)

type aeadReadEncrypter struct {
	aeadEncrypterCommon

	reader io.ReadCloser

	// See readChunk() for description of curChunk and nextChunkByte.
	curChunk      []byte
	nextChunkByte byte
	gotEOF        bool
	curChunkLen   int
	chunkReadPos  int
	curChunkNum   uint32

	totalPlaintext int64
	// We keep these two variables only for checking in Metadata() if the user read all the ciphertext.
	totalCiphertext int64
	totalReadFrom   int64
}

func (e *aeadReadEncrypter) Read(p []byte) (int, error) {
	if e.closed {
		return 0, fmt.Errorf("encrypter was closed")
	}

	read := 0
	remaining := len(p)
	var err error
	for {
		n := e.curChunkLen - e.chunkReadPos
		if n > remaining {
			copy(p[read:], e.curChunk[e.chunkReadPos:e.chunkReadPos+remaining])
			e.chunkReadPos += remaining
			read += remaining
			break
		}

		copy(p[read:], e.curChunk[e.chunkReadPos:e.chunkReadPos+n])
		e.chunkReadPos += n
		read += n
		remaining -= n

		err = e.readChunk()
		if err != nil {
			break
		}
	}
	e.totalReadFrom += int64(read)
	return read, err
}

func (e *aeadReadEncrypter) readChunk() error {
	if e.gotEOF {
		return io.EOF
	}
	chunkPayload := e.chunkSize - e.chunkTagSize
	// NOTE: curChunk has size chunkSize+1 to correctly determine the last chunk: during the first readChunk() we
	// read chunkPayload+1 bytes and encrypt the first chunkPayload bytes, saving the last byte for the second chunk.
	// Next calls to readChunk() will read the last chunkPayload bytes and set gotEOF if less than chunkPayload bytes are read.
	// We want to do only one Read() call, so we allocate the curChunk to hold at least chunkPayload+1 bytes.
	var n int
	var err error
	if e.totalCiphertext == 0 {
		// Reading the first chunk.
		n, err = io.ReadFull(e.reader, e.curChunk[:chunkPayload+1])
	} else {
		n, err = io.ReadFull(e.reader, e.curChunk[1:chunkPayload+1])
		// Copy the first byte from the last Read(). n is equal to number of bytes in curChunk
		e.curChunk[0] = e.nextChunkByte
		n++
	}
	if err != nil && (err != io.EOF && err != io.ErrUnexpectedEOF) {
		return err
	}
	if n == chunkPayload+1 {
		e.nextChunkByte = e.curChunk[chunkPayload]
	} else {
		e.gotEOF = true
	}
	curChunkPayload := n
	if curChunkPayload > chunkPayload {
		curChunkPayload = chunkPayload
	}

	fillNonceBuf(e.nonceBuf, noncePrefixSize, e.curChunkNum, e.gotEOF)
	e.aead.Seal(e.curChunk[:0], e.nonceBuf, e.curChunk[:curChunkPayload], nil)
	e.chunkReadPos = 0
	e.curChunkLen = curChunkPayload + e.chunkTagSize
	e.curChunkNum++
	e.totalPlaintext += int64(curChunkPayload)
	e.totalCiphertext += int64(e.curChunkLen)
	return nil
}

func (e *aeadReadEncrypter) Close() error {
	if e.closed {
		return nil
	}

	err := e.reader.Close()
	if err != nil {
		return err
	}
	e.close(e.totalPlaintext)
	return nil
}

func (e *aeadReadEncrypter) Metadata() (string, error) {
	if e.totalCiphertext != e.totalReadFrom {
		return "", fmt.Errorf("did not read until EOF")
	}
	return e.metadata()
}

func (e *aeadReadEncrypter) KeyID() string {
	return e.keyID()
}

func (e *aeadReadEncrypter) CiphertextSize(plaintextSize int64) int64 {
	return ciphertextSize(plaintextSize, e.chunkSize, e.chunkTagSize)
}

func (e *aeadReadEncrypter) initCustomKeyReadEncrypter(
	keyID string, aad []byte, kmsClient kmslbclient.KMSClient, reader io.ReadCloser,
	options AeadEncrypterOptions, callCreds *kmslbclient.CallCredentials,
) error {
	err := e.initCustomKeyEncrypterCommon(keyID, aad, kmsClient, options, callCreds)
	if err != nil {
		return err
	}

	e.reader = reader
	// See readChunk for explanation of curChunk size.
	if e.chunkTagSize > 0 {
		e.curChunk = make([]byte, e.chunkSize)
	} else {
		e.curChunk = make([]byte, e.chunkSize+1)
	}
	return nil
}

func (e *aeadReadEncrypter) initDefaultKeyReadEncrypter(
	dks DefaultKeysStorage, defaultKeyID string, aad []byte, reader io.ReadCloser,
	options AeadEncrypterOptions,
) error {
	err := e.initDefaultKeyEncrypterCommon(dks, defaultKeyID, aad, options)
	if err != nil {
		return err
	}

	e.reader = reader
	// See readChunk for explanation of curChunk size.
	if e.chunkTagSize > 0 {
		e.curChunk = make([]byte, e.chunkSize)
	} else {
		e.curChunk = make([]byte, e.chunkSize+1)
	}
	return nil
}

// Returns a new StreamingReadEncrypter for given reader. keyID is a key id to be passed to kmsClient,
// aad is either additional authenticated data or nil, kmsClient is used to encrypt the data encryption key,
// reader is the plaintext reader.
func NewStreamingAeadReadEncrypter(
	keyID string, aad []byte, kmsClient kmslbclient.KMSClient,
	reader io.ReadCloser, callCreds *kmslbclient.CallCredentials,
) (StreamingReadEncrypter, error) {
	return NewStreamingAeadReadEncrypterWithOptions(keyID, aad, kmsClient, reader, AeadEncrypterOptions{}, callCreds)
}

// Returns a new StreamingReadEncrypter with given encrypter options.
func NewStreamingAeadReadEncrypterWithOptions(
	keyID string, aad []byte, kmsClient kmslbclient.KMSClient, reader io.ReadCloser,
	options AeadEncrypterOptions, callCreds *kmslbclient.CallCredentials,
) (StreamingReadEncrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	e := &aeadReadEncrypter{}
	err := e.initCustomKeyReadEncrypter(keyID, aad, kmsClient, reader, options, callCreds)
	if err != nil {
		return nil, err
	}
	return e, nil
}

// Returns a new StreamingReadEncrypter with given encrypter options.
func NewStreamingAeadDefaultReadEncrypterWithOptions(
	dks DefaultKeysStorage, defaultKeyID string, aad []byte, reader io.ReadCloser, options AeadEncrypterOptions,
) (StreamingReadEncrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	e := &aeadReadEncrypter{}
	err := e.initDefaultKeyReadEncrypter(dks, defaultKeyID, aad, reader, options)
	if err != nil {
		return nil, err
	}
	return e, nil
}
