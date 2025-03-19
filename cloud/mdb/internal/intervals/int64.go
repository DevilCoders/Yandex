package intervals

import (
	"fmt"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type Int64 struct {
	min BoundInt64
	max BoundInt64
}

func NewInt64(min int64, minType BoundType, max int64, maxType BoundType) (Int64, error) {
	i := Int64{
		min: BoundInt64{Value: min, Type: minType},
		max: BoundInt64{Value: max, Type: maxType},
	}

	if err := i.validate(); err != nil {
		return Int64{}, err
	}

	return i, nil
}

func MustNewInt64(min int64, minType BoundType, max int64, maxType BoundType) Int64 {
	i, err := NewInt64(min, minType, max, maxType)
	if err != nil {
		panic(err)
	}

	return i
}

func (i Int64) String() string {
	var min string
	if i.min.Type == Unbounded {
		min = i.min.Type.MinString()
	} else {
		min = fmt.Sprintf("%s%d", i.min.Type.MinString(), i.min.Value)
	}

	var max string
	if i.max.Type == Unbounded {
		max = i.max.Type.MaxString()
	} else {
		max = fmt.Sprintf("%d%s", i.max.Value, i.max.Type.MaxString())
	}

	return fmt.Sprintf("%s,%s", min, max)
}

func (i Int64) validate() error {
	// Valid if any value is unbounded (unbounded value will be ignored)
	if i.min.Type == Unbounded || i.max.Type == Unbounded {
		return nil
	}

	diff := i.max.Value - i.min.Value

	// Invalid if min > max for any bounded interval
	if diff < 0 {
		return xerrors.Errorf("min value cannot be more than max value for bound intervals: %s", i)
	}

	if i.min.Type == Exclusive && i.max.Type == Exclusive {
		if diff < 2 {
			return xerrors.Errorf("no valid values in interval with exclusive bounds: %s", i)
		}
	}

	if (i.min.Type == Exclusive && i.max.Type == Inclusive) ||
		(i.min.Type == Inclusive && i.max.Type == Exclusive) {
		if diff < 1 {
			return xerrors.Errorf("no valid values in interval with one exclusive bound: %s", i)
		}
	}

	return nil
}

func (i Int64) Includes(v int64) bool {
	if !i.min.inMinBound(v) {
		return false
	}

	if !i.max.inMaxBound(v) {
		return false
	}

	return true
}

func (i Int64) Min() int64 {
	switch i.min.Type {
	case Inclusive:
		return i.min.Value
	case Exclusive:
		return i.min.Value + 1
	default:
		return 0
	}
}

type BoundInt64 struct {
	Value int64
	Type  BoundType
}

func (b BoundInt64) inMinBound(v int64) bool {
	switch b.Type {
	case Inclusive:
		if v < b.Value {
			return false
		}
	case Exclusive:
		if v <= b.Value {
			return false
		}
	}

	return true
}

func (b BoundInt64) inMaxBound(v int64) bool {
	switch b.Type {
	case Inclusive:
		if v > b.Value {
			return false
		}
	case Exclusive:
		if v >= b.Value {
			return false
		}
	}

	return true
}
