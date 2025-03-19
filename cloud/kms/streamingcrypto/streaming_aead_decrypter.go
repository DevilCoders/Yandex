package streamingcrypto

import (
	"errors"
	"io"
	"strconv"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
)

type aeadDecrypter struct {
	aeadDecrypterCommon

	reader ReadSeekerCloser

	curChunk []byte
	// len(chunk) == chunkSize, chunkLen is the actual length of chunk, can be less than chunkSize for the last chunk
	curChunkLen  int
	curChunkNum  uint32
	plaintextPos int64
}

func (d *aeadDecrypter) Close() error {
	return d.reader.Close()
}

func (d *aeadDecrypter) Read(p []byte) (int, error) {
	chunkPayload := int64(d.chunkSize - d.chunkTagSize)
	read := 0
	remaining := len(p)
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
			return read, io.EOF
		}
		if remaining < n {
			n = remaining
		}
		copy(p[read:], d.curChunk[chunkPos:chunkPos+n])
		d.plaintextPos += int64(n)
		read += n
		remaining -= n
	}
	return read, nil
}

func (d *aeadDecrypter) readChunk(chunkNum uint32) error {
	if chunkNum >= d.totalChunks {
		return io.EOF
	}
	var err error
	if chunkNum != d.curChunkNum+1 {
		_, err = d.reader.Seek(int64(d.chunkSize)*int64(chunkNum), io.SeekStart)
		if err != nil {
			return err
		}
	}
	d.curChunkLen, err = d.readChunkCommon(d.reader, chunkNum, d.curChunk)
	if err == nil || err == io.EOF {
		d.curChunkNum = chunkNum
	}
	return err
}

func (d *aeadDecrypter) Seek(offset int64, whence int) (int64, error) {
	switch whence {
	case io.SeekStart:
		d.plaintextPos = offset
	case io.SeekCurrent:
		d.plaintextPos += offset
	case io.SeekEnd:
		d.plaintextPos = d.plaintextSize + offset
	default:
		panic("unknown whence " + strconv.Itoa(whence))
	}
	return d.plaintextPos, nil
}

func (d *aeadDecrypter) initDecrypter(
	metadata string, aad []byte, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient,
	reader ReadSeekerCloser, callCreds *kmslbclient.CallCredentials,
) error {
	err := d.initDecrypterCommon(metadata, aad, dks, kmsClient, callCreds)
	if err != nil {
		return err
	}

	d.reader = reader

	d.curChunk = make([]byte, d.chunkSize)
	d.curChunkNum = maxNumChunks

	return nil
}

// Returns a new StreamingRangeDecrypter for given reader. metadata must be the result of StreamingEncrypter/StreamingReadEncrypter
// Metadata() method. aad is either additional authenticated data or nil, must be the same as the aad used for encrypting the plaintext,
// kmsClient is used to decrypt the data encryption key, reader is the ciphertext reader.
func NewStreamingAeadDecrypter(
	metadata string, aad []byte, kmsClient kmslbclient.KMSClient, reader ReadSeekerCloser, callCreds *kmslbclient.CallCredentials,
) (StreamingDecrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	d := &aeadDecrypter{}
	err := d.initDecrypter(metadata, aad, nil, kmsClient, reader, callCreds)
	if err != nil {
		return nil, err
	}
	return d, nil
}

func NewStreamingAeadDecrypterWithMetadataFlag(
	metadata string, aad []byte, kmsClient kmslbclient.KMSClient, reader ReadSeekerCloser,
	callCreds *kmslbclient.CallCredentials, failOnOldMetadata bool,
) (StreamingDecrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	d := &aeadDecrypter{}
	d.failOnOldMetadata = failOnOldMetadata
	err := d.initDecrypter(metadata, aad, nil, kmsClient, reader, callCreds)
	if err != nil {
		return nil, err
	}
	return d, nil
}

func NewStreamingAeadDecrypterWithMetadataFlagAndDefaultKey(
	metadata string, aad []byte, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient, reader ReadSeekerCloser,
	callCreds *kmslbclient.CallCredentials, failOnOldMetadata bool,
) (StreamingDecrypter, error) {
	if disableForExportFlag != "" {
		return nil, errors.New(disableForExportError)
	}

	d := &aeadDecrypter{}
	d.failOnOldMetadata = failOnOldMetadata
	err := d.initDecrypter(metadata, aad, dks, kmsClient, reader, callCreds)
	if err != nil {
		return nil, err
	}
	return d, nil
}
