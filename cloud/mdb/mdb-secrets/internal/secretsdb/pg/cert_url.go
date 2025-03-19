package pg

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/mdb/internal/crt/letsencrypt"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	querySelectCertURL = `SELECT url FROM secrets.cert_urls WHERE host = $1`
	queryInsertCertURL = `
INSERT INTO secrets.cert_urls VALUES ($1, $2)
ON CONFLICT (host) DO UPDATE
SET url = $2
`
	queryDeleteCertURL = `DELETE FROM secrets.cert_urls WHERE host = $1`
)

// CertificateURL retrieves certificate fetch url for specified host.
func (s *serviceImpl) CertificateURL(ctx context.Context, host string) (string, error) {
	node := s.cluster.Alive()
	if node == nil {
		return "", secretsdb.ErrNotAvailable.WithFrame()
	}

	var certURL string
	err := node.DBx().QueryRowContext(ctx, querySelectCertURL, host).Scan(&certURL)
	if xerrors.Is(err, sql.ErrNoRows) {
		return "", letsencrypt.ErrCertURLNotFound.WithFrame()
	}
	if err != nil {
		return "", err
	}

	return certURL, nil
}

// SetCertificateURL stores certificate fetch url for specified host.
func (s *serviceImpl) SetCertificateURL(ctx context.Context, host, url string) error {
	m := s.cluster.Primary()
	if m == nil {
		return secretsdb.ErrNoMaster.WithFrame()
	}

	_, err := m.DBx().ExecContext(ctx, queryInsertCertURL, host, url)
	return err
}

// RemoveCertificateURL removes certificate fetch url for specified host.
func (s *serviceImpl) RemoveCertificateURL(ctx context.Context, host string) error {
	m := s.cluster.Primary()
	if m == nil {
		return secretsdb.ErrNoMaster.WithFrame()
	}

	_, err := m.DBx().ExecContext(ctx, queryDeleteCertURL, host)
	return err
}
