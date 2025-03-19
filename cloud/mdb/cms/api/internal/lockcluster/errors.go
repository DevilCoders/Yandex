package lockcluster

import (
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/xreflect"
)

var (
	NotAcquiredConflicts = xerrors.New("lock is not acquired because of conflicts")
	NotAcquired          = xerrors.New("lock is not acquired")
	WrongHolder          = xerrors.New("lock is acquired by another holder")
	NoConflicts          = xerrors.New("there are no conflicts for lock")
	UnmanagedHost        = xerrors.New("host is unmanaged")
)

type Error struct {
	err    error
	reason string
}

func (e *Error) Error() string {
	return e.err.Error()
}

func (e *Error) Is(err error) bool {
	return err == e.err
}

func (e *Error) As(target interface{}) bool {
	return xreflect.Assign(e.err, target)
}

func (e *Error) Unwrap() error {
	return xerrors.Unwrap(e.err)
}

func ErrorReason(err error) (string, error) {
	if e, ok := err.(*Error); ok {
		return e.reason, e.err
	}
	return "", err
}

func NewError(err error, reason string) error {
	return &Error{err: err, reason: reason}
}
