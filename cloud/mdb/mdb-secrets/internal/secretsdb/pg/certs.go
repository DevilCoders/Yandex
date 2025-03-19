package pg

import (
	"context"
	"database/sql"
	"encoding/json"

	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	// language=PostgreSQL
	querySelectCertForUpdate = `
		SELECT host, alt_names, key, crt, ca, expiration
		FROM secrets.certs
		WHERE host = $1 FOR NO KEY UPDATE`
	querySelectCert = `
		SELECT host, alt_names, key, crt, ca, expiration
		FROM secrets.certs
		WHERE host = $1`
	queryInsertDummyCert = `
		INSERT INTO secrets.certs(host, alt_names, key, crt, ca, expiration)
		VALUES ($1, '{}', '{}', '', '', now() - INTERVAL '1 year')
		ON CONFLICT (host) DO NOTHING`
	queryUpdateCert = `
		UPDATE secrets.certs
		SET key= $2,
    		crt = $3,
    		ca = $4,
    		expiration = $5,
		    alt_names = $6,
		    modified_at = now()
		WHERE host = $1`
	queryInsertCert = `
		INSERT INTO secrets.certs(host, alt_names, key, crt, ca, expiration)
		VALUES ($1, $2, $3, $4, $5, $6)
		ON CONFLICT (host) DO NOTHING`
)

func (s *serviceImpl) GetCert(ctx context.Context, host string) (*secretsdb.Cert, error) {
	node := s.cluster.Alive()
	if node == nil {
		return nil, secretsdb.ErrNoMaster.WithFrame()
	}

	dst := &secretsdb.Cert{}
	var rawKey []byte
	var altNames pgtype.TextArray

	err := node.DBx().QueryRowContext(ctx, querySelectCert, host).
		Scan(&dst.Host,
			&altNames,
			&rawKey,
			&dst.Crt,
			&dst.CA,
			&dst.Expiration,
		)
	if xerrors.Is(err, sql.ErrNoRows) {
		return nil, secretsdb.ErrCertNotFound.WithFrame()
	}
	if err != nil {
		return nil, err
	}
	err = altNames.AssignTo(&dst.AltNames)
	if err != nil {
		return nil, err
	}
	err = json.Unmarshal(rawKey, &dst.Key)
	if err != nil {
		return nil, err
	}
	if dst.Crt == "" {
		return nil, secretsdb.ErrCertNotFound
	}
	return dst, nil
}

func (s *serviceImpl) InsertDummyCert(ctx context.Context, hostname string) error {
	m := s.cluster.Primary()
	if m == nil {
		return secretsdb.ErrNoMaster.WithFrame()
	}

	_, err := m.DBx().ExecContext(ctx, queryInsertDummyCert, hostname)
	return err
}

func (s *serviceImpl) UpdateCert(ctx context.Context, hostname string, ca string, newCert func(ctx context.Context) (*secretsdb.CertUpdate, error), needUpdate func(cert *secretsdb.Cert) bool) (*secretsdb.Cert, error) {
	m := s.cluster.Primary()
	if m == nil {
		return nil, secretsdb.ErrNoMaster.WithFrame()
	}

	var dst *secretsdb.Cert
	var err error
	err = sqlutil.InTx(ctx, m, func(binding sqlutil.TxBinding) error {
		// TODO: use QueryBinding
		dst, err = parseCertsRow(binding.Tx().QueryRowxContext(ctx, querySelectCertForUpdate,
			hostname))
		if err != nil {
			return err
		}

		if needUpdate(dst) {
			certUpd, newCertErr := newCert(ctx)
			if newCertErr != nil {
				return newCertErr
			}

			encKey, encErr := s.encryptData(certUpd.Key)
			if encErr != nil {
				return encErr
			}

			// TODO: use QueryBinding
			_, err = binding.Tx().ExecContext(
				ctx,
				queryUpdateCert,
				hostname,
				string(encKey),
				certUpd.Crt,
				ca,
				certUpd.Expiration,
				append([]string{}, certUpd.AltNames...))
			if err != nil {
				return err
			}

			//refresh cert after the update
			// TODO: use QueryBinding
			dst, err = parseCertsRow(binding.Tx().QueryRowxContext(ctx, querySelectCert,
				hostname))
			if err != nil {
				return err
			}
		}
		return nil
	})

	if err != nil {
		return nil, err
	}

	return dst, nil
}

func (s *serviceImpl) InsertNewCert(ctx context.Context, hostname string, ca string, altNames []string, cert secretsdb.CertUpdate) error {
	m := s.cluster.Primary()
	if m == nil {
		return secretsdb.ErrNoMaster.WithFrame()
	}

	encKey, encErr := s.encryptData(cert.Key)
	if encErr != nil {
		return encErr
	}

	res, err := m.DBx().ExecContext(ctx, queryInsertCert, hostname, altNames, string(encKey), cert.Crt, ca, cert.Expiration)
	if err != nil {
		return err
	}
	rowsAffected, err := res.RowsAffected()
	if err != nil {
		return err
	}

	if rowsAffected == 0 {
		return secretsdb.ErrCertAlreadyExist.WithFrame()
	}
	return nil
}

func parseCertsRow(row *sqlx.Row) (*secretsdb.Cert, error) {
	dst := &secretsdb.Cert{}
	var rawKey []byte
	var altNames pgtype.TextArray

	err := row.Scan(
		&dst.Host,
		&altNames,
		&rawKey,
		&dst.Crt,
		&dst.CA,
		&dst.Expiration)
	if err != nil {
		return nil, err
	}
	err = altNames.AssignTo(&dst.AltNames)
	if err != nil {
		return nil, err
	}
	err = json.Unmarshal(rawKey, &dst.Key)
	if err != nil {
		return nil, err
	}
	return dst, nil
}
