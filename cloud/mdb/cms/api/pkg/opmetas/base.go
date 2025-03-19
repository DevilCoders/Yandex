package opmetas

import "a.yandex-team.ru/library/go/core/xerrors"

type OpMeta interface {
}

var UnknownOpTypeErr = xerrors.NewSentinel("unknown operation type")
