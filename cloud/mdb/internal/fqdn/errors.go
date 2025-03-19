package fqdn

import (
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrInvalidFQDN = xerrors.NewSentinel("invalid fqdn")
)
