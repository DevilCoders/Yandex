package pg

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
)

const (
	// language=PostgreSQL
	queryDeleteCert = `
		DELETE
		FROM secrets.certs
		WHERE host = $1`
)

// DeleteCert deletes certificate.
func (s *serviceImpl) DeleteCert(ctx context.Context, hostname string) error {
	node := s.cluster.Primary()
	if node == nil {
		return secretsdb.ErrNoMaster.WithFrame()
	}

	_, err := node.DBx().ExecContext(ctx, queryDeleteCert, hostname)
	return err
}
