package chutil

import (
	"context"
	"database/sql/driver"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// HandleErrBadConn is for specific clickhouse-go case when it returns ErrBadConn
// on context cancellation.
func HandleErrBadConn(ctx context.Context, err error) error {
	// Trigger only on ErrBadConn
	if !xerrors.Is(err, driver.ErrBadConn) {
		return err
	}

	select {
	case <-ctx.Done():
		// Do nothing if context error is already part of error stack. This is for when
		// driver fixes the issue.
		if !xerrors.Is(err, ctx.Err()) {
			err = ctx.Err()
		}
	default:
	}

	return err
}
