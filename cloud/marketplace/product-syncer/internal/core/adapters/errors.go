package adapters

import "a.yandex-team.ru/library/go/core/xerrors"

var (
	// NOTE: mostly billing error, should be refactored out to separate package once more adapters will be added.

	// ErrNotFound failed to find specified resource.
	ErrNotFound = xerrors.New("requested resource not found")

	// ErrInternalServerError request failed with .
	ErrInternalClientError = xerrors.New("remote service internal error")

	// ErrInternalServerError internal server error.
	ErrInternalServerError = xerrors.New("internal service error")

	// ErrNotAuthorized no permissions error.
	ErrNotAuthorized = xerrors.New("not authorized")

	// ErrNotAuthenticated wrong (or unrecognized) credentials provided.
	ErrNotAuthenticated = xerrors.New("authentication credentials were not provided or given token not valid for any token type.")
)
