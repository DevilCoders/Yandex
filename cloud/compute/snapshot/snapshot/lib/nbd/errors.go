package nbd

import (
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
)

var (
	// Public`

	// ErrNoFreeNbd ...
	ErrNoFreeNbd = common.NewError("ErrNoFreeNbd", "no free nbd device found", false)
	// ErrNbdInternal ...
	ErrNbdInternal = common.NewError("ErrNbdInternal", "nbd internal error", false)

	// Private

	// ErrNotNbdDevice ...
	ErrNotNbdDevice = common.NewError("ErrNotNbdDevice", "provided path is not nbd device", false)
	// ErrNbdTimeout ...
	ErrNbdTimeout = common.NewError("ErrNbdTimeout", "nbd device timed out", false)
)
