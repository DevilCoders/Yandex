package ydb

import (
	"context"
	"database/sql/driver"
	"errors"
	"time"

	"github.com/cenkalti/backoff/v4"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type retrier struct {
	backoffOverride func() backoff.BackOff
}

func (s *retrier) retryRead(ctx context.Context, op backoff.Operation) error {
	bo := s.backOff(ctx)
	checkedOp := func() error {
		return retryReadError(op())
	}

	return backoff.Retry(checkedOp, bo)
}

func (s *retrier) retryWrite(ctx context.Context, op backoff.Operation) error {
	bo := s.backOff(ctx)
	checkedOp := func() error {
		return retryWriteError(op())
	}

	return backoff.Retry(checkedOp, bo)
}

func (s *retrier) backOff(ctx context.Context) backoff.BackOffContext {
	if s.backoffOverride != nil {
		return backoff.WithContext(s.backoffOverride(), ctx)
	}
	ebo := backoff.NewExponentialBackOff()
	ebo.InitialInterval = time.Millisecond * 20
	ebo.MaxInterval = time.Second
	ebo.MaxElapsedTime = time.Second * 5
	ebo.RandomizationFactor = 0.25

	return backoff.WithContext(ebo, ctx)
}

func retryReadError(err error) error {
	if err == nil {
		return err
	}
	var te *ydb.TransportError
	var oe *ydb.OpError

	switch {
	case errors.As(err, &te) && te.Reason == ydb.TransportErrorResourceExhausted:
		return err
	case errors.As(err, &oe) && oe.Reason != ydb.StatusNotFound && oe.Reason != ydb.StatusSchemeError:
		return err
	case errors.Is(err, driver.ErrBadConn):
		return err
	}
	return backoff.Permanent(err)
}

// retryableWriteOpCodes filled from ydb list of uncompleted operations status codes
var retryableWriteOpCodes = map[ydb.StatusCode]bool{
	ydb.StatusAborted:      true,
	ydb.StatusUnavailable:  true,
	ydb.StatusOverloaded:   true,
	ydb.StatusBadSession:   true,
	ydb.StatusSessionBusy:  true,
	ydb.StatusUnauthorized: true,
}

func retryWriteError(err error) error {
	if err == nil {
		return err
	}
	var te *ydb.TransportError
	var oe *ydb.OpError

	switch {
	case errors.As(err, &te) && te.Reason == ydb.TransportErrorResourceExhausted:
		return err
	case errors.As(err, &oe) && retryableWriteOpCodes[oe.Reason]:
		return err
	case errors.Is(err, driver.ErrBadConn):
		return err
	}
	return backoff.Permanent(err)
}
