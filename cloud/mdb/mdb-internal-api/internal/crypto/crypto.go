package crypto

import (
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh Crypto

type Crypto interface {
	Encrypt(msg []byte) (pillars.CryptoKey, error)
	GenerateRandomString(len int, abc []rune) (secret.String, error)
}

const PasswordValidRunes = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

const PasswordDefaultLegth = 96

func GenerateRawPassword(c Crypto, l int, abc []rune) (secret.String, error) {
	if len(abc) == 0 {
		abc = []rune(PasswordValidRunes)
	}
	raw, err := c.GenerateRandomString(l, abc)
	if err != nil {
		return secret.String{}, xerrors.Errorf("failed to generate password: %w", err)
	}
	return raw, nil
}

func GenerateEncryptedPassword(c Crypto, l int, abc []rune) (pillars.CryptoKey, error) {
	raw, err := GenerateRawPassword(c, l, abc)
	if err != nil {
		return pillars.CryptoKey{}, err
	}
	ecrypted, err := c.Encrypt([]byte(raw.Unmask()))
	if err != nil {
		return pillars.CryptoKey{}, xerrors.Errorf("failed to encrypt password: %w", err)
	}
	return ecrypted, nil
}
