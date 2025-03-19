package optional

import (
	"encoding/json"
	"errors"
	"fmt"
	"reflect"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
)

// ErrMissing happens when optional has no value set
var ErrMissing = errors.New("optional has no value")

const mustErrorMsg = "requested must-have optional value but its missing"

// Bool holds bool value
type Bool struct {
	Bool  bool
	Valid bool
}

// NewBool creates new bool value
func NewBool(v bool) Bool {
	return Bool{Bool: v, Valid: true}
}

// Set value
func (o *Bool) Set(v bool) {
	o.Bool = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Bool) Get() (bool, error) {
	if !o.Valid {
		return false, ErrMissing
	}

	return o.Bool, nil
}

// Must returns value if its set or throws panic
func (o *Bool) Must() bool {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Bool
}

// BoolPointer holds bool pointer value
type BoolPointer struct {
	Bool  *bool
	Valid bool
}

// NewBool creates new bool value
func NewBoolPointer(v *bool) BoolPointer {
	return BoolPointer{Bool: v, Valid: true}
}

// Set value
func (o *BoolPointer) Set(v *bool) {
	o.Bool = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *BoolPointer) Get() (*bool, error) {
	if !o.Valid {
		return nil, ErrMissing
	}

	return o.Bool, nil
}

// Must returns value if its set or throws panic
func (o *BoolPointer) Must() *bool {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Bool
}

// String holds string value
type String struct {
	String string
	Valid  bool
}

// NewString creates new string value
func NewString(v string) String {
	return String{String: v, Valid: true}
}

// Set value
func (o *String) Set(v string) {
	o.String = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *String) Get() (string, error) {
	if !o.Valid {
		return "", ErrMissing
	}

	return o.String, nil
}

// Must returns value if its set or throws panic
func (o *String) Must() string {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.String
}

// Strings holds []string value
type Strings struct {
	Strings []string
	Valid   bool
}

func NewStrings(v []string) Strings {
	return Strings{
		Strings: v,
		Valid:   true,
	}
}

// Set value
func (o *Strings) Set(v []string) {
	o.Strings = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Strings) Get() ([]string, error) {
	if !o.Valid {
		return nil, ErrMissing
	}

	return o.Strings, nil
}

// Must returns value if its set or throws panic
func (o *Strings) Must() []string {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Strings
}

// Int64 holds int64 value
type Int64 struct {
	Int64 int64
	Valid bool
}

func (o *Int64) MarshalJSON() ([]byte, error) {
	if o == nil || !o.Valid {
		return json.Marshal(nil)
	}
	return json.Marshal(o.Int64)
}

func (o *Int64) UnmarshalJSON(b []byte) error {
	// accepts both int64 and optional.Int64 as an input
	if len(b) == 0 {
		// nil?
		o.Int64 = int64(0)
		o.Valid = true
		return nil
	}

	if err := json.Unmarshal(b, &o.Int64); err == nil {
		// int64?
		o.Valid = true
		return nil
	}

	m := make(map[string]interface{})
	if err := json.Unmarshal(b, &m); err != nil {
		// optional.Int64?
		return err
	}

	for k, v := range m {
		if k == "Int64" {
			switch t := v.(type) {
			case int64:
				o.Int64 = t
			case int:
				o.Int64 = int64(t)
			case float64:
				o.Int64 = int64(t)
			default:
				return fmt.Errorf("type Int64 conversion error: %T, %+v", v, v)
			}
		}

		if k == "Valid" {
			switch t := v.(type) {
			case bool:
				o.Valid = t
			default:
				return fmt.Errorf("type Valid conversion error: %T, %+v", v, v)
			}
		}
	}

	return nil
}

// NewInt64 creates new int64 value
func NewInt64(v int64) Int64 {
	return Int64{Int64: v, Valid: true}
}

// Set value
func (o *Int64) Set(v int64) {
	o.Int64 = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Int64) Get() (int64, error) {
	if !o.Valid {
		return 0, ErrMissing
	}

	return o.Int64, nil
}

// Must returns value if its set or throws panic
func (o *Int64) Must() int64 {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Int64
}

// Int64Pointer holds pointer to int64 value
type Int64Pointer struct {
	Int64 *int64
	Valid bool
}

// NewInt64 creates new int64 value
func NewInt64Pointer(v *int64) Int64Pointer {
	return Int64Pointer{Int64: v, Valid: true}
}

// Set value
func (o *Int64Pointer) Set(v *int64) {
	o.Int64 = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Int64Pointer) Get() (*int64, error) {
	if !o.Valid {
		return nil, ErrMissing
	}

	return o.Int64, nil
}

// Must returns value if its set or throws panic
func (o *Int64Pointer) Must() *int64 {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Int64
}

