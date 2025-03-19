package adapters

import "a.yandex-team.ru/library/go/core/xerrors"

var (
	// ErrInternalServerError request failed with .
	ErrInternalClientError = xerrors.New("remote service internal error")

	// ErrInternalServerError internal server error.
	ErrInternalServerError = xerrors.New("internal service error")

	// ErrNotAuthorized no permissions error.
	ErrNotAuthorized = xerrors.New("not authorized")

	ErrNotFound = xerrors.New("not found")

	// ErrNotAuthenticated wrong (or unrecognized) credentials provided.
	ErrNotAuthenticated = xerrors.New("authentication credentials were not provided or given token not valid for any token type.")

	// ErrBackendTimeout marks the timeout of one of upstream subservices.
	ErrBackendTimeout = xerrors.New("backend timeout")
)
