package errors

import "golang.org/x/xerrors"

var (
	ErrNotFound              = xerrors.New("not found")
	ErrExpectedLenOneorLess  = xerrors.New("expected one or less")
	ErrThereOneMoreMigration = xerrors.New("there more than one migration")
	ErrUnexpectedError       = xerrors.New("unexpected error")
)
