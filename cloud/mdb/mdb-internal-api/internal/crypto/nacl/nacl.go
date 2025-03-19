package nacl

import (
	"crypto/rand"
	"encoding/base64"
	"math/big"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type NaclProvider struct {
	box *nacl.Box
}

var _ crypto.Crypto = &NaclProvider{}

func New(peersPublicKey, privateKey string) (*NaclProvider, error) {
	parsedPeersPublicKey, err := nacl.ParseKey(peersPublicKey)
	if err != nil {
		return &NaclProvider{}, err
	}
	parsedPrivateKey, err := nacl.ParseKey(privateKey)
	if err != nil {
		return &NaclProvider{}, err
	}

	return &NaclProvider{
		box: &nacl.Box{
			PeersPublicKey: parsedPeersPublicKey,
			PrivateKey:     parsedPrivateKey,
		},
	}, nil
}

func (nc *NaclProvider) Encrypt(msg []byte) (pillars.CryptoKey, error) {
	encryptedBytes, err := nc.box.Seal(msg)
	if err != nil {
		return pillars.CryptoKey{}, err
	}
	return pillars.CryptoKey{
		EncryptionVersion: 1,
		Data:              base64.URLEncoding.EncodeToString(encryptedBytes),
	}, nil
}

func (nc *NaclProvider) GenerateRandomString(l int, abc []rune) (secret.String, error) {
	res := make([]rune, l)
	abcLen := big.NewInt(int64(len(abc)))
	for i := 0; i < l; i++ {
		j, err := rand.Int(rand.Reader, abcLen)
		if err != nil {
			return secret.String{}, xerrors.Errorf("failed to generate random string: %w", err)
		}
		res[i] = abc[j.Int64()]
	}
	return secret.NewString(string(res)), nil
}
