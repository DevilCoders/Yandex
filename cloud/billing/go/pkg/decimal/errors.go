package decimal

import (
	"errors"

	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var (
	ErrRange  = errsentinel.New("value out of range for decimal type")
	ErrSyntax = errsentinel.New("invalid syntax for decimal literal")
	ErrParse  = errsentinel.New("parsing error")
	ErrScan   = errsentinel.New("decimal scan error")

	ErrNan = errors.New("NaN value unsupported") //nolint:ST1005
	ErrInf = errors.New("Inf value unsupported") //nolint:ST1005
)
