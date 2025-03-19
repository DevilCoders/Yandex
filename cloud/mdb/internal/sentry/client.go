package sentry

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../scripts/mockgen.sh Client

type Client interface {
	CaptureError(ctx context.Context, err error, tags map[string]string)
	CaptureErrorAndWait(ctx context.Context, err error, tags map[string]string)
}

var globalClient Client = Noop()

func GlobalClient() Client {
	return globalClient
}

func SetGlobalClient(c Client) {
	globalClient = c
}

// CapturePanicAndWait converts panic error to error (if it needed) and sends it to the Sentry
func CapturePanicAndWait(ctx context.Context, panicError interface{}, tags map[string]string) {
	var sentryError error
	switch panicError := panicError.(type) {
	case error:
		sentryError = panicError
	default:
		sentryError = xerrors.Errorf("panic: %+v", panicError)
	}
	GlobalClient().CaptureErrorAndWait(ctx, sentryError, tags)
}
