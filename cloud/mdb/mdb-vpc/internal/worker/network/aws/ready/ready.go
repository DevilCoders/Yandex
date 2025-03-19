package ready

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ErrorTester struct {
	l   log.Logger
	err error
}

func (e *ErrorTester) IsPermanentError(ctx context.Context, err error) bool {
	if xerrors.Is(err, e.err) {
		ctxlog.Debug(ctx, e.l, "still not ready")
		return false
	}
	return true
}

func NewErrorTester(l log.Logger, err error) *ErrorTester {
	return &ErrorTester{l: l, err: err}
}

type CheckingObject interface{}
type ReadinessChecker struct {
	f   func(ctx context.Context) (CheckingObject, error)
	obj CheckingObject
}

func (c *ReadinessChecker) IsReady(ctx context.Context) error {
	obj, err := c.f(ctx)
	c.obj = obj
	return err
}

func (c *ReadinessChecker) GetResult() CheckingObject {
	return c.obj
}

func NewReadinessChecker(f func(ctx context.Context) (CheckingObject, error)) *ReadinessChecker {
	return &ReadinessChecker{
		f: f,
	}
}
