package errsentinel

import (
	"errors"
	"fmt"
	"io"

	"a.yandex-team.ru/library/go/x/xreflect"
)

// NewSentinel acts as New but does not add stack frame
func New(text string) *Sentinel {
	return &Sentinel{error: errors.New(text)}
}

// Sentinel error
type Sentinel struct {
	error
}

// Wrap error with this sentinel error. Adds stack frame.
func (s *Sentinel) Wrap(err error) error {
	if err == nil {
		panic("tried to wrap a nil error")
	}

	return &sentinelWrapper{
		err:     s,
		wrapped: err,
	}
}

type sentinelWrapper struct {
	err     error
	wrapped error
}

func (e *sentinelWrapper) Error() string {
	return fmt.Sprintf("%s", e)
}

func (e *sentinelWrapper) Format(s fmt.State, v rune) {
	switch v {
	case 'v':
		if s.Flag('+') {
			_, _ = io.WriteString(s, e.err.Error())
			_, _ = io.WriteString(s, ": ")
			_, _ = fmt.Fprintf(s, "%+v", e.wrapped)

			return
		}
		fallthrough
	case 's':
		_, _ = io.WriteString(s, e.err.Error())
		_, _ = io.WriteString(s, ": ")
		_, _ = io.WriteString(s, e.wrapped.Error())
	case 'q':
		_, _ = fmt.Fprintf(s, "%q", fmt.Sprintf("%s: %s", e.err.Error(), e.wrapped.Error()))
	}
}

// Unwrap implements Wrapper interface
func (e *sentinelWrapper) Unwrap() error {
	return e.wrapped
}

// Is checks if ew holds the specified error. Checks only immediate error.
func (e *sentinelWrapper) Is(target error) bool {
	return e.err == target
}

// As checks if error holds the specified error type. Checks only immediate error.
// It does NOT perform target checks as it relies on errors.As to do it
func (e *sentinelWrapper) As(target interface{}) bool {
	return xreflect.Assign(e.err, target)
}
