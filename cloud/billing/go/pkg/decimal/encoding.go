package decimal

import "strings"

// MarshalJSON implements json.Marshaler. Value marshals as Number if it is safe for float representation
// ans as string in other case.
func (d Decimal128) MarshalJSON() ([]byte, error) {
	if d.IsNan() {
		return nil, ErrNan
	}

	val := d.String()
	if quoteNeeded(val) {
		return []byte(`"` + val + `"`), nil
	}
	return []byte(val), nil
}

// UnmarshalJSON implements json.Unmarshaler. Value can be number or json string.
func (d *Decimal128) UnmarshalJSON(data []byte) (err error) {
	if data[0] == '"' {
		data = data[1 : len(data)-1]
	}
	*d, err = parse(string(data))

	if d.IsNan() {
		return ErrNan
	}
	return
}

// UnmarshalYAML implements yaml.Unmarshaler. Value in source should be string.
func (d *Decimal128) UnmarshalYAML(unmarshal func(interface{}) error) (err error) {
	var val string

	if err = unmarshal(&val); err != nil {
		return err
	}

	*d, err = parse(val)
	return err
}

func quoteNeeded(v string) bool {
	strLen := len(v)
	if strings.HasPrefix(v, "-") {
		strLen--
	}

	if strLen <= unquotedDecimals {
		return false
	}
	if strings.IndexByte(v, '.') >= 0 {
		return strLen > unquotedDecimals+1
	}
	return true
}
