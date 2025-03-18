package errors

import (
	"errors"
	"fmt"
)

type ErrorResult interface {
	Wrap(msg string) ErrorResult
	Error() error
	Status() ErrorStatus
}

// TODO: add msg
type ErrorResultImpl struct {
	Err         error
	ErrorStatus ErrorStatus
}

type ErrorStatus int64

const (
	Unspecified ErrorStatus = iota
	ValidationError
	AuthorizationError
	ForbiddenError
	NotFoundError
	InternalError
	FatalError
)

func NewErrorResult(msg string, status ErrorStatus) ErrorResult {
	return ErrorResultImpl{
		Err:         errors.New(msg),
		ErrorStatus: status,
	}
}

func WrapError(msg string, status ErrorStatus, err error) ErrorResult {
	return ErrorResultImpl{
		Err:         wrapError(msg, err),
		ErrorStatus: status,
	}
}

func (r ErrorResultImpl) Wrap(msg string) ErrorResult {
	return ErrorResultImpl{
		Err:         wrapError(msg, r.Err),
		ErrorStatus: r.ErrorStatus,
	}
}

func (r ErrorResultImpl) Error() error {
	return r.Err
}

func (r ErrorResultImpl) Status() ErrorStatus {
	return r.ErrorStatus
}

func wrapError(msg string, err error) error {
	return fmt.Errorf("%s: %w", msg, err)
}
