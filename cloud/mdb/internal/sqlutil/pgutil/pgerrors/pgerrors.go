package pgerrors

import (
	"io"
	"net"

	"github.com/jackc/pgconn"
	"github.com/jackc/pgerrcode"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func makeErrorMap(keys ...string) map[string]struct{} {
	ret := make(map[string]struct{})
	for _, k := range keys {
		ret[k] = struct{}{}
	}
	return ret
}

var tempPostgreSQLErrors = makeErrorMap(
	// Odyssey use that code
	// ERROR: odyssey: c0b193120979f: remote server read/write error s2a866017d102 (SQLSTATE 08006)
	pgerrcode.ConnectionFailure,
	// PgBouncer use that code
	//	* pgbouncer cannot connect to server
	//  * query_wait_timeout
	//  * invalid server parameter
	//  * server conn crashed?
	//  * the database system is in recovery mode
	pgerrcode.ProtocolViolation,
	// 'Pure' Postgres codes
	pgerrcode.ConnectionException,
	pgerrcode.TooManyConnections,
	pgerrcode.LockNotAvailable, // ERROR: canceling statement due to lock timeout
	pgerrcode.OperatorIntervention,
	pgerrcode.QueryCanceled,
	pgerrcode.AdminShutdown, // terminating connection due to administrator command
	pgerrcode.CrashShutdown,
	// Odyssey also forward it
	// FATAL: odyssey: c890e71787623: the database system is in recovery mode (SQLSTATE 57P03)'
	pgerrcode.CannotConnectNow,
	pgerrcode.SystemError,
	pgerrcode.IOError,
)

// IsTemporary return true if error is temporary Postgres or connection pooler error
// We expect that:
// 1. You use sqlutil.Cluster
// 2. You use pgx
// TODO: remove in favor of semerr.IsUnavailable(err) when every call uses error conversion code path
func IsTemporary(err error) bool {
	if semerr.IsUnavailable(err) {
		return true
	}

	var pgErr *pgconn.PgError
	if xerrors.As(err, &pgErr) {
		if _, ok := tempPostgreSQLErrors[pgErr.Code]; ok {
			return true
		}
	}

	var netError net.Error
	if xerrors.As(err, &netError) {
		// treat all network errors as temporary
		// cause we caller should use sqlutil.Cluster
		// and get that error when run query
		return true
	}

	return xerrors.Is(err, io.EOF)
}

func Code(err error) (string, bool) {
	var pgErr *pgconn.PgError
	if xerrors.As(err, &pgErr) {
		return pgErr.Code, true
	}

	return "", false
}

func DefaultWrapper(err error) error {
	wrapped, ok := semerr.WrapWellKnownChecked(err)
	if ok {
		return wrapped
	}

	if code, ok := Code(err); ok {
		switch code {
		case pgerrcode.UniqueViolation:
			return sqlerrors.ErrAlreadyExists.Wrap(err)
		}
		if _, ok := tempPostgreSQLErrors[code]; ok {
			return semerr.WrapWithUnavailable(err, "unavailable")
		}
	}

	if xerrors.Is(err, io.EOF) || xerrors.Is(err, io.ErrUnexpectedEOF) {
		return semerr.WrapWithUnavailable(err, "unavailable")
	}

	return err
}
