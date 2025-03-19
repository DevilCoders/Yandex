package sqlerrors

import "a.yandex-team.ru/library/go/core/xerrors"

// TODO: think about unexposed semantic errors so that we won't have to
// specify error types for NotFound, AlreadyExists and other such stuff
var (
	ErrAlreadyExists = xerrors.NewSentinel("already exists")
	ErrNotFound      = xerrors.NewSentinel("not found")
)
