package errors

import (
	"context"
	"errors"
	"fmt"

	"github.com/ydb-platform/ydb-go-sdk/v3"
	grpc_codes "google.golang.org/grpc/codes"
	grpc_status "google.golang.org/grpc/status"

	disk_manager "a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/pkg/client/codes"
)

////////////////////////////////////////////////////////////////////////////////

type RetriableError struct {
	Err              error
	IgnoreRetryLimit bool
}

func (e *RetriableError) Error() string {
	return fmt.Sprintf("Retriable error: %v", e.Err)
}

func (e *RetriableError) Unwrap() error {
	return e.Err
}

func (e *RetriableError) Is(target error) bool {
	t, ok := target.(*RetriableError)
	if !ok {
		return false
	}

	return t.Err == nil || (e.Err == t.Err)
}

func MakeRetriable(err error, ignoreRetryLimit bool) error {
	if err != nil {
		return &RetriableError{
			Err:              err,
			IgnoreRetryLimit: ignoreRetryLimit,
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type NonRetriableError struct {
	Err    error
	Silent bool
}

func (e *NonRetriableError) Error() string {
	return fmt.Sprintf("Non retriable error: %v", e.Err)
}

func (e *NonRetriableError) Unwrap() error {
	return e.Err
}

func (e *NonRetriableError) Is(target error) bool {
	t, ok := target.(*NonRetriableError)
	if !ok {
		return false
	}

	return t.Err == nil || (e.Err == t.Err)
}

////////////////////////////////////////////////////////////////////////////////

type NonCancellableError struct {
	Err error
}

func (e *NonCancellableError) Error() string {
	return fmt.Sprintf("Non cancellable error: %v", e.Err)
}

func (e *NonCancellableError) Unwrap() error {
	return e.Err
}

func (e *NonCancellableError) Is(target error) bool {
	t, ok := target.(*NonCancellableError)
	if !ok {
		return false
	}

	return t.Err == nil || (e.Err == t.Err)
}

////////////////////////////////////////////////////////////////////////////////

type WrongGenerationError struct{}

func (WrongGenerationError) Error() string {
	return "Wrong generation"
}

////////////////////////////////////////////////////////////////////////////////

type InterruptExecutionError struct{}

func (InterruptExecutionError) Error() string {
	return "Interrupt execution"
}

////////////////////////////////////////////////////////////////////////////////

type NotFoundError struct {
	TaskID         string
	IdempotencyKey string
}

func (e NotFoundError) Error() string {
	return fmt.Sprintf(
		"No task with ID=%v, IdempotencyKey=%v",
		e.TaskID,
		e.IdempotencyKey,
	)
}

// HACK: Need to avoid default comparator that uses inner fields.
func (e NotFoundError) Is(target error) bool {
	_, ok := target.(*NotFoundError)
	return ok
}

////////////////////////////////////////////////////////////////////////////////

type ErrorDetails = disk_manager.ErrorDetails

type DetailedError struct {
	Err     error
	Details *ErrorDetails
	Silent  bool
}

func (e *DetailedError) Error() string {
	return e.Err.Error()
}

func (e *DetailedError) Unwrap() error {
	return e.Err
}

func (e *DetailedError) Is(target error) bool {
	t, ok := target.(*DetailedError)
	if !ok {
		return false
	}

	return t.Err == nil || (e.Err == t.Err)
}

func (e *DetailedError) GRPCStatus() *grpc_status.Status {
	status, _ := grpc_status.FromError(e.Err)
	return status
}

////////////////////////////////////////////////////////////////////////////////

func isCancelledError(err error) bool {
	switch {
	case
		errors.Is(err, context.Canceled),
		ydb.IsTransportError(err, grpc_codes.Canceled):
		return true
	default:
		return false
	}
}

func LogError(
	ctx context.Context,
	err error,
	format string,
	args ...interface{},
) {

	description := fmt.Sprintf(format, args...)

	if Is(err, &WrongGenerationError{}) ||
		Is(err, &InterruptExecutionError{}) ||
		isCancelledError(err) {

		logging.Debug(ctx, "%v: %v", description, err)
	} else {
		logging.Warn(ctx, "%v: %v", description, err)
	}
}

////////////////////////////////////////////////////////////////////////////////

func New(text string) error {
	return errors.New(text)
}

func As(err error, target interface{}) bool {
	return errors.As(err, target)
}

func Is(err, target error) bool {
	return errors.Is(err, target)
}

func CanRetry(err error) bool {
	if Is(err, &WrongGenerationError{}) || Is(err, &InterruptExecutionError{}) {
		return true
	}

	return !Is(err, &NonCancellableError{}) &&
		!Is(err, &NonRetriableError{}) &&
		Is(err, &RetriableError{})
}

func IsPublic(err error) bool {
	detailedError := &DetailedError{}
	if !As(err, &detailedError) {
		return false
	}

	if detailedError.Details == nil {
		return false
	}

	return !detailedError.Details.Internal
}

func IsSilent(err error) bool {
	nonRetriableError := &NonRetriableError{}
	if As(err, &nonRetriableError) {
		return nonRetriableError.Silent
	}

	detailedError := &DetailedError{}
	if As(err, &detailedError) {
		return detailedError.Silent
	}

	return false
}

////////////////////////////////////////////////////////////////////////////////

func CreateInvalidArgumentError(format string, args ...interface{}) error {
	message := fmt.Sprintf(format, args...)

	return &DetailedError{
		Err: &NonRetriableError{
			Err: New(message),
		},
		Details: &ErrorDetails{
			Code:     codes.InvalidArgument,
			Message:  message,
			Internal: true,
		},
	}
}
