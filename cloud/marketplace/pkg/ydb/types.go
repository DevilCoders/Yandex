package ydb

import (
	"bytes"
	"encoding/json"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type AnyJSON string

func (a AnyJSON) Bytes() []byte {
	return []byte(a)
}

func NewAnyJSON(in string) *AnyJSON {
	out := AnyJSON(in)
	return &out
}

type QueryBuilder struct {
	tableRoot string
}

func NewQueryBuilder(tableRoot string) QueryBuilder {
	return QueryBuilder{
		tableRoot: tableRoot,
	}
}

func MakeStringsList(fieldName string, values []string) ydb.Value {
	var items []ydb.Value
	for _, k := range values {
		items = append(items, ydb.StructValue(
			ydb.StructFieldValue(fieldName, ydb.UTF8Value(k)),
		))
	}

	if len(items) == 0 {
		return ydb.ListValue()
	}

	return ydb.ListValue(items...)
}

func (QueryBuilder) DeclareType(variable string, t ydb.Type) string {
	return fmt.Sprintf("DECLARE $%s AS %s;", variable, typeToString(t))
}

func (QueryBuilder) TableExpression(variable string) string {
	return fmt.Sprintf("AS_TABLE($%s)", variable)
}

func (QueryBuilder) Quote(variable string) string {
	return fmt.Sprintf("`%s`", variable)
}

func (q QueryBuilder) TablePath(path string) string {
	var (
		builder strings.Builder
		root    = q.tableRoot
	)

	if root != "" && !strings.HasSuffix(root, "/") {
		root += "/"
	}

	fmt.Fprintf(&builder, "`%s%s`", root, path)

	return builder.String()
}

func typeToString(t ydb.Type) string {
	var buf bytes.Buffer
	ydb.WriteTypeStringTo(&buf, t)
	return buf.String()
}

func MakeStringStructType(fieldName string) ydb.Type {
	return ydb.Struct(
		ydb.StructField(fieldName, ydb.TypeUTF8),
	)
}

func MakeListOfStringStructType(fieldName string) ydb.Type {
	return ydb.List(MakeStringStructType(fieldName))
}

var epoch = time.Unix(0, 0)

type UInt64Ts time.Time

func (d *UInt64Ts) Scan(x interface{}) error {
	v, ok := x.(uint64)
	if !ok {
		return convertError(v, x)
	}
	*d = UInt64Ts(time.Unix(int64(v), 0).UTC())
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

func (a *AnyJSON) Scan(x interface{}) (err error) {
	if x == nil {
		*a = "null"
		return nil
	}
	v, ok := x.(string)
	if !ok {
		return convertError(v, x)
	}
	*a = AnyJSON(v)
	return nil
}

func (a AnyJSON) Value() ydb.Value {
	switch a {
	case "", "null":
		return ydb.NullValue(ydb.TypeJSON)
	default:
		return ydb.OptionalValue(ydb.JSONValue(string(a)))
	}
}

type ListStringJSON []string

func (lsj *ListStringJSON) Scan(x interface{}) (err error) {
	if x == nil {
		*lsj = nil
		return nil
	}
	v, ok := x.(string)
	if !ok {
		return convertError(v, x)
	}
	out := []string{}
	err = json.Unmarshal([]byte(v), &out)
	if err != nil {
		return err
	}
	*lsj = ListStringJSON(out)
	return nil
}

func (lsj ListStringJSON) Value() ydb.Value {
	switch lsj {
	case nil:
		return ydb.NullValue(ydb.TypeJSON)
	default:
		listStringJSON, _ := json.Marshal(lsj)
		return ydb.OptionalValue(ydb.JSONValue(string(listStringJSON)))
	}
}

type MapStringStringJSON map[string]string

func (m *MapStringStringJSON) Scan(x interface{}) (err error) {
	if x == nil {
		*m = nil
		return nil
	}
	v, ok := x.(string)
	if !ok {
		return convertError(v, x)
	}
	out := make(map[string]string)
	err = json.Unmarshal([]byte(v), &out)
	if err != nil {
		return err
	}
	*m = MapStringStringJSON(out)
	return nil
}

func (m MapStringStringJSON) Value() ydb.Value {
	switch m {
	case nil:
		return ydb.NullValue(ydb.TypeJSON)
	default:
		mapStringStringJSON, _ := json.Marshal(m)
		return ydb.OptionalValue(ydb.JSONValue(string(mapStringStringJSON)))
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

func convertError(dst, src interface{}) error {
	return fmt.Errorf(
		"ydb/qtool: can not convert value type %[1]T (%[1]v) to a %[2]T",
		src, dst,
	)
}
