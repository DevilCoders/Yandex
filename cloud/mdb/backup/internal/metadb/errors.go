package metadb

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/xreflect"
)

type Errors interface {
	Add(Error)
	Marshal() ([]byte, error)
}

var (
	DeployInProgressError        = NewError("deploy is still in progress", true)
	UnsupportedClusterTypeError  = NewError("unknown cluster type", false)
	TimedOutError                = NewError("job is timed out", false)
	StorageBackupsNotFound       = NewError("backups were not found in storage", false)
	StorageCreatedBackupNotFound = NewError("created backup was not found in storage", false)
	StorageDeletedBackupFound    = NewError("deleted backup was found", false)
	CanNotLockBackups            = NewError("required backups are locked already", true)
)

type Error struct {
	Msg    string
	IsTemp bool
	Err    error
	TS     time.Time
}

func FromError(err error) *Error {
	if err == nil {
		return &Error{}
	}

	if merr, ok := err.(*Error); ok {
		return merr
	}
	if semerr.IsUnavailable(err) {
		return WrapTempError(err, "generic temporary error")
	}

	return WrapPermError(err, "generic permanent error")
}

func NewError(msg string, temp bool) *Error {
	return &Error{
		Msg:    msg,
		IsTemp: temp,
		Err:    xerrors.SkipErrorf(2, msg),
		TS:     time.Now(),
	}
}

func WrapPermError(err error, msg string) *Error {
	return &Error{
		Msg:    msg,
		IsTemp: false,
		Err:    xerrors.SkipErrorf(2, msg+": %w", err),
		TS:     time.Now(),
	}
}

func WrapTempError(err error, msg string) *Error {
	return &Error{
		Msg:    msg,
		IsTemp: true,
		Err:    xerrors.SkipErrorf(2, msg+": %w", err),
		TS:     time.Now(),
	}
}

// Error implements error interface
func (e *Error) Error() string {
	return e.Err.Error()
}

func (e *Error) Is(err error) bool {
	return err == e.Err
}

func (e *Error) As(target interface{}) bool {
	return xreflect.Assign(e.Err, target)
}

// Unwrap implements Wrapper interface
func (e *Error) Unwrap() error {
	return xerrors.Unwrap(e.Err)
}

// Temporary reports whether error is known to be temporary.
func (e *Error) Temporary() bool {
	return e.IsTemp
}
