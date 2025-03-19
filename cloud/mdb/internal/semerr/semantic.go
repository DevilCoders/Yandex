package semerr

import (
	"context"
	"fmt"
	"net"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/xreflect"
)

// Semantic meaning of the error
type Semantic int

// Known error meanings
const (
	SemanticUnknown Semantic = iota
	SemanticNotImplemented
	SemanticUnavailable
	SemanticInvalidInput
	SemanticInternal
	SemanticAuthentication
	SemanticAuthorization
	SemanticFailedPrecondition
	SemanticNotFound
	SemanticAlreadyExists
)

// Error with semantic info
type Error struct {
	Message  string
	Semantic Semantic
	Details  interface{}

	err error
}

func newError(s Semantic, text string) *Error {
	return &Error{
		Semantic: s,
		Message:  text,
		err:      xerrors.SkipErrorf(2, text),
	}
}

func newErrorf(s Semantic, format string, a ...interface{}) *Error {
	return &Error{
		Semantic: s,
		Message:  fmt.Sprintf(format, a...),
		err:      xerrors.SkipErrorf(2, format, a...),
	}
}

func wrapError(s Semantic, err error, text string) *Error {
	return &Error{
		Semantic: s,
		Message:  text,
		err:      xerrors.SkipErrorf(2, text+": %w", err),
	}
}

func wrapErrorf(s Semantic, err error, format string, a ...interface{}) *Error {
	args := a
	args = append(args, err)
	return &Error{
		Semantic: s,
		Message:  fmt.Sprintf(format, a...),
		err:      xerrors.SkipErrorf(2, format+": %w", args),
	}
}

// Error implements error interface
func (e *Error) Error() string {
	return fmt.Sprintf("%s", e)
}

func (e *Error) Format(s fmt.State, v rune) {
	e.err.(fmt.Formatter).Format(s, v)
}

func (e *Error) Is(err error) bool {
	return err == e.err
}

func (e *Error) As(target interface{}) bool {
	return xreflect.Assign(e.err, target)
}

// Unwrap implements Wrapper interface
func (e *Error) Unwrap() error {
	// TODO: test for correct unwrap
	return xerrors.Unwrap(e.err)
}

// isSemanticError returns true if target error is specified semantic error
func isSemanticError(err error, semantic Semantic) bool {
	target := AsSemanticError(err)
	if target == nil {
		return false
	}

	return target.Semantic == semantic
}

// AsSemanticError returns semantic error if there is one
func AsSemanticError(err error) *Error {
	var target *Error
	if !xerrors.As(err, &target) {
		return nil
	}

	return target
}

// unknown constructs Unknown error
func unknown(text string) *Error {
	return newError(SemanticUnknown, text)
}

// unknownf constructs Unknown error with formatting
func unknownf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticUnknown, format, a...)
}

// wrapWithUnknown constructs Unknown error which wraps provided error
func wrapWithUnknown(err error, text string) *Error {
	return wrapError(SemanticUnknown, err, text)
}

// wrapWithUnknownf constructs Unknown error with formatting which wraps provided error
func wrapWithUnknownf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticUnknown, err, format, a...)
}

// isUnknown returns true if target error is Unknown semantic error
func isUnknown(err error) bool {
	return isSemanticError(err, SemanticUnknown)
}

// Internal constructs Internal error
func Internal(text string) *Error {
	return newError(SemanticInternal, text)
}

// Internalf constructs Internal error with formatting
func Internalf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticInternal, format, a...)
}

// WrapWithInternal constructs Internal error which wraps provided error
func WrapWithInternal(err error, text string) *Error {
	return wrapError(SemanticInternal, err, text)
}

// WrapWithInternalf constructs Internal error with formatting which wraps provided error
func WrapWithInternalf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticInternal, err, format, a...)
}

// NotImplemented constructs NotImplemented error
func NotImplemented(text string) *Error {
	return newError(SemanticNotImplemented, text)
}

// NotImplementedf constructs NotImplemented error with formatting
func NotImplementedf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticNotImplemented, format, a...)
}

// WrapWithNotImplemented constructs NotImplemented error which wraps provided error
func WrapWithNotImplemented(err error, text string) *Error {
	return wrapError(SemanticNotImplemented, err, text)
}

// WrapWithNotImplementedf constructs NotImplemented error with formatting which wraps provided error
func WrapWithNotImplementedf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticNotImplemented, err, format, a...)
}

// IsNotImplemented returns true if target error is NotImplemented semantic error
func IsNotImplemented(err error) bool {
	return isSemanticError(err, SemanticNotImplemented)
}

// Unavailable constructs Unavailable error
func Unavailable(text string) *Error {
	return newError(SemanticUnavailable, text)
}

// Unavailablef constructs Unavailable error with formatting
func Unavailablef(format string, a ...interface{}) *Error {
	return newErrorf(SemanticUnavailable, format, a...)
}

// WrapWithUnavailable constructs Unavailable error which wraps provided error
func WrapWithUnavailable(err error, text string) *Error {
	return wrapError(SemanticUnavailable, err, text)
}

// WrapWithUnavailablef constructs Unavailable error with formatting which wraps provided error
func WrapWithUnavailablef(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticUnavailable, err, format, a...)
}

// IsUnavailable returns true if target error is Unavailable semantic error
func IsUnavailable(err error) bool {
	return isSemanticError(err, SemanticUnavailable)
}

// InvalidInput constructs InvalidInput error
func InvalidInput(text string) *Error {
	return newError(SemanticInvalidInput, text)
}

// InvalidInputf constructs InvalidInput error with formatting
func InvalidInputf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticInvalidInput, format, a...)
}

// WrapWithInvalidInput constructs InvalidInput error which wraps provided error
func WrapWithInvalidInput(err error, text string) *Error {
	return wrapError(SemanticInvalidInput, err, text)
}

