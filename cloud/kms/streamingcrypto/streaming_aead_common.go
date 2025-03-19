package streamingcrypto

import (
	"context"
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"crypto/sha256"
	"encoding/base64"
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"hash"
	"io"
	"strconv"

	"github.com/golang/protobuf/proto"
	"golang.org/x/crypto/chacha20poly1305"
	"golang.org/x/crypto/hkdf"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient"
	"a.yandex-team.ru/cloud/kms/streamingcrypto/proto/metadata"
)

type AEADAlgorithm int

const (
	AesGcm128 = iota + 1
	AesGcm256
	ChaCha20Poly1305
)

type AeadEncrypterOptions struct {
	Algorithm AEADAlgorithm
	ChunkSize int
}

// Export restrictions: see https://st.yandex-team.ru/CLOUD-103550 for more info on this whole mess.

// disableForExportFlag can be set by passing the flag -X=a.yandex-team.ru/cloud/kms/streamingcrypto.disableForExportFlag=nocryptoforyou
// to go link command. Please set the flag to some unique value so that you can later check that it is indeed set by grepping the binary.
var disableForExportFlag = ""

const disableForExportError = "streamingcrypto disabled due to export restrictions"

const (
	defaultAlgorithm = AesGcm256
	defaultChunkSize = 4096

	// All algorithms have the same nonce and tag sizes.
	noncePrefixSize = 7
	nonceSize       = 12
	tagSize         = 16

	chunkSizeMeta     = "chunkSize"
	tagSizeMeta       = "tagSize"
	algorithmMeta     = "algorithm"
	hkdfMeta          = "hkdf"
	keyURIMeta        = "keyUri"
	encryptedIkmMeta  = "eikm"
	saltMeta          = "salt"
	noncePrefixMeta   = "noncePrefix"
	plaintextSizeMeta = "plaintext"

	aesGcm128Name        = "AES128-GCM"
	aesGcm256Name        = "AES256-GCM"
	chaCha20Poly1305Name = "CHACHA20-POLY1305"
	hkdfSha256Name       = "SHA-256"

	maxNumChunks = 0xfffffffe // 2^32 - 2
)

func deriveKey(hash func() hash.Hash, ikm []byte, salt []byte, aad []byte, keySize int) ([]byte, error) {
	r := hkdf.New(hash, ikm, salt, aad)
	derived := make([]byte, keySize)
	_, err := io.ReadFull(r, derived)
	if err != nil {
		return nil, err
	}
	return derived, nil
}

func fillNonceBuf(nonceBuf []byte, prefixSize int, chunkNum uint32, isLastChunk bool) {
	binary.LittleEndian.PutUint32(nonceBuf[prefixSize:], chunkNum)
	if isLastChunk {
		nonceBuf[prefixSize+4] = 1
	} else {
		nonceBuf[prefixSize+4] = 0
	}
}

func algorithmToString(algorithm AEADAlgorithm) (string, error) {
	switch algorithm {
	case AesGcm128:
		return aesGcm128Name, nil
	case AesGcm256:
		return aesGcm256Name, nil
	case ChaCha20Poly1305:
		return chaCha20Poly1305Name, nil
	default:
		return "", fmt.Errorf("unknown algorithm %d", algorithm)
	}
}

func algorithmToPbAlgorithm(algorithm AEADAlgorithm) (metadata.Algorithm, error) {
	switch algorithm {
	case AesGcm128:
		return metadata.Algorithm_AES128_GCM, nil
	case AesGcm256:
		return metadata.Algorithm_AES256_GCM, nil
	case ChaCha20Poly1305:
		return metadata.Algorithm_CHACHA20_POLY1305, nil
	default:
		return -1, fmt.Errorf("unknown algorithm %d", algorithm)
	}
}

func algorithmFromPbAlgorithm(algorithm metadata.Algorithm) (AEADAlgorithm, error) {
	switch algorithm {
	case metadata.Algorithm_AES128_GCM:
		return AesGcm128, nil
	case metadata.Algorithm_AES256_GCM:
		return AesGcm256, nil
	case metadata.Algorithm_CHACHA20_POLY1305:
		return ChaCha20Poly1305, nil
	default:
		return -1, fmt.Errorf("unknown algorithm %d", algorithm)
	}
}

