package sentry

import "context"

type noop struct{}

func (n noop) CaptureError(context.Context, error, map[string]string)        {}
func (n noop) CaptureErrorAndWait(context.Context, error, map[string]string) {}

var noopClient Client = noop{}

func Noop() Client {
	return noopClient
}
