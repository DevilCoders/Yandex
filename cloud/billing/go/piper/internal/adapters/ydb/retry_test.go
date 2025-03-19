package ydb

import (
	"context"
	"database/sql/driver"
	"testing"
	"time"

	"github.com/cenkalti/backoff/v4"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type retryTestSuite struct {
	baseMetaSessionTestSuite

	isWriteTest bool
	testFunc    func(ctx context.Context, op backoff.Operation) error

	remainTries int
}

func TestRetryRead(t *testing.T) {
	suite.Run(t, &retryTestSuite{isWriteTest: false})
}

func TestRetryWrite(t *testing.T) {
	suite.Run(t, &retryTestSuite{isWriteTest: true})
}

func (suite *retryTestSuite) SetupTest() {
	suite.baseMetaSessionTestSuite.SetupTest()
	suite.session.backoffOverride = func() backoff.BackOff {
		return suite
	}
	suite.testFunc = suite.session.retryRead
	if suite.isWriteTest {
		suite.testFunc = suite.session.retryWrite
	}
}

func (suite *retryTestSuite) TestOk() {
	err := suite.testFunc(suite.ctx, func() error {
		return nil
	})
	suite.Require().NoError(err)
}

func (suite *retryTestSuite) TestGeneralError() {
	suite.remainTries = 10
	err := suite.testFunc(suite.ctx, func() error {
		return errTest
	})
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, errTest)
	suite.Require().NotZero(suite.remainTries)
}

func (suite *retryTestSuite) TestBadConnError() {
	suite.remainTries = 10
	err := suite.testFunc(suite.ctx, func() error {
		return driver.ErrBadConn
	})
	suite.Require().Error(err)
	suite.Require().ErrorIs(err, driver.ErrBadConn)
	suite.Require().Zero(suite.remainTries)
}

func (suite *retryTestSuite) TestTransportErrors() {
	cases := []struct {
		reason ydb.TransportErrorCode
		retry  bool
	}{
		{ydb.TransportErrorUnknownCode, false},
		{ydb.TransportErrorCanceled, false},
		{ydb.TransportErrorUnknown, false},
		{ydb.TransportErrorInvalidArgument, false},
		{ydb.TransportErrorDeadlineExceeded, false},
		{ydb.TransportErrorNotFound, false},
		{ydb.TransportErrorAlreadyExists, false},
		{ydb.TransportErrorPermissionDenied, false},
		{ydb.TransportErrorResourceExhausted, true},
		{ydb.TransportErrorFailedPrecondition, false},
		{ydb.TransportErrorAborted, false},
		{ydb.TransportErrorOutOfRange, false},
		{ydb.TransportErrorUnimplemented, false},
		{ydb.TransportErrorInternal, false},
		{ydb.TransportErrorUnavailable, false},
		{ydb.TransportErrorDataLoss, false},
		{ydb.TransportErrorUnauthenticated, false},
	}
	for _, c := range cases {
		suite.Run(c.reason.String(), func() {
			suite.remainTries = 2
			err := suite.testFunc(suite.ctx, func() error {
				return &ydb.TransportError{Reason: c.reason}
			})
			suite.Require().Error(err)
			if c.retry {
				suite.Zero(suite.remainTries)
			} else {
				suite.Require().NotZero(suite.remainTries)
			}
		})
	}
}

func (suite *retryTestSuite) TestOpErrors() {
	readOnly := !suite.isWriteTest
	cases := []struct {
		reason ydb.StatusCode
		retry  bool
	}{
		{ydb.StatusUnknownStatus, readOnly},
		{ydb.StatusBadRequest, readOnly},
		{ydb.StatusUnauthorized, true},
		{ydb.StatusInternalError, readOnly},
		{ydb.StatusAborted, true},
		{ydb.StatusUnavailable, true},
		{ydb.StatusOverloaded, true},
		{ydb.StatusSchemeError, false},
		{ydb.StatusGenericError, readOnly},
		{ydb.StatusTimeout, readOnly},
		{ydb.StatusBadSession, true},
		{ydb.StatusPreconditionFailed, readOnly},
		{ydb.StatusAlreadyExists, readOnly},
		{ydb.StatusNotFound, false},
		{ydb.StatusSessionExpired, readOnly},
		{ydb.StatusCancelled, readOnly},
		{ydb.StatusUndetermined, readOnly},
		{ydb.StatusUnsupported, readOnly},
		{ydb.StatusSessionBusy, true},
	}
	for _, c := range cases {
		suite.Run(c.reason.String(), func() {
			suite.remainTries = 2
			err := suite.testFunc(suite.ctx, func() error {
				return &ydb.OpError{Reason: c.reason}
			})
			suite.Require().Error(err)
			if c.retry {
				suite.Zero(suite.remainTries)
			} else {
				suite.Require().NotZero(suite.remainTries)
			}
		})
	}
}

func (suite *retryTestSuite) NextBackOff() time.Duration {
	if suite.remainTries <= 0 {
		return backoff.Stop
	}
	suite.remainTries--
	return time.Microsecond
}

func (suite *retryTestSuite) Reset() {}