// WrapWithInvalidInputf constructs InvalidInput error with formatting which wraps provided error
func WrapWithInvalidInputf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticInvalidInput, err, format, a...)
}

// IsInvalidInput returns true if target error is InvalidInput semantic error
func IsInvalidInput(err error) bool {
	return isSemanticError(err, SemanticInvalidInput)
}

// Authentication constructs Authentication error
func Authentication(text string) *Error {
	return newError(SemanticAuthentication, text)
}

// Authenticationf constructs Authentication error with formatting
func Authenticationf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticAuthentication, format, a...)
}

// WrapWithAuthentication constructs Authentication error which wraps provided error
func WrapWithAuthentication(err error, text string) *Error {
	return wrapError(SemanticAuthentication, err, text)
}

// WrapWithAuthenticationf constructs Authentication error with formatting which wraps provided error
func WrapWithAuthenticationf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticAuthentication, err, format, a...)
}

// IsAuthentication returns true if target error is Authentication semantic error
func IsAuthentication(err error) bool {
	return isSemanticError(err, SemanticAuthentication)
}

// Authorization constructs Authorization error
func Authorization(text string) *Error {
	return newError(SemanticAuthorization, text)
}

// Authorizationf constructs Authorization error with formatting
func Authorizationf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticAuthorization, format, a...)
}

// WrapWithAuthorization constructs Authorization error which wraps provided error
func WrapWithAuthorization(err error, text string) *Error {
	return wrapError(SemanticAuthorization, err, text)
}

// WrapWithAuthorizationf constructs Authorization error with formatting which wraps provided error
func WrapWithAuthorizationf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticAuthorization, err, format, a...)
}

// IsAuthorization returns true if target error is Authorization semantic error
func IsAuthorization(err error) bool {
	return isSemanticError(err, SemanticAuthorization)
}

// FailedPrecondition constructs FailedPrecondition error
func FailedPrecondition(text string) *Error {
	return newError(SemanticFailedPrecondition, text)
}

// FailedPreconditionf constructs FailedPrecondition error with formatting
func FailedPreconditionf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticFailedPrecondition, format, a...)
}

// WrapWithFailedPrecondition constructs FailedPrecondition error which wraps provided error
func WrapWithFailedPrecondition(err error, text string) *Error {
	return wrapError(SemanticFailedPrecondition, err, text)
}

// WrapWithFailedPreconditionf constructs FailedPrecondition error with formatting which wraps provided error
func WrapWithFailedPreconditionf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticFailedPrecondition, err, format, a...)
}

// IsFailedPrecondition returns true if target error is FailedPrecondition semantic error
func IsFailedPrecondition(err error) bool {
	return isSemanticError(err, SemanticFailedPrecondition)
}

// NotFound constructs NotFound error
func NotFound(text string) *Error {
	return newError(SemanticNotFound, text)
}

// NotFoundf constructs NotFound error with formatting
func NotFoundf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticNotFound, format, a...)
}

// WrapWithNotFound constructs NotFound error which wraps provided error
func WrapWithNotFound(err error, text string) *Error {
	return wrapError(SemanticNotFound, err, text)
}

// WrapWithNotFoundf constructs NotFound error with formatting which wraps provided error
func WrapWithNotFoundf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticNotFound, err, format, a...)
}

// IsNotFound returns true if target error is NotFound semantic error
func IsNotFound(err error) bool {
	return isSemanticError(err, SemanticNotFound)
}

// AlreadyExists constructs AlreadyExists error
func AlreadyExists(text string) *Error {
	return newError(SemanticAlreadyExists, text)
}

// AlreadyExistsf constructs AlreadyExists error with formatting
func AlreadyExistsf(format string, a ...interface{}) *Error {
	return newErrorf(SemanticAlreadyExists, format, a...)
}

// WrapWithAlreadyExists constructs AlreadyExists error which wraps provided error
func WrapWithAlreadyExists(err error, text string) *Error {
	return wrapError(SemanticAlreadyExists, err, text)
}

// WrapWithAlreadyExistsf constructs AlreadyExists error with formatting which wraps provided error
func WrapWithAlreadyExistsf(err error, format string, a ...interface{}) *Error {
	return wrapErrorf(SemanticAlreadyExists, err, format, a...)
}

// IsAlreadyExists returns true if target error is AlreadyExists semantic error
func IsAlreadyExists(err error) bool {
	return isSemanticError(err, SemanticAlreadyExists)
}

// WhitelistErrors provided in argument, convert others into unknown errors
func WhitelistErrors(err error, semantics ...Semantic) error {
	se := AsSemanticError(err)
	if se == nil {
		return err
	}

	for _, sem := range semantics {
		if se.Semantic == sem {
			return err
		}
	}

	// Do not return any semantic error beyond those specified above
	return wrapWithUnknown(err, "unknown")
}

// WrapWellKnown errors with semantic errors.
func WrapWellKnown(err error) error {
	err, _ = WrapWellKnownChecked(err)
	return err
}

// WrapWellKnownChecked errors with semantic errors, returns true if wrapped an error.
//nolint:ST1008
func WrapWellKnownChecked(err error) (error, bool) {
	// Skip semantic errors
	var se *Error
	if xerrors.As(err, &se) {
		return err, true
	}

	// Skip context errors because other errors can wrap those (net.Error does this)
	if xerrors.Is(err, context.Canceled) || xerrors.Is(err, context.DeadlineExceeded) {
		return err, false
	}

	var netError net.Error
	if xerrors.As(err, &netError) {
		// TODO: might want to consider TLS certificate errors as different type
		return WrapWithUnavailable(err, "unavailable"), true
	}

	return err, false
}