func algorithmFromString(s string) (AEADAlgorithm, error) {
	switch s {
	case aesGcm128Name:
		return AesGcm128, nil
	case aesGcm256Name:
		return AesGcm256, nil
	case chaCha20Poly1305Name:
		return ChaCha20Poly1305, nil
	default:
		return -1, fmt.Errorf("unknown algorithm " + s)
	}
}

func keySizeFromAlgorithm(algorithm AEADAlgorithm) (int, error) {
	switch algorithm {
	case AesGcm128:
		return 16, nil
	case AesGcm256, ChaCha20Poly1305:
		return 32, nil
	default:
		return 0, fmt.Errorf("unknown algorithm %d", algorithm)
	}
}

func saltSizeForAlgorithm(algorithm AEADAlgorithm) (int, error) {
	switch algorithm {
	case AesGcm128, AesGcm256, ChaCha20Poly1305:
		return 16, nil
	default:
		return 0, fmt.Errorf("unknown algorithm %d", algorithm)
	}
}

func aeadForAlgorithm(algorithm AEADAlgorithm, key []byte) (cipher.AEAD, error) {
	switch algorithm {
	case AesGcm128, AesGcm256:
		block, err := aes.NewCipher(key)
		if err != nil {
			return nil, err
		}
		return cipher.NewGCM(block)
	case ChaCha20Poly1305:
		return chacha20poly1305.New(key)
	default:
		return nil, fmt.Errorf("unknown algorithm %d", int(algorithm))
	}
}

func ciphertextSize(plaintextSize int64, chunkSize int, chunkTagSize int) int64 {
	if plaintextSize < 0 {
		return 0
	} else if plaintextSize == 0 {
		// We want to always have at least one authentication tag even for empty inputs.
		return int64(chunkTagSize)
	}

	chunkPayload := int64(chunkSize - chunkTagSize)
	numFullChunks := plaintextSize / chunkPayload
	ret := numFullChunks * int64(chunkSize)
	lastChunkSize := plaintextSize % chunkPayload
	if lastChunkSize > 0 {
		ret += lastChunkSize + int64(chunkTagSize)
	}
	return ret
}

type aeadEncrypterCommon struct {
	pbMetadata *metadata.CryptoMetadata
	closed     bool

	chunkSize    int
	chunkTagSize int

	algorithm AEADAlgorithm
	aead      cipher.AEAD
	nonceBuf  []byte
}

func (e *aeadEncrypterCommon) metadata() (string, error) {
	if !e.closed {
		return "", fmt.Errorf("Close() must be called before Metadata()")
	}

	metaBytes, err := proto.Marshal(e.pbMetadata)
	if err != nil {
		return "", fmt.Errorf("unable to serialize proto message: %v", err)
	}
	return base64.StdEncoding.EncodeToString(metaBytes), nil
}

func (e *aeadEncrypterCommon) keyID() string {
	if customMeta := e.pbMetadata.GetCustomKeyMetadata(); customMeta != nil {
		return customMeta.KeyId
	} else if defaultMeta := e.pbMetadata.GetDefaultMetadata(); defaultMeta != nil {
		return defaultMeta.KeyId
	} else {
		panic("metadata unspecified")
	}
}

