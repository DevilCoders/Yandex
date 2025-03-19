package types

import (
	"encoding/json"
	"fmt"
	"strconv"
	"time"

	jlexer "github.com/mailru/easyjson/jlexer"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type JSONAnything string

func (a *JSONAnything) UnmarshalJSON(data []byte) error {
	*a = JSONAnything(string(data))
	return nil
}

func (a JSONAnything) MarshalJSON() ([]byte, error) {
	return []byte(a), nil
}

type JSONAnyString string

func (a *JSONAnyString) UnmarshalJSON(data []byte) error {
	if data[0] == '"' {
		return json.Unmarshal(data, (*string)(a))
	}
	*a = JSONAnyString(string(data))
	return nil
}

func (a JSONAnyString) MarshalJSON() ([]byte, error) {
	return json.Marshal(string(a))
}

func (a JSONAnyString) String() string {
	return string(a)
}

type JSONTimestamp time.Time

func (t *JSONTimestamp) UnmarshalJSON(data []byte) error {
	v, err := strconv.Atoi(string(data))
	if err != nil {
		return fmt.Errorf("timestamp parse error: %w", err)
	}
	if v <= 0 {
		return nil
	}

	*t = JSONTimestamp(time.Unix(int64(v), 0))
	return nil
}

func (t JSONTimestamp) MarshalJSON() ([]byte, error) {
	tm := time.Time(t)
	if tm.IsZero() {
		return []byte{'0'}, nil
	}
	return json.Marshal(time.Time(t).Unix())
}

func (t JSONTimestamp) Time() time.Time {
	return time.Time(t)
}

// JSONDecimal is for use in direct keys of json metric. Caused by some issues during push to distinct field of YT table
type JSONDecimal decimal.Decimal128

func (d *JSONDecimal) UnmarshalJSON(data []byte) error {
	return (*decimal.Decimal128)(d).UnmarshalJSON(data)
}

func (d JSONDecimal) MarshalJSON() ([]byte, error) {
	return []byte(`"` + decimal.Decimal128(d).String() + `"`), nil
}

func (d JSONDecimal) Decimal128() decimal.Decimal128 {
	return decimal.Decimal128(d)
}

type JSONLabels map[string]JSONAnyString

func (v *JSONLabels) UnmarshalJSON(data []byte) error {
	r := jlexer.Lexer{Data: data}
	v.UnmarshalEasyJSON(&r)
	return r.Error()
}

func (v *JSONLabels) UnmarshalEasyJSON(l *jlexer.Lexer) {
	if l.IsStart() {
		defer l.Consumed()
	}
	switch {
	case l.IsNull():
		l.Skip()
	case l.IsDelim('['): // Empty list is acceptable
		l.Delim('[')
		l.Delim(']')
	default:
		l.Delim('{')
		*v = make(JSONLabels)
		for !l.IsDelim('}') {
			key := string(l.String())
			l.WantColon()
			var value JSONAnyString
			if data := l.Raw(); l.Ok() {
				l.AddError((value).UnmarshalJSON(data))
			}
			(*v)[key] = value
			l.WantComma()
		}
		l.Delim('}')
	}
}
