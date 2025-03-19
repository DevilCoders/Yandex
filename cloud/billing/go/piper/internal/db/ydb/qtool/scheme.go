package qtool

import (
	"bytes"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

func TableSpec(pk []string, cols ...TableColDesc) string {
	buf := &bytes.Buffer{}
	for i, c := range cols {
		if i > 0 {
			_, _ = buf.WriteString(",\n")
		}
		_, _ = buf.WriteString(c.Name)
		_ = buf.WriteByte(' ')
		ydb.WriteTypeStringTo(buf, c.Type)
	}

	if len(pk) == 0 {
		return buf.String()
	}
	if buf.Len() > 0 {
		_, _ = buf.WriteString(",\n")
	}
	_, _ = buf.WriteString("PRIMARY KEY (")
	for i, pkc := range pk {
		if i > 0 {
			_ = buf.WriteByte(',')
		}
		buf.WriteString(pkc)
	}
	_ = buf.WriteByte(')')
	return buf.String()
}

type TableColDesc struct {
	Name string
	Type ydb.Type
}

func TableCol(name string, tp ydb.Type) TableColDesc {
	return TableColDesc{Name: name, Type: tp}
}
