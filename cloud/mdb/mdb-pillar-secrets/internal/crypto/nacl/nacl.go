package nacl

import (
	"encoding/base64"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/crypto"
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

func (nc *NaclProvider) Decrypt(key crypto.CryptoKey) (secret.String, error) {
	var data []byte
	ok := false

	switch key.EncryptionVersion {
	case 0:
		data, ok = []byte(key.Data), true
	case 1:
		encrypted, err := base64.URLEncoding.DecodeString(key.Data)
		if err != nil {
			return secret.String{}, err
		}
		data, ok = nc.box.Open(encrypted)
	default:
		return secret.String{}, semerr.InvalidInputf("invalid encryption version %q", key.EncryptionVersion)
	}

	if !ok {
		return secret.String{}, semerr.Internal("failed to decrypt data")
	}

	return secret.NewString(string(data)), nil
}