// Uint64 holds uint64 value
type Uint64 struct {
	Uint64 uint64
	Valid  bool
}

// NewUint64 creates new uint64 value
func NewUint64(v uint64) Uint64 {
	return Uint64{Uint64: v, Valid: true}
}

// Set value
func (o *Uint64) Set(v uint64) {
	o.Uint64 = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Uint64) Get() (uint64, error) {
	if !o.Valid {
		return 0, ErrMissing
	}

	return o.Uint64, nil
}

// Must returns value if its set or throws panic
func (o *Uint64) Must() uint64 {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Uint64
}

// Float64 holds float64 value
type Float64 struct {
	Float64 float64
	Valid   bool
}

// NewFloat64 creates new float64 value
func NewFloat64(v float64) Float64 {
	return Float64{Float64: v, Valid: true}
}

// Set value
func (o *Float64) Set(v float64) {
	o.Float64 = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Float64) Get() (float64, error) {
	if !o.Valid {
		return 0, ErrMissing
	}

	return o.Float64, nil
}

// Must returns value if its set or throws panic
func (o *Float64) Must() float64 {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Float64
}

// Time holds time.Time value
type Time struct {
	Time  time.Time
	Valid bool
}

// NewTime creates new time.Time value
func NewTime(v time.Time) Time {
	return Time{Time: v, Valid: true}
}

// Set value
func (o *Time) Set(v time.Time) {
	o.Time = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Time) Get() (time.Time, error) {
	if !o.Valid {
		return time.Time{}, ErrMissing
	}

	return o.Time, nil
}

// Must returns value if its set or throws panic
func (o *Time) Must() time.Time {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Time
}

// Duration holds time.Duration value
type Duration struct {
	Duration time.Duration
	Valid    bool
}

// NewDuration creates new time.Duration value
func NewDuration(v time.Duration) Duration {
	return Duration{Duration: v, Valid: true}
}

// Set value
func (o *Duration) Set(v time.Duration) {
	o.Duration = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *Duration) Get() (time.Duration, error) {
	if !o.Valid {
		return time.Duration(0), ErrMissing
	}

	return o.Duration, nil
}

// Must returns value if its set or throws panic
func (o *Duration) Must() time.Duration {
	if !o.Valid {
		panic(mustErrorMsg)
	}

	return o.Duration
}

// OptionalPassword holds Password value
type OptionalPassword struct {
	Password secret.String
	Valid    bool
}

// OptionalPassword creates new password value
func NewOptionalPassword(p secret.String) OptionalPassword {
	return OptionalPassword{Password: p, Valid: true}
}

// Set value
func (o *OptionalPassword) Set(v secret.String) {
	o.Password = v
	o.Valid = true
}

// Get value if its set or return an error
func (o *OptionalPassword) Get() (secret.String, error) {
	if !o.Valid {
		return secret.String{}, ErrMissing
	}

	return o.Password, nil
}

func MarshalStructWithOnlyOptionalFields(s interface{}) ([]byte, error) {
	m := make(map[string]interface{})
	valueOfConfig := reflect.ValueOf(s).Elem()
	for i := 0; i < valueOfConfig.NumField(); i++ {
		fieldInfo := valueOfConfig.Type().Field(i)
		isValid := valueOfConfig.Field(i).FieldByName("Valid").Interface().(bool)
		if isValid {
			m[fieldInfo.Tag.Get("json")] = valueOfConfig.Field(i).Field(0).Interface()
		}
	}
	return json.Marshal(m)
}

func UnmarshalToStructWithOnlyOptionalFields(b []byte, s interface{}) error {
	m := make(map[string]interface{})
	if err := json.Unmarshal(b, &m); err != nil {
		return err
	}
	valueOfConfig := reflect.ValueOf(s).Elem()
	for i := 0; i < valueOfConfig.NumField(); i++ {
		fieldInfo := valueOfConfig.Type().Field(i)
		value, ok := m[fieldInfo.Tag.Get("json")]
		if !ok {
			continue
		}
		setConfigValue := valueOfConfig.Field(i).Addr().MethodByName("Set")
		if fieldInfo.Type == reflect.TypeOf(NewInt64(0)) {
			int64Value := int64(value.(float64))
			setConfigValue.Call([]reflect.Value{reflect.ValueOf(int64Value)})
		} else if fieldInfo.Type == reflect.TypeOf(NewUint64(0)) {
			uint64Value := uint64(value.(float64))
			setConfigValue.Call([]reflect.Value{reflect.ValueOf(uint64Value)})
		} else {
			setConfigValue.Call([]reflect.Value{reflect.ValueOf(value)})
		}

	}
	return nil
}
