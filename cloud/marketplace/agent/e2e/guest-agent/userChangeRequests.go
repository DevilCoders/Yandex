package e2e

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/sha256"
	"encoding/base64"
	"math/big"
	"time"
)

// userChangeRequest used to create user change userChangeRequest.
type userChangeRequest struct {
	Modulus  string
	Exponent string
	Username string
	Expires  int64
	Schema   string
}

func newWindowsUserChangeRequest(name, modulus, exponent string) (r userChangeRequest) {
	r = userChangeRequest{
		Modulus:  modulus,
		Exponent: exponent,
		Username: name,
		Expires:  time.Now().Unix(),
		Schema:   "v1",
	}

	return
}

const UserChangeResponseType = "UserChangeResponse"

// userChangeResponse is sent from agent as a userChangeResponse to user change userChangeRequest.
type userChangeResponse struct {
	Modulus           string
	Exponent          string
	Username          string
	EncryptedPassword string
	Success           bool
	Error             string
}

// encryptionKey contain private key and helpers to encode/decode password.
type encryptionKey struct {
	rsaKey *rsa.PrivateKey
}

func newEncryptionKey() (rsaKey *encryptionKey, err error) {
	var k *rsa.PrivateKey
	k, err = rsa.GenerateKey(rand.Reader, 2048)
	rsaKey = &encryptionKey{
		rsaKey: k,
	}

	return
}

func (k *encryptionKey) GetModulus() string {
	return base64.StdEncoding.EncodeToString(k.rsaKey.PublicKey.N.Bytes())
}

func (k *encryptionKey) GetExponent() string {
	return base64.StdEncoding.EncodeToString(big.NewInt(int64(k.rsaKey.PublicKey.E)).Bytes())
}

func (k *encryptionKey) Decrypt(encStr string) (str string, err error) {
	var encBytes []byte

	encBytes, err = base64.StdEncoding.DecodeString(encStr)
	if err != nil {
		return
	}

	var decBytes []byte

	decBytes, err = rsa.DecryptOAEP(sha256.New(), rand.Reader, k.rsaKey, encBytes, nil)
	if err != nil {
		return
	}

	str = string(decBytes)

	return
}
