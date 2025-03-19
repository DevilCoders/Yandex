package pgerrors_test

import (
	"fmt"
	"io"
	"net"
	"strconv"
	"testing"

	"github.com/jackc/pgconn"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestIsTemporary(t *testing.T) {
	tempErrors := map[string][]error{
		"io error": {
			&net.OpError{Op: "dial", Net: "tcp6"},
			io.EOF,
		},
		"cluster error": {
			semerr.Unavailable("unavailable"),
		},
		"Postgres error": {
			&pgconn.PgError{
				Severity: "FATAL",
				Code:     "57P01",
				Message:  "terminating connection due to administrator command",
			},
		},
		"Bouncer error": {
			&pgconn.PgError{
				Severity: "ERROR",
				Code:     "08P01",
				Message:  "query_wait_timeout",
			},
		},
		"Odyssey error": {
			&pgconn.PgError{
				Severity: "ERROR",
				Code:     "08006",
				Message:  "odyssey: ce74b57335417: remote server read/write error s68409c15ca59",
			},
		},
	}
	for errorClass, errorsInClass := range tempErrors {
		for _, tempErr := range errorsInClass {
			t.Run(fmt.Sprintf("%s is temporary [%s]", errorClass, tempErr), func(t *testing.T) {
				require.True(t, pgerrors.IsTemporary(tempErr))
			})
			wrappedErr := xerrors.Errorf("got error. I wrap it: %w", tempErr)
			t.Run(fmt.Sprintf("Wrapped %s is temporary [%s]", errorClass, wrappedErr), func(t *testing.T) {
				require.True(t, pgerrors.IsTemporary(wrappedErr))
			})
		}
	}

	nonTempErrors := map[string]error{
		"unexpected errors":         strconv.ErrSyntax,
		"unexpected Postgres error": &pgconn.PgError{Code: "23000", Message: "integrity_constraint_violation"},
	}
	for errorDesc, nonTempErr := range nonTempErrors {
		t.Run(fmt.Sprintf("%s is not temporary: [%s]", errorDesc, nonTempErr), func(t *testing.T) {
			require.False(t, pgerrors.IsTemporary(nonTempErr))
		})
	}
}