func (e *aeadEncrypterCommon) initCustomKeyEncrypterCommon(
	keyID string, aad []byte, kmsClient kmslbclient.KMSClient,
	options AeadEncrypterOptions, callCreds *kmslbclient.CallCredentials,
) error {
	if options.Algorithm == 0 {
		options.Algorithm = defaultAlgorithm
	}
	if options.ChunkSize == 0 {
		options.ChunkSize = defaultChunkSize
	}
	if options.ChunkSize <= tagSize {
		return fmt.Errorf("ChunkSize is too small: minimum size required is %d", tagSize+1)
	}

	customKeyMeta := &metadata.CustomKeyMetadata{}
	e.pbMetadata = &metadata.CryptoMetadata{
		Data: &metadata.CryptoMetadata_CustomKeyMetadata{CustomKeyMetadata: customKeyMeta},
	}

	e.chunkSize = options.ChunkSize
	customKeyMeta.ChunkSize = int32(e.chunkSize)
	e.chunkTagSize = tagSize
	customKeyMeta.TagSize = int32(e.chunkTagSize)
	e.algorithm = options.Algorithm
	var err error
	if customKeyMeta.Algorithm, err = algorithmToPbAlgorithm(options.Algorithm); err != nil {
		return err
	}
	customKeyMeta.KeyId = keyID

	saltSize, err := saltSizeForAlgorithm(options.Algorithm)
	if err != nil {
		return err
	}
	salt := make([]byte, saltSize)
	_, err = rand.Read(salt)
	if err != nil {
		return err
	}
	customKeyMeta.Salt = salt

	noncePrefix := make([]byte, noncePrefixSize)
	_, err = rand.Read(noncePrefix)
	if err != nil {
		return err
	}
	// Init nonce prefix
	e.nonceBuf = make([]byte, nonceSize)
	copy(e.nonceBuf, noncePrefix)
	customKeyMeta.NoncePrefix = noncePrefix

	keySize, err := keySizeFromAlgorithm(options.Algorithm)
	if err != nil {
		return err
	}
	ikm := make([]byte, keySize)
	_, err = rand.Read(ikm)
	if err != nil {
		return err
	}
	encryptedIkm, err := kmsClient.Encrypt(context.Background(), keyID, ikm, aad, callCreds)
	if err != nil {
		return err
	}
	customKeyMeta.Eikm = encryptedIkm

	key, err := deriveKey(sha256.New, ikm, salt, aad, keySize)
	if err != nil {
		return err
	}
	customKeyMeta.Hkdf = metadata.Hkdf_SHA_256

	e.aead, err = aeadForAlgorithm(e.algorithm, key)
	if err != nil {
		return err
	}

	return nil
}

func (e *aeadEncrypterCommon) initDefaultKeyEncrypterCommon(
	dks DefaultKeysStorage, defaultKeyID string, aad []byte, options AeadEncrypterOptions,
) error {
	if options.Algorithm == 0 {
		options.Algorithm = defaultAlgorithm
	}
	if options.ChunkSize == 0 {
		options.ChunkSize = defaultChunkSize
	}
	if options.ChunkSize <= tagSize {
		return fmt.Errorf("ChunkSize is too small: minimum size required is %d", tagSize+1)
	}

	defaultKeyMeta := &metadata.DefaultMetadata{}
	e.pbMetadata = &metadata.CryptoMetadata{
		Data: &metadata.CryptoMetadata_DefaultMetadata{DefaultMetadata: defaultKeyMeta},
	}

	e.chunkSize = options.ChunkSize
	defaultKeyMeta.ChunkSize = int32(e.chunkSize)
	e.chunkTagSize = tagSize
	defaultKeyMeta.TagSize = int32(e.chunkTagSize)
	e.algorithm = options.Algorithm
	var err error
	if defaultKeyMeta.Algorithm, err = algorithmToPbAlgorithm(options.Algorithm); err != nil {
		return err
	}
	var defaultKey DefaultKey
	if defaultKeyID == "" {
		defaultKey, err = dks.GetPrimaryDefaultKey()
	} else {
		defaultKey, err = dks.GetDefaultKeyByID(defaultKeyID)
	}
	if err != nil {
		return err
	}
	defaultKeyMeta.KeyId = defaultKey.id

	saltSize, err := saltSizeForAlgorithm(options.Algorithm)
	if err != nil {
		return err
	}
	salt := make([]byte, saltSize)
	_, err = rand.Read(salt)
	if err != nil {
		return err
	}
	defaultKeyMeta.Salt = salt

	noncePrefix := make([]byte, noncePrefixSize)
	_, err = rand.Read(noncePrefix)
	if err != nil {
		return err
	}
	// Init nonce prefix
	e.nonceBuf = make([]byte, nonceSize)
	copy(e.nonceBuf, noncePrefix)
	defaultKeyMeta.NoncePrefix = noncePrefix

	keySize, err := keySizeFromAlgorithm(options.Algorithm)
	if err != nil {
		return err
	}
	ikm := make([]byte, len(defaultKey.contents))
	copy(ikm, defaultKey.contents)

	key, err := deriveKey(sha256.New, ikm, salt, aad, keySize)
	if err != nil {
		return err
	}
	defaultKeyMeta.Hkdf = metadata.Hkdf_SHA_256

	e.aead, err = aeadForAlgorithm(e.algorithm, key)
	if err != nil {
		return err
	}

	return nil
}

