package errtool

import (
	"context"
	"errors"
)

// IsContextError check if error most likely caused by this context cancelation
func IsContextError(ctx context.Context, err error) bool {
	if errors.Is(err, context.Canceled) || errors.Is(err, context.DeadlineExceeded) {
		return ctx.Err() != nil
	}
	return false
}

// WrapNotCtxErr apply wrapper only if error not caused by context cancelation
func WrapNotCtxErr(ctx context.Context, w ErrWrapper, err error) error {
	if IsContextError(ctx, err) {
		return err
	}
	return w.Wrap(err)
}

type ErrWrapper interface {
	Wrap(error) error
}
