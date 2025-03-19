package pgutil

import (
	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/internal/intervals"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func boundTypeFromPGX(bt pgtype.BoundType) (intervals.BoundType, error) {
	switch bt {
	case pgtype.Inclusive:
		return intervals.Inclusive, nil
	case pgtype.Exclusive:
		return intervals.Exclusive, nil
	case pgtype.Unbounded:
		return intervals.Unbounded, nil
	case pgtype.Empty:
		// This should never happen - empty bound type must be handled before calling this function
		panic("empty bound type")
	}

	// This should never happen
	return 0, xerrors.Errorf("unknown bound type %q", bt)
}

func IntervalInt64FromPGX(r pgtype.Int8range) (intervals.Int64, error) {
	if r.Status != pgtype.Present {
		return intervals.Int64{}, nil
	}

	if r.LowerType == pgtype.Empty || r.UpperType == pgtype.Empty {
		return intervals.Int64{}, nil
	}

	lowerType, err := boundTypeFromPGX(r.LowerType)
	if err != nil {
		return intervals.Int64{}, err
	}

	upperType, err := boundTypeFromPGX(r.UpperType)
	if err != nil {
		return intervals.Int64{}, err
	}

	return intervals.NewInt64(r.Lower.Int, lowerType, r.Upper.Int, upperType)
}

func OptionalIntervalInt64FromPGX(r pgtype.Int8range) (optional.IntervalInt64, error) {
	if r.Status != pgtype.Present {
		return optional.IntervalInt64{}, nil
	}

	i, err := IntervalInt64FromPGX(r)
	if err != nil {
		return optional.IntervalInt64{}, err
	}

	return optional.IntervalInt64{IntervalInt64: i, Valid: true}, nil
}