func (e *aeadEncrypterCommon) close(plaintextSize int64) {
	if customMeta := e.pbMetadata.GetCustomKeyMetadata(); customMeta != nil {
		customMeta.PlaintextSize = plaintextSize
	} else if defaultMeta := e.pbMetadata.GetDefaultMetadata(); defaultMeta != nil {
		defaultMeta.PlaintextSize = plaintextSize
	} else {
		panic("metadata unspecified")
	}
	e.closed = true
}

type aeadDecrypterCommon struct {
	failOnOldMetadata bool
	chunkSize         int
	chunkTagSize      int

	plaintextSize int64
	totalChunks   uint32

	aead     cipher.AEAD
	nonceBuf []byte
}

func (d *aeadDecrypterCommon) initDecrypterCommon(
	metadata string,
	aad []byte,
	dks DefaultKeysStorage,
	kmsClient kmslbclient.KMSClient,
	callCreds *kmslbclient.CallCredentials,
) error {
	protoBytes, err := base64.StdEncoding.DecodeString(metadata)
	if err == nil {
		return d.initDecrypterPbMetadata(protoBytes, aad, dks, kmsClient, callCreds)
	} else if !d.failOnOldMetadata {
		return d.initDecrypterJSONMetadata(metadata, aad, kmsClient, callCreds)
	} else {
		return errors.New("old json metadata format not supported")
	}
}

func (d *aeadDecrypterCommon) readChunkCommon(reader io.Reader, chunkNum uint32, curChunk []byte) (int, error) {
	isLastChunk := chunkNum == d.totalChunks-1
	curChunkLen := d.chunkSize
	if isLastChunk {
		curChunkLen = d.lastChunkSize()
	}
	n, err := io.ReadFull(reader, curChunk[:curChunkLen])
	if err != nil {
		return 0, err
	}
	if n != curChunkLen {
		return 0, fmt.Errorf("mismatched chunk length and read size: %d vs %d", curChunkLen, n)
	}

	fillNonceBuf(d.nonceBuf, noncePrefixSize, chunkNum, isLastChunk)
	_, err = d.aead.Open(curChunk[:0], d.nonceBuf, curChunk[:curChunkLen], nil)
	if err != nil {
		return 0, fmt.Errorf("error decrypting chunk %d: %s", chunkNum, err)
	}
	return curChunkLen, nil
}

func (d *aeadDecrypterCommon) lastChunkSize() int {
	if d.plaintextSize == 0 {
		return 0
	}
	chunkPayload := int64(d.chunkSize - d.chunkTagSize)
	modulo := int(d.plaintextSize % chunkPayload)
	if modulo == 0 {
		return int(chunkPayload) + d.chunkTagSize
	}
	return modulo + d.chunkTagSize
}

func (d *aeadDecrypterCommon) initDecrypterPbMetadata(
	metaBytes []byte, aad []byte, dks DefaultKeysStorage, kmsClient kmslbclient.KMSClient, callCreds *kmslbclient.CallCredentials,
) error {
	var meta metadata.CryptoMetadata
	err := proto.Unmarshal(metaBytes, &meta)
	if err != nil {
		return fmt.Errorf("unable to deserialize metadata proto message: %v", err)
	}
	if customKeyMeta := meta.GetCustomKeyMetadata(); customKeyMeta != nil {
		return d.initCustomKeyDecrypter(customKeyMeta, aad, kmsClient, callCreds)
	} else if defaultKeyMeta := meta.GetDefaultMetadata(); defaultKeyMeta != nil {
		return d.initDefaultKeyDecrypter(defaultKeyMeta, aad, dks)
	} else {
		return errors.New("invalid metadata, custom key or default key metadata supported")
	}
}

