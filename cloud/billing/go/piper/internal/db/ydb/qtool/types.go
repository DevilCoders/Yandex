package qtool

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

var epoch = time.Unix(0, 0)

type UInt64Ts time.Time

func (d *UInt64Ts) Scan(x interface{}) error {
	v, ok := x.(uint64)
	if !ok {
		return convertError(v, x)
	}
	*d = UInt64Ts(time.Unix(int64(v), 0))
	return nil
}

func (d UInt64Ts) Value() ydb.Value {
	ts := time.Time(d).Unix()
	return ydb.Uint64Value(uint64(ts))
}

type Timestamp time.Time

func (d *Timestamp) Scan(x interface{}) error {
	v, ok := x.(uint64)
	if !ok {
		return convertError(v, x)
	}
	*d = Timestamp(time.Unix(0, int64(v)*1000).UTC())
	return nil
}

func (d Timestamp) Value() ydb.Value {
	ts := ydb.TimestampValueFromTime(time.Time(d))
	return ts
}

type Date time.Time

func (d *Date) Scan(x interface{}) error {
	v, ok := x.(uint32)
	if !ok {
		return convertError(v, x)
	}
	*d = Date(epoch.Add(time.Hour * 24 * time.Duration(v)).UTC())
	return nil
}

func (d Date) Value() ydb.Value {
	ts := ydb.DateValueFromTime(time.Time(d))
	return ts
}

type DateString time.Time

func (d *DateString) Scan(x interface{}) error {
	v, ok := x.(string)
	if !ok {
		return convertError(v, x)
	}
	t, err := time.ParseInLocation("2006-01-02", v, time.Local)
	if err != nil {
		return err
	}
	*d = DateString(t.Local())
	return nil
}

func (d DateString) Value() ydb.Value {
	dt := time.Time(d).Format("2006-01-02")
	return ydb.UTF8Value(dt)
}

type DefaultDecimal decimal.Decimal128

func (d *DefaultDecimal) Scan(x interface{}) (err error) {
	v, ok := x.(ydbsql.Decimal)
	if !ok {
		return convertError(v, x)
	}
	*(*decimal.Decimal128)(d), err = decimal.DecimalFromYDB(v.Bytes, v.Precision, v.Scale)
	return err
}

func (d DefaultDecimal) Value() ydb.Value {
	return defaultDecConverter.Convert(decimal.Decimal128(d))
}

type JSONAnything string

func (ja *JSONAnything) Scan(x interface{}) (err error) {
	if x == nil {
		*ja = "null"
		return nil
	}
	v, ok := x.(string)
	if !ok {
		return convertError(v, x)
	}
	*ja = JSONAnything(v)
	return nil
}

func (ja JSONAnything) Value() ydb.Value {
	switch ja {
	case "", "null":
		return ydb.NullValue(ydb.TypeJSON)
	default:
		return ydb.OptionalValue(ydb.JSONValue(string(ja)))
	}
}

type String string

func (s *String) Scan(x interface{}) (err error) {
	if x == nil {
		*s = ""
		return nil
	}
	v, ok := x.(string)
	if !ok {
		return convertError(v, x)
	}
	*s = String(v)
	return nil
}

func (s String) Value() ydb.Value {
	if s == "" {
		return ydb.NullValue(ydb.TypeUTF8)
	}
	return ydb.OptionalValue(ydb.UTF8Value(string(s)))
}

var defaultDecConverter = func() *decimal.YDBProducer {
	producer, err := decimal.NewYDBProducer(22, 9)
	if err != nil {
		panic(err)
	}
	return producer
}()

func convertError(dst, src interface{}) error {
	return fmt.Errorf(
		"ydb/qtool: can not convert value type %[1]T (%[1]v) to a %[2]T",
		src, dst,
	)
}
