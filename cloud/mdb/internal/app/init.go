package app

import (
	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func init() {
	xerrors.EnableStackThenFrames()
	flags.RegisterLogLevelFlagGlobal()
}