func (d *aeadDecrypterCommon) initDecrypterJSONMetadata(
	metadata string, aad []byte, kmsClient kmslbclient.KMSClient, callCreds *kmslbclient.CallCredentials,
) error {
	var metamap map[string]string
	err := json.Unmarshal([]byte(metadata), &metamap)
	if err != nil {
		return err
	}
	d.chunkSize, err = strconv.Atoi(metamap[chunkSizeMeta])
	if err != nil {
		return fmt.Errorf("bad chunkSize: " + metamap[chunkSizeMeta])
	}
	d.chunkTagSize, err = strconv.Atoi(metamap[tagSizeMeta])
	if err != nil {
		return fmt.Errorf("bad tagSize: " + metamap[tagSizeMeta])
	}

	d.plaintextSize, err = strconv.ParseInt(metamap[plaintextSizeMeta], 10, 64)
	if err != nil {
		return fmt.Errorf("bad plaintext size: " + metamap[plaintextSizeMeta])
	}
	if d.plaintextSize < 0 {
		return fmt.Errorf("bad plaintext size: %d", d.plaintextSize)
	}
	if d.plaintextSize > 0 {
		chunkPayload := int64(d.chunkSize - d.chunkTagSize)
		d.totalChunks = uint32((d.plaintextSize-1)/chunkPayload) + 1
	}

	keyID := metamap[keyURIMeta]
	if keyID == "" {
		return fmt.Errorf("missing keyUri")
	}

	algorithm, err := algorithmFromString(metamap[algorithmMeta])
	if err != nil {
		return nil
	}
	if metamap[hkdfMeta] != hkdfSha256Name {
		return fmt.Errorf("incompatible hkdf: " + metamap[hkdfMeta])
	}
	encryptedIkm, err := base64.StdEncoding.DecodeString(metamap[encryptedIkmMeta])
	if err != nil {
		return fmt.Errorf("error decrypting ikm: " + err.Error())
	}
	salt, err := base64.StdEncoding.DecodeString(metamap[saltMeta])
	if err != nil {
		return fmt.Errorf("error decrypting salt: " + err.Error())
	}
	noncePrefix, err := base64.StdEncoding.DecodeString(metamap[noncePrefixMeta])
	if err != nil {
		return fmt.Errorf("error decrypting noncePrefix: " + err.Error())
	}
	if len(noncePrefix) != noncePrefixSize {
		return fmt.Errorf("incorrect nonce prefix len: " + metamap[noncePrefixMeta])
	}

	ikm, err := kmsClient.Decrypt(context.Background(), keyID, encryptedIkm, aad, callCreds)
	if err != nil {
		return err
	}

	keySize, err := keySizeFromAlgorithm(algorithm)
	if err != nil {
		return err
	}
	if len(ikm) != keySize {
		return fmt.Errorf("mismatch between ikm and algorithm sizes: %d vs %d", len(ikm), keySize)
	}

	key, err := deriveKey(sha256.New, ikm, salt, aad, keySize)
	if err != nil {
		return err
	}

	d.nonceBuf = make([]byte, nonceSize)
	copy(d.nonceBuf, noncePrefix)

	d.aead, err = aeadForAlgorithm(algorithm, key)
	if err != nil {
		return err
	}
	return nil
}

