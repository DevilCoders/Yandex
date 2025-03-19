package pg

import (
	"context"
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// PutGpg inserts gpg key
func (s *serviceImpl) PutGpg(ctx context.Context, cid, key string) (*secretsdb.EncryptedData, error) {
	m := s.cluster.Primary()
	if m == nil {
		return nil, secretsdb.ErrNoMaster.WithFrame()
	}

	byteKey, err := s.encryptData(key)
	if err != nil {
		return nil, err
	}

	var keyA []byte

	err = sqlutil.InTx(ctx, m, func(binding sqlutil.TxBinding) error {
		// TODO: use QueryBinding
		_, err = binding.Tx().ExecContext(ctx, `
		INSERT INTO secrets.gpg_keys(cid, gpg_key)
		VALUES ($1, $2)
		ON CONFLICT (cid) DO NOTHING`,
			cid, string(byteKey))
		if err != nil {
			return err
		}

		// TODO: use QueryBinding
		row := binding.Tx().QueryRowxContext(ctx, `
		SELECT gpg_key FROM secrets.gpg_keys WHERE cid = $1`,
			cid)
		if row.Err() != nil {
			return row.Err()
		}
		err = row.Scan(&keyA)
		if err != nil {
			return err
		}

		return nil
	})
	if err != nil {
		return nil, err
	}

	var ed secretsdb.EncryptedData
	if err = json.Unmarshal(keyA, &ed); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal encrypted data: %w", err)
	}

	return &ed, nil
}
