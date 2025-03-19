package qtool

import (
	"bytes"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

const (
	v1SyntaxComment stringTemplate = "--!syntax_v1\n"
)

var (
	InfTime         = time.Date(99990, 12, 31, 0, 0, 0, 0, time.UTC).Local()
	InfTS           = uint64(InfTime.Unix())
	InfTSDecl       = fmt.Sprintf("$inf_ts = CAST(%d as UInt64);", InfTS)
	DecimalZeroDecl = `$zero = CAST("0" as Decimal(22,9));`
)

type QueryString interface{}

func Query(lines ...QueryString) Template {
	t := Template{v1SyntaxComment}

	for _, l := range lines {
		t = append(t, queryLineToTemplate(l)...)
	}
	return t.simplify()
}

func Table(name string) QueryString {
	return tableName(name)
}

func TableAs(name string, as string) QueryString {
	return Template{tableName(name), stringTemplate("as "), stringTemplate(as), stringTemplate("\n")}
}

func TypeToString(t ydb.Type) string {
	buf := bytes.Buffer{}
	ydb.WriteTypeStringTo(&buf, t)
	return buf.String()
}

func Declare(name string, t ydb.Type) string {
	return fmt.Sprintf("DECLARE $%s AS %s;", name, TypeToString(t))
}

func NamedQuery(name string, lines ...QueryString) QueryString {
	t := Template{
		stringTemplate("$"),
		stringTemplate(name),
		stringTemplate("=(\n"),
	}

	for _, l := range lines {
		t = append(t, queryLineToTemplate(l)...)
	}
	t = append(t, stringTemplate(");\n"))

	return t.simplify()
}

func ReplaceFromValues(table string, columns ...string) Template {
	colList := strings.Join(columns, ",")

	t := Template{
		stringTemplate("REPLACE INTO"),
		tableName(table),
		stringTemplate("("),
		stringTemplate(colList),
		stringTemplate(")\nSELECT "),
		stringTemplate(colList),
		stringTemplate("\n FROM AS_TABLE($values);"),
	}
	return t.simplify()
}

func PrefixedCols(prefix string, columns ...QueryString) QueryString {
	t := Template{}
	for i, c := range columns {
		if i > 0 {
			t = append(t, stringTemplate(","))
		}
		t = append(t, stringTemplate(prefix), stringTemplate("."))
		t = append(t, queryStringToTemplate(c)...)
		t = append(t, stringTemplate(" as "))
		t = append(t, queryStringToTemplate(c)...)
	}
	t = append(t, stringTemplate(" "))
	return t.simplify()
}

func Cols(columns ...QueryString) QueryString {
	t := Template{}
	for i, c := range columns {
		if i > 0 {
			t = append(t, stringTemplate(","))
		}
		t = append(t, queryStringToTemplate(c)...)
	}
	t = append(t, stringTemplate("\n"))
	return t.simplify()
}