func (d *aeadDecrypterCommon) initCustomKeyDecrypter(
	customKeyMeta *metadata.CustomKeyMetadata, aad []byte, kmsClient kmslbclient.KMSClient, callCreds *kmslbclient.CallCredentials,
) error {
	d.chunkSize = int(customKeyMeta.ChunkSize)
	d.chunkTagSize = int(customKeyMeta.TagSize)
	d.plaintextSize = customKeyMeta.PlaintextSize
	if d.plaintextSize < 0 {
		return fmt.Errorf("bad plaintext size: %d", d.plaintextSize)
	}
	if d.plaintextSize > 0 {
		chunkPayload := int64(d.chunkSize - d.chunkTagSize)
		d.totalChunks = uint32((d.plaintextSize-1)/chunkPayload) + 1
	}
	if customKeyMeta.KeyId == "" {
		return fmt.Errorf("missing keyId")
	}
	algorithm, err := algorithmFromPbAlgorithm(customKeyMeta.Algorithm)
	if err != nil {
		return err
	}

	if customKeyMeta.Hkdf != metadata.Hkdf_SHA_256 {
		return fmt.Errorf("incompatible hkdf: " + customKeyMeta.Hkdf.String())
	}
	if len(customKeyMeta.NoncePrefix) != noncePrefixSize {
		return fmt.Errorf("incorrect nonce prefix len: " + strconv.Itoa(len(customKeyMeta.NoncePrefix)))
	}

	ikm, err := kmsClient.Decrypt(context.Background(), customKeyMeta.KeyId, customKeyMeta.Eikm, aad, callCreds)
	if err != nil {
		return err
	}

	keySize, err := keySizeFromAlgorithm(algorithm)
	if err != nil {
		return err
	}
	if len(ikm) != keySize {
		return fmt.Errorf("mismatch between ikm and algorithm sizes: %d vs %d", len(ikm), keySize)
	}

	key, err := deriveKey(sha256.New, ikm, customKeyMeta.Salt, aad, keySize)
	if err != nil {
		return err
	}

	d.nonceBuf = make([]byte, nonceSize)
	copy(d.nonceBuf, customKeyMeta.NoncePrefix)

	d.aead, err = aeadForAlgorithm(algorithm, key)
	if err != nil {
		return err
	}
	return nil
}

func (d *aeadDecrypterCommon) initDefaultKeyDecrypter(
	defaultMetadata *metadata.DefaultMetadata, aad []byte, dks DefaultKeysStorage,
) error {
	if dks == nil {
		return errors.New("default keys storage is not provided")
	}
	d.chunkSize = int(defaultMetadata.ChunkSize)
	d.chunkTagSize = int(defaultMetadata.TagSize)
	d.plaintextSize = defaultMetadata.PlaintextSize
	if d.plaintextSize < 0 {
		return fmt.Errorf("bad plaintext size: %d", d.plaintextSize)
	}
	if d.plaintextSize > 0 {
		chunkPayload := int64(d.chunkSize - d.chunkTagSize)
		d.totalChunks = uint32((d.plaintextSize-1)/chunkPayload) + 1
	}
	if defaultMetadata.KeyId == "" {
		return fmt.Errorf("missing keyId")
	}
	defaultKey, err := dks.GetDefaultKeyByID(defaultMetadata.KeyId)
	if err != nil {
		return err
	}
	algorithm, err := algorithmFromPbAlgorithm(defaultMetadata.Algorithm)
	if err != nil {
		return err
	}
	if defaultMetadata.Hkdf != metadata.Hkdf_SHA_256 {
		return fmt.Errorf("incompatible hkdf: " + defaultMetadata.Hkdf.String())
	}
	if len(defaultMetadata.NoncePrefix) != noncePrefixSize {
		return fmt.Errorf("incorrect nonce prefix len: " + strconv.Itoa(len(defaultMetadata.NoncePrefix)))
	}

	ikm := make([]byte, len(defaultKey.contents))
	copy(ikm, defaultKey.contents)

	keySize, err := keySizeFromAlgorithm(algorithm)
	if err != nil {
		return err
	}

	key, err := deriveKey(sha256.New, ikm, defaultMetadata.Salt, aad, keySize)
	if err != nil {
		return err
	}

	d.nonceBuf = make([]byte, nonceSize)
	copy(d.nonceBuf, defaultMetadata.NoncePrefix)

	d.aead, err = aeadForAlgorithm(algorithm, key)
	if err != nil {
		return err
	}
	return nil
}
