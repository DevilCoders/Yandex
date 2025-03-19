package crypto

import (
	"crypto"
	"crypto/rand"
	"crypto/rsa"
	"crypto/sha512"
	"crypto/x509"
	"encoding/pem"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Private/Public keys constants
const (
	privateKeyPemType  = "RSA PRIVATE KEY"
	publicKeyPemType   = "RSA PUBLIC KEY"
	certificatePemType = "CERTIFICATE"
	signatureHasher    = crypto.SHA512
	pssSaltLength      = rsa.PSSSaltLengthAuto
	keyBitSize         = 2048
	keyForTestBitSize  = 512 // unsecure! For tests only!
)

// PrivateKey type used for signing data
type PrivateKey rsa.PrivateKey

// PublicKey type used for verifying signed data
type PublicKey rsa.PublicKey

// GeneratePrivateKey generates private key
func GeneratePrivateKey() (*PrivateKey, error) {
	key, err := rsa.GenerateKey(rand.Reader, keyBitSize)
	return (*PrivateKey)(key), err
}

// GeneratePrivateKeyForTest generates private key
func GeneratePrivateKeyForTest() (*PrivateKey, error) {
	key, err := rsa.GenerateKey(rand.Reader, keyForTestBitSize)
	return (*PrivateKey)(key), err
}

// NewPrivateKey parses private key from PEM or PKCS8
func NewPrivateKey(bytes []byte) (*PrivateKey, error) {
	b, _ := pem.Decode(bytes)
	if b != nil {
		// Try decoding from what we got from PEM
		bytes = b.Bytes
	}

	if key, err := NewPrivateKeyFromPKCS1(bytes); err == nil {
		return key, err
	}

	return NewPrivateKeyFromPKCS8(bytes)
}

// NewPrivateKeyFromPKCS1 parses private key from PKCS1
func NewPrivateKeyFromPKCS1(bytes []byte) (*PrivateKey, error) {
	key, err := x509.ParsePKCS1PrivateKey(bytes)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse private key: %w", err)
	}

	return (*PrivateKey)(key), err
}

// NewPrivateKeyFromPKCS8 parses private key from PKCS8
func NewPrivateKeyFromPKCS8(bytes []byte) (*PrivateKey, error) {
	key, err := x509.ParsePKCS8PrivateKey(bytes)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse private key: %w", err)
	}

	pkey, ok := key.(*rsa.PrivateKey)
	if !ok {
		return nil, xerrors.New("private key is not RSA")
	}

	return (*PrivateKey)(pkey), nil
}

// NewPrivateKeyFromPEM parses private key from PEM
func NewPrivateKeyFromPEM(bytes []byte) (*PrivateKey, error) {
	b, _ := pem.Decode(bytes)
	if b == nil {
		return nil, xerrors.New("PEM not found")
	}

	return NewPrivateKeyFromPKCS8(b.Bytes)
}

// GetPublicKey returns public key associated with this private key
func (key *PrivateKey) GetPublicKey() *PublicKey {
	return (*PublicKey)(&key.PublicKey)
}

// Sign signs data
func (key *PrivateKey) Sign(data []byte) ([]byte, error) {
	return rsa.SignPSS(
		rand.Reader,
		(*rsa.PrivateKey)(key),
		signatureHasher,
		data,
		&rsa.PSSOptions{
			SaltLength: pssSaltLength,
			Hash:       signatureHasher,
		},
	)
}

// HashAndSign hashes data and signs it
func (key *PrivateKey) HashAndSign(data []byte) ([]byte, error) {
	hash := sha512.Sum512(data)
	return key.Sign(hash[:])
}

// EncodeToPKCS8 encodes private key to PKCS8
func (key *PrivateKey) EncodeToPKCS8() ([]byte, error) {
	return x509.MarshalPKCS8PrivateKey((*rsa.PrivateKey)(key))
}

// EncodeToPEM encodes private key to PEM
func (key *PrivateKey) EncodeToPEM() ([]byte, error) {
	bytes, err := key.EncodeToPKCS8()
	if err != nil {
		return nil, err
	}

	return pem.EncodeToMemory(
		&pem.Block{
			Type:  privateKeyPemType,
			Bytes: bytes,
		},
	), nil
}

// NewPublicKey parses public key from PEM or PKCS1
func NewPublicKey(bytes []byte) (*PublicKey, error) {
	b, _ := pem.Decode(bytes)
	if b != nil {
		// Try decoding from what we got from PEM
		bytes = b.Bytes
	}

	if key, err := NewPublicFromPKCS1(bytes); err == nil {
		return key, err
	}

	return NewPublicFromPKXI(bytes)
}

// NewPublicFromPKCS1 parses public key from PKCS1
func NewPublicFromPKCS1(bytes []byte) (*PublicKey, error) {
	key, err := x509.ParsePKCS1PublicKey(bytes)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse public key: %w", err)
	}

	return (*PublicKey)(key), err
}

// NewPublicFromPKXI parses public key from PKXI
func NewPublicFromPKXI(bytes []byte) (*PublicKey, error) {
	key, err := x509.ParsePKIXPublicKey(bytes)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse public key: %w", err)
	}

	pkey, ok := key.(*rsa.PublicKey)
	if !ok {
		return nil, xerrors.New("public key is not RSA")
	}

	return (*PublicKey)(pkey), nil
}

// NewPublicKeyFromPEM parses public key from PEM
func NewPublicKeyFromPEM(bytes []byte) (*PublicKey, error) {
	b, _ := pem.Decode(bytes)
	if b == nil {
		return nil, xerrors.New("PEM not found")
	}

	return NewPublicFromPKCS1(b.Bytes)
}

// Verify verifies data against signature
func (key *PublicKey) Verify(data []byte, signature []byte) error {
	return rsa.VerifyPSS(
		(*rsa.PublicKey)(key),
		signatureHasher,
		data,
		signature,
		&rsa.PSSOptions{
			SaltLength: pssSaltLength,
			Hash:       signatureHasher,
		},
	)
}

// HashAndVerify hashes data and verifies it against signature
func (key *PublicKey) HashAndVerify(data []byte, signature []byte) error {
	hash := sha512.Sum512(data)
	return key.Verify(hash[:], signature)
}

// EncodeToPKCS1 encodes public key to PKCS1
func (key *PublicKey) EncodeToPKCS1() []byte {
	return x509.MarshalPKCS1PublicKey((*rsa.PublicKey)(key))
}

// EncodeToPEM encodes public key to PEM
func (key *PublicKey) EncodeToPEM() []byte {
	return pem.EncodeToMemory(
		&pem.Block{
			Type:  publicKeyPemType,
			Bytes: key.EncodeToPKCS1(),
		},
	)
}

// CertificatesFromPem parse certificate from pem
func CertificatesFromPem(pemData []byte) ([]*x509.Certificate, error) {
	var certBlocks []byte

	block, rest := pem.Decode(pemData)
	for block != nil {
		if block.Type == certificatePemType {
			certBlocks = append(certBlocks, block.Bytes...)
			if len(rest) == 0 {
				break
			}
		}
		block, rest = pem.Decode(rest)
	}

	return x509.ParseCertificates(certBlocks)
}
