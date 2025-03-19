package logging

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

////////////////////////////////////////////////////////////////////////////////

func SetLoggingFields(ctx context.Context) context.Context {
	fields := make([]log.Field, 0)

	idempotencyKey := headers.GetIdempotencyKey(ctx)
	if len(idempotencyKey) != 0 {
		fields = append(fields, log.String("IDEMPOTENCY_KEY", idempotencyKey))
	}

	requestID := headers.GetRequestID(ctx)
	if len(requestID) != 0 {
		fields = append(fields, log.String("REQUEST_ID", requestID))
	}

	return ctxlog.WithFields(
		ctx,
		fields...,
	)
}
