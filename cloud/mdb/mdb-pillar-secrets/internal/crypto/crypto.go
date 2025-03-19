package crypto

import "a.yandex-team.ru/cloud/mdb/internal/secret"

//go:generate ../../../scripts/mockgen.sh Crypto

type Crypto interface {
	Decrypt(key CryptoKey) (secret.String, error)
}

type CryptoKey struct {
	Data              string `json:"data"`
	EncryptionVersion int64  `json:"encryption_version"`
}
