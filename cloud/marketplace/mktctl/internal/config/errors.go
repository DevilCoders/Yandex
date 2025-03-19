package config

import "errors"

var (
	ErrProfileNotSet   = errors.New("no profile set as current")
	ErrProfileNotFound = errors.New("profile not found")
)
