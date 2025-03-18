package pgx

import (
	"database/sql"
	"database/sql/driver"
	"net"
	"time"

	uuid "github.com/gofrs/uuid"
	"github.com/jackc/pgtype"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Arr is a generic Array struct
type Arr struct {
	V interface{}
}

// Array is a helper function for Postgres array values manupulations.
// It implements basic database/sql interfaces.
// Example:
//     ids := []int64{42, 189032, 1489}
//     var names []string
//     err := conn.QueryRow(`
//     			SELECT
//     				array_agg(name) AS names
//     			FROM users
//     			WHERE id = ANY($1)
//     		`, pgxutil.Array(ids)).
//     	Scan(pgxutil.Array(&names))
func Array(v interface{}) interface {
	driver.Valuer
	sql.Scanner
} {
	return Arr{v}
}

// Value implements the driver.Valuer interface.
func (array Arr) Value() (driver.Value, error) {
	switch at := array.V.(type) {
	case []int16, []uint16:
		arr := new(pgtype.Int2Array)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []int, []int32, []uint, []uint32:
		arr := new(pgtype.Int4Array)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []int64, []uint64:
		arr := new(pgtype.Int8Array)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case [][16]byte:
		arr := new(pgtype.UUIDArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []uuid.UUID:
		us := make([][uuid.Size]byte, len(at))
		for i, u := range at {
			us[i] = [uuid.Size]byte(u)
		}
		arr := new(pgtype.UUIDArray)
		if err := arr.Set(us); err != nil {
			return nil, err
		}
		return arr.Value()

	case []float32:
		arr := new(pgtype.Float4Array)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []float64:
		arr := new(pgtype.Float8Array)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case [][]byte:
		arr := new(pgtype.ByteaArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []bool:
		arr := new(pgtype.BoolArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []string, VarcharArray:
		arr := new(pgtype.VarcharArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case TextArray:
		arr := new(pgtype.TextArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []net.IP, []*net.IPNet:
		arr := new(pgtype.InetArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case []time.Time:
		arr := new(pgtype.DateArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()

	case EnumArray:
		arr := new(pgtype.EnumArray)
		if err := arr.Set(at); err != nil {
			return nil, err
		}
		return arr.Value()
	}

	return nil, xerrors.Errorf("unsupported type %T", array.V)
}

// Scan implements the sql.Scanner interface.
func (array Arr) Scan(src interface{}) error {
	switch at := array.V.(type) {
	case *[]int16, *[]uint16:
		var v pgtype.Int2Array
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]int, *[]int32, *[]uint, *[]uint32:
		var v pgtype.Int4Array
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]int64, *[]uint64:
		var v pgtype.Int8Array
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[][16]byte:
		var v pgtype.UUIDArray
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]uuid.UUID:
		var v pgtype.UUIDArray
		if err := v.Scan(src); err != nil {
			return err
		}
		*at = make([]uuid.UUID, len(v.Elements))
		for i := range v.Elements {
			var u [uuid.Size]byte
			if err := v.Elements[i].AssignTo(&u); err != nil {
				return err
			}
			(*at)[i] = uuid.UUID(u)
		}
		return nil

	case *[]float32:
		var v pgtype.Float4Array
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]float64:
		var v pgtype.Float8Array
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[][]byte:
		var v pgtype.ByteaArray
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]bool:
		var v pgtype.BoolArray
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]string, *VarcharArray:
		var v pgtype.VarcharArray
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *TextArray:
		var v pgtype.TextArray
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]net.IP, *[]*net.IPNet:
		var v pgtype.InetArray
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)

	case *[]time.Time:
		var v pgtype.DateArray
		if err := v.Scan(src); err != nil {
			return err
		}
		return v.AssignTo(array.V)
	}

	return xerrors.Errorf("unsupported type %T", array.V)
}
