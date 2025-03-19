package pg

import (
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
)

func errorToSemErr(err error) error {
	code, ok := pgerrors.Code(err)
	if !ok {
		return err
	}

	switch code {
	case "MDD01": // Already registered
		return semerr.FailedPrecondition(err.Error())
	case "MDD02": // Registration timeout
		return semerr.InvalidInput(err.Error())
	case "MDD03": // Not found
		return semerr.NotFound(err.Error())
	case "MDD04": // No open master
		return semerr.FailedPrecondition(err.Error())
	case "MDD05": // Invalid state
		return semerr.FailedPrecondition(err.Error())
	case "MDD06": // Invalid input
		return semerr.InvalidInput(err.Error())
	default:
		return err
	}
}
