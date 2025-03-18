package timestamp

import (
	"database/sql"
	"database/sql/driver"
	"encoding"
	"encoding/json"
	"fmt"
	"strconv"
	"time"
)

var _ encoding.TextUnmarshaler = new(Timestamp)
var _ json.Marshaler = new(Timestamp)
var _ json.Unmarshaler = new(Timestamp)
var _ sql.Scanner = new(Timestamp)
var _ driver.Valuer = new(Timestamp)

type Timestamp struct {
	time.Time
}

// Now returns new current timestamp
func Now() Timestamp {
	return Timestamp{Time: time.Now()}
}

// UnmarshalText implements TextUnmarshaler
func (t *Timestamp) UnmarshalText(b []byte) error {
	v, err := strconv.ParseInt(string(b), 10, 64)
	if err != nil {
		return err
	}

	*t = Timestamp{time.Unix(v, 0)}
	return nil
}

// UnmarshalJSON implements json.Unmarshaler
func (t *Timestamp) UnmarshalJSON(b []byte) error {
	return t.UnmarshalText(b)
}

// MarshalJSON implements json.Marshaler
func (t Timestamp) MarshalJSON() ([]byte, error) {
	return json.Marshal(strconv.FormatInt(t.Unix(), 10))
}

// Scan implements sql.Scanner
func (t *Timestamp) Scan(src interface{}) error {
	switch src := src.(type) {
	case []byte:
		return t.UnmarshalText(src)
	case string:
		return t.UnmarshalText([]byte(src))
	case time.Time:
		*t = Timestamp{src}
	case int:
		*t = Timestamp{time.Unix(int64(src), 0)}
	case int64:
		*t = Timestamp{time.Unix(src, 0)}
	case uint:
		*t = Timestamp{time.Unix(int64(src), 0)}
	case uint64:
		*t = Timestamp{time.Unix(int64(src), 0)}
	default:
		return fmt.Errorf("cannot convert %T to Timestamp", src)
	}
	return nil
}

// Value implements sql.Valuer so that QR can be written to databases transparently.
func (t Timestamp) Value() (driver.Value, error) {
	return t.Time, nil
}
