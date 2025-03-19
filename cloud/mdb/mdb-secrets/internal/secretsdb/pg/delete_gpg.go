package pg

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// DeleteGpg deletes gpg key for specified cid
func (s *serviceImpl) DeleteGpg(ctx context.Context, cid string) error {
	m := s.cluster.Primary()
	if m == nil {
		return secretsdb.ErrNoMaster.WithFrame()
	}

	_, err := m.DBx().ExecContext(ctx, `DELETE from secrets.gpg_keys where cid=$1`, cid)
	if err != nil {
		return xerrors.Errorf("failed to delete gpg key: %w", err)
	}

	return nil
}
