package misc

import (
	"io"

	"golang.org/x/xerrors"

	"github.com/gofrs/uuid"
	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

// GcdInt64 Implements Euclid's algorithm.
func GcdInt64(a, b int64) int64 {
	for b != 0 {
		a, b = b, a%b
	}
	return a
}

// Ceil round a number n up to multiple of div.
func Ceil(n, div int64) int64 {
	return (n + div - 1) / div * div
}

// CheckContextError rewrites err with context error.
func CheckContextError(ctx context.Context, err error, override bool) error {
	if ctx.Err() != nil && (err == nil || override) {
		return ctx.Err()
	}
	return err
}

// ReaderWithContext implements io.Reader-like interface but is cancellable.
type ReaderWithContext struct {
	io.Reader
	Ctx context.Context
}

// Read implements io.Reader.
func (r *ReaderWithContext) Read(p []byte) (n int, err error) {
	n, err = r.Reader.Read(p)
	return n, CheckContextError(r.Ctx, err, false)
}

// FillIDIfAbsent replaces the empty string field with UUID.
func FillIDIfAbsent(ctx context.Context, id *string) error {
	if id == nil || *id != "" {
		return nil
	}

	newID, err := uuid.NewV4()
	if err != nil {
		log.G(ctx).Error("FillIDIfAbsent: failed to generate uuid", zap.Error(err))
		return xerrors.Errorf("fillIDIfAbsent: failed to generate uuid: %w", err)
	}
	*id = newID.String()
	return nil
}

type GetEtag func() (string, error)

type GetLength func() (int64, error)
