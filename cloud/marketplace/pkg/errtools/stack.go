package errtools

import (
	"context"
	"runtime"
	"strconv"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/mds/go/xstrings"
)

const (
	framesDepth = 3
)

type stackError struct {
	error
	frames [framesDepth]uintptr
}

// withCaller wrap error with caller information
func withCaller(err error, skip int) error {
	if err == nil {
		return nil
	}

	if xerrors.Is(err, context.Canceled) || xerrors.Is(err, context.DeadlineExceeded) {
		return err
	}

	var wrapped *stackError
	if xerrors.As(err, &wrapped) {
		return err
	}
	wrapped = &stackError{error: err}
	runtime.Callers(skip+1, wrapped.frames[:])
	return wrapped
}

// WithPrevCaller is shorthand for withCaller(err, 1)
func WithPrevCaller(err error) error {
	return withCaller(err, 2)
}

// WithCurCaller is shorthand for withCaller(err, 0)
func WithCurCaller(err error) error {
	return withCaller(err, 1)
}

func (f stackError) Error() string {
	return xstrings.Concat(f.error.Error(), " (", f.locationString(), ")")
}

func (f stackError) Unwrap() error {
	return f.error
}

func (f stackError) locationString() string {
	frames := runtime.CallersFrames(f.frames[:])
	if _, ok := frames.Next(); !ok {
		return "::"
	}
	fr, ok := frames.Next()
	if !ok {
		return "::"
	}
	return xstrings.Concat(trimmedPath(fr.File), " ", funcName(fr.Function), ":", strconv.Itoa(fr.Line))
}

func funcName(name string) string {
	i := strings.LastIndex(name, "/")
	name = name[i+1:]
	i = strings.Index(name, ".")
	return name[i+1:]
}

func trimmedPath(path string) string {
	i := strings.LastIndex(path, "/")
	if i == -1 {
		return path
	}
	i = strings.LastIndex(path[:i], "/")
	if i == -1 {
		return path
	}
	return path[i+1:]
}
