package pg

import (
	"encoding/base64"
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *serviceImpl) encryptData(msg string) ([]byte, error) {
	box := nacl.Box{
		PeersPublicKey: s.saltPublicKey,
		PrivateKey:     s.privateKey,
	}
	encrypted, err := box.Seal([]byte(msg))
	if err != nil {
		return nil, xerrors.Errorf("failed to seal nacl.Box: %v", err)
	}

	versioned := secretsdb.EncryptedData{
		Version: 1,
		Data:    base64.URLEncoding.EncodeToString(encrypted),
	}

	return json.Marshal(&versioned)
}
