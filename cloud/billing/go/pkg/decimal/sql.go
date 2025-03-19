package decimal

import (
	"database/sql/driver"
	"fmt"
	"strings"
)

// DBDecimal is wrapper over Decimal 128 for use in db operations.
// This type needed because of conflict in Scan method with fmt.Scanner.
type DBDecimal struct {
	Decimal128
}

// Scan implements the sql.Scanner interface for database deserialization.
func (d *DBDecimal) Scan(value interface{}) (err error) {
	var val string

	// first try to see if the data is stored in database as a Numeric datatype
	switch v := value.(type) {

	case float32:
		d.Decimal128, err = NewFromFloat64(float64(v))
		return

	case float64:
		d.Decimal128, err = NewFromFloat64(v)
		return

	case int64:
		d.Decimal128, err = FromInt64(v)
		return

	case string:
		val = v

	case []byte:
		val = string(v)

	default:
		return ErrParse.Wrap(fmt.Errorf("could not convert value '%#v'", v))
	}
	if strings.HasPrefix(val, `"`) && strings.HasSuffix(val, `"`) {
		val = val[1 : len(val)-1]
	}

	d.Decimal128, err = parse(val)
	return
}

// Value implements the driver.Valuer interface for database serialization.
func (d DBDecimal) Value() (driver.Value, error) {
	return d.String(), nil
}

// DBNullDecimal is for db operations with nullable decimal values. If value is NULL it
// will have Valid=false.
type DBNullDecimal struct {
	DBDecimal
	Valid bool
}

// Scan implements the sql.Scanner interface for database deserialization.
func (d *DBNullDecimal) Scan(value interface{}) (err error) {
	if value == nil {
		d.Valid = false
		return nil
	}
	d.Valid = true
	return d.DBDecimal.Scan(value)
}

// Value implements the driver.Valuer interface for database serialization.
func (d *DBNullDecimal) Value() (driver.Value, error) {
	if !d.Valid {
		return nil, nil
	}
	return d.DBDecimal.Value()
}
