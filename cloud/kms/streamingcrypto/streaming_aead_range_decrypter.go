package streamingcrypto

import (
	"errors"
	"fmt"
	"io"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
)

type aeadRangeDecrypter struct {
	aeadDecrypterCommon
}

func (d *aeadRangeDecrypter) Close() error {
	return nil
}

func (d *aeadRangeDecrypter) CiphertextRange(ptStart, ptStop int64) (start int64, stop int64, err error) {
	if ptStart < 0 || ptStop < 0 || ptStop < ptStart {
		err = fmt.Errorf("bad plaintextRange: %d, %d", ptStart, ptStop)
		return
	}

	chunkPayload := int64(d.chunkSize - d.chunkTagSize)
	startChunkNum := ptStart / chunkPayload
	start = startChunkNum * int64(d.chunkSize)
	endChunkNum := ptStop / chunkPayload
	endChunkOffset := ptStop % chunkPayload
	if endChunkOffset > 0 {
		endChunkNum++
	}
	stop = endChunkNum * int64(d.chunkSize)

	maxRange := ciphertextSize(d.plaintextSize, d.chunkSize, d.chunkTagSize)
	if start > maxRange {
		start = maxRange
	}
	if stop > maxRange {
		stop = maxRange
	}
	return
}

type aeadSubrangeDecryptor struct {
	aeadDecrypterCommon

	reader io.ReadCloser

	curChunk []byte
	// len(chunk) == chunkSize, chunkLen is the actual length of chunk, can be less than chunkSize for the last chunk
	curChunkLen  int
	curChunkNum  uint32
	plaintextPos int64
	plaintextEnd int64
}

func (d *aeadSubrangeDecryptor) Read(p []byte) (int, error) {
	chunkPayload := int64(d.chunkSize - d.chunkTagSize)
	read := 0
	remaining := len(p)
	remainingPlaintext := int(d.plaintextEnd - d.plaintextPos)
	readUntilEOF := false
	if remaining >= remainingPlaintext {
		remaining = remainingPlaintext
		readUntilEOF = true
	}

	for remaining > 0 {
		chunkNum := uint32(d.plaintextPos / chunkPayload)
		chunkPos := int(d.plaintextPos % chunkPayload)
		if chunkNum != d.curChunkNum {
			err := d.readChunk(chunkNum)
			if err != nil {
				return read, err
			}
		}
		n := d.curChunkLen - d.chunkTagSize - chunkPos
		if n <= 0 {
			return read, io.ErrUnexpectedEOF
		}
		if remaining < n {
			n = remaining
		}
		copy(p[read:], d.curChunk[chunkPos:chunkPos+n])
		d.plaintextPos += int64(n)
		read += n
		remaining -= n
	}
	if readUntilEOF {
		return read, io.EOF
	}
	return read, nil
}

func (d *aeadSubrangeDecryptor) readChunk(chunkNum uint32) error {
	if chunkNum >= d.totalChunks {
		// We always trim the read range in Read() so that we should never get EOF from reader.
		return io.ErrUnexpectedEOF
	}
	if d.curChunkNum != maxNumChunks && d.curChunkNum != chunkNum-1 {
		return fmt.Errorf("expected to read either chunk 0 or chunk %d, got %d", d.curChunkNum+1, chunkNum)
	}
	var err error
	d.curChunkLen, err = d.readChunkCommon(d.reader, chunkNum, d.curChunk)
	if err == nil || err == io.EOF {
		d.curChunkNum = chunkNum
	}
	return err
}

func (d *aeadSubrangeDecryptor) Close() error {
	return d.reader.Close()
}

func (d *aeadRangeDecrypter) Plaintext(plaintextRange Range, ciphertextReader io.ReadCloser) (io.ReadCloser, error) {
	if plaintextRange.Start < 0 || plaintextRange.Stop < 0 || plaintextRange.Stop < plaintextRange.Start {
		return nil, fmt.Errorf("bad plaintextRange: %d, %d", plaintextRange.Start, plaintextRange.Stop)
	}

	ret := &aeadSubrangeDecryptor{}
	ret.chunkSize = d.chunkSize
	ret.chunkTagSize = d.chunkTagSize
	ret.plaintextSize = d.plaintextSize
	ret.totalChunks = d.totalChunks
	ret.aead = d.aead
	ret.nonceBuf = make([]byte, len(d.nonceBuf))
	copy(ret.nonceBuf, d.nonceBuf)
	ret.reader = ciphertextReader
	ret.curChunk = make([]byte, d.chunkSize)
	ret.curChunkNum = maxNumChunks
	ret.plaintextPos = plaintextRange.Start
	if ret.plaintextPos > ret.plaintextSize {
		ret.plaintextPos = ret.plaintextSize
	}
	ret.plaintextEnd = plaintextRange.Stop
	if ret.plaintextEnd > ret.plaintextSize {
		ret.plaintextEnd = ret.plaintextSize
	}
	return ret, nil
}

func (d *aeadRangeDecrypter) initRangeDecrypter(
	metadata string, aad []byte, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient, callCreds *kmslbclient.CallCredentials,
) error {
	return d.initDecrypterCommon(metadata, aad, dks, kmsClient, callCreds)
}

// Returns a new StreamingRangeDecrypter. metadata must be the result of StreamingEncrypter/StreamingReadEncrypter
// Metadata() method. aad is either additional authenticated data or nil, must be the same as the aad used for encrypting the plaintext,
// kmsClient is used to decrypt the data encryption key.
func NewStreamingAeadRangeDecrypter(
	metadata string, aad []byte, kmsClient kmslbclient.KMSClient, callCreds *kmslbclient.CallCredentials,
) (StreamingRangeDecrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	d := &aeadRangeDecrypter{}
	err := d.initRangeDecrypter(metadata, aad, nil, kmsClient, callCreds)
	if err != nil {
		return nil, err
	}
	return d, nil
}

func NewStreamingAeadRangeDecrypterWithMetadataFlag(
	metadata string,
	aad []byte,
	kmsClient kmslbclient.KMSClient,
	callCreds *kmslbclient.CallCredentials,
	failOnOldMetadata bool,
) (StreamingRangeDecrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	d := &aeadRangeDecrypter{}
	d.failOnOldMetadata = failOnOldMetadata
	err := d.initRangeDecrypter(metadata, aad, nil, kmsClient, callCreds)
	if err != nil {
		return nil, err
	}
	return d, nil
}

func NewStreamingAeadRangeDecrypterWithMetadataFlagAndDefaultKey(
	metadata string,
	aad []byte,
	dks DefaultKeysStorage,
	kmsClient kmslbclient.KMSClient,
	callCreds *kmslbclient.CallCredentials,
	failOnOldMetadata bool,
) (StreamingRangeDecrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	d := &aeadRangeDecrypter{}
	d.failOnOldMetadata = failOnOldMetadata
	err := d.initRangeDecrypter(metadata, aad, dks, kmsClient, callCreds)
	if err != nil {
		return nil, err
	}
	return d, nil
}
