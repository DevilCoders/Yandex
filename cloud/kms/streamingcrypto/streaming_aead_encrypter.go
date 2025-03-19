package streamingcrypto

import (
	"errors"
	"fmt"
	"io"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
)

type aeadEncrypter struct {
	aeadEncrypterCommon

	writer io.WriteCloser

	curChunk      []byte
	chunkWritePos int
	curChunkNum   uint32
	plaintextSize int64
}

func (e *aeadEncrypter) Write(p []byte) (int, error) {
	if e.closed {
		return 0, fmt.Errorf("encrypter was closed")
	}

	written := 0
	remaining := len(p)
	for {
		n := e.chunkSize - e.chunkTagSize - e.chunkWritePos
		if n >= remaining {
			copy(e.curChunk[e.chunkWritePos:], p[written:written+remaining])
			e.chunkWritePos += remaining
			written += remaining
			break
		}

		copy(e.curChunk[e.chunkWritePos:], p[written:written+n])
		e.chunkWritePos += n
		written += n
		remaining -= n

		err := e.flushChunk(false)
		if err != nil {
			// There is an inherent problem with Writer interface in golang as in what should a buffering Writer
			// return as a number of written bytes in case of an error. Seems that no users actually care
			// about it, anyway.
			return written, err
		}
	}
	return written, nil
}

func (e *aeadEncrypter) Close() error {
	if e.closed {
		return nil
	}

	err := e.flushChunk(true)
	if err != nil {
		return err
	}
	err = e.writer.Close()
	if err != nil {
		return err
	}
	e.close(e.plaintextSize)
	return nil
}

func (e *aeadEncrypter) Metadata() (string, error) {
	return e.metadata()
}

func (e *aeadEncrypter) KeyID() string {
	return e.keyID()
}

func (e *aeadEncrypter) CiphertextSize(plaintextSize int64) int64 {
	return ciphertextSize(plaintextSize, e.chunkSize, e.chunkTagSize)
}

func (e *aeadEncrypter) initCustomKeyEncrypter(
	keyID string, aad []byte, kmsClient kmslbclient.KMSClient, writer io.WriteCloser,
	options AeadEncrypterOptions, callCreds *kmslbclient.CallCredentials,
) error {
	err := e.initCustomKeyEncrypterCommon(keyID, aad, kmsClient, options, callCreds)
	if err != nil {
		return err
	}

	e.writer = writer
	e.curChunk = make([]byte, e.chunkSize)
	return nil
}

func (e *aeadEncrypter) initDefaultKeyEncrypter(
	dks DefaultKeysStorage, defaultKeyID string, aad []byte, writer io.WriteCloser, options AeadEncrypterOptions,
) error {
	err := e.initDefaultKeyEncrypterCommon(dks, defaultKeyID, aad, options)
	if err != nil {
		return err
	}

	e.writer = writer
	e.curChunk = make([]byte, e.chunkSize)
	return nil
}

func (e *aeadEncrypter) flushChunk(isLastChunk bool) error {
	fillNonceBuf(e.nonceBuf, noncePrefixSize, e.curChunkNum, isLastChunk)
	e.aead.Seal(e.curChunk[:0], e.nonceBuf, e.curChunk[:e.chunkWritePos], nil)

	_, err := e.writer.Write(e.curChunk[:e.chunkWritePos+e.chunkTagSize])
	if err != nil {
		return err
	}
	e.plaintextSize += int64(e.chunkWritePos)
	e.chunkWritePos = 0
	e.curChunkNum++
	if e.curChunkNum >= maxNumChunks {
		return fmt.Errorf("maximum number of chunks hit: %d", maxNumChunks)
	}
	return nil
}

// Returns a new StreamingEncrypter for given reader. keyID is a key id to be passed to kmsClient,
// aad is either additional authenticated data or nil, kmsClient is used to encrypt the data encryption key,
// writer is the ciphertext reader.
func NewStreamingAeadEncrypter(
	keyID string, aad []byte, kmsClient kmslbclient.KMSClient,
	writer io.WriteCloser, callCreds *kmslbclient.CallCredentials,
) (StreamingEncrypter, error) {
	return NewStreamingAeadEncrypterWithOptions(keyID, aad, kmsClient, writer, AeadEncrypterOptions{}, callCreds)
}

// Returns a new StreamingEncrypter with given encrypter options.
func NewStreamingAeadEncrypterWithOptions(
	keyID string, aad []byte, kmsClient kmslbclient.KMSClient, writer io.WriteCloser,
	options AeadEncrypterOptions, callCreds *kmslbclient.CallCredentials,
) (StreamingEncrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	e := &aeadEncrypter{}
	err := e.initCustomKeyEncrypter(keyID, aad, kmsClient, writer, options, callCreds)
	if err != nil {
		return nil, err
	}
	return e, nil
}

// Returns a new StreamingEncrypter with given encrypter options.
func NewStreamingAeadDefaultEncrypterWithOptions(
	dks DefaultKeysStorage, defaultKeyID string, aad []byte, writer io.WriteCloser,
	options AeadEncrypterOptions,
) (StreamingEncrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	e := &aeadEncrypter{}
	err := e.initDefaultKeyEncrypter(dks, defaultKeyID, aad, writer, options)
	if err != nil {
		return nil, err
	}
	return e, nil
}
