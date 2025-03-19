package idempotence

import (
	"context"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ctxKey string

func (c ctxKey) String() string {
	return "context key " + string(c)
}

const (
	logFieldName = "idempotence_id"
)

// WithLogField adds idempotence id log field to context
func WithLogField(ctx context.Context, id string) context.Context {
	return ctxlog.WithFields(ctx, log.String(logFieldName, id))
}

func Validate(id string) error {
	if _, err := uuid.FromString(id); err != nil {
		return xerrors.New("idempotence id is not UUID")
	}

	return nil
}
