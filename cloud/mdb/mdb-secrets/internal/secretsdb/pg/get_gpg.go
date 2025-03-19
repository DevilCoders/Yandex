package pg

import (
	"context"
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// GetGpg retrieves gpg key.
// Can return secretsdb.ErrKeyNotFound
func (s *serviceImpl) GetGpg(ctx context.Context, cid string) (*secretsdb.EncryptedData, error) {
	var keys [][]byte
	node := s.cluster.Alive()
	if node == nil {
		return nil, secretsdb.ErrNotAvailable.WithFrame()
	}
	err := node.DBx().SelectContext(ctx, &keys, `SELECT gpg_key from secrets.gpg_keys where cid=$1`, cid)
	if err != nil {
		return nil, xerrors.Errorf("failed to select gpg key: %w", err)
	}
	if len(keys) == 0 {
		return nil, secretsdb.ErrKeyNotFound
	}

	ed := secretsdb.EncryptedData{}
	if err = json.Unmarshal(keys[0], &ed); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal encrypted data: %w", err)
	}
	return &ed, nil
}
