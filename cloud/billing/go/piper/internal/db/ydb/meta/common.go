package meta

import "a.yandex-team.ru/kikimr/public/sdk/go/ydb"

var stringList = ydb.List(ydb.TypeUTF8)

func makeStringList(keys []string) ydb.Value {
	var items []ydb.Value
	for _, k := range keys {
		items = append(items, ydb.UTF8Value(k))
	}
	return ydb.ListValue(items...)
}

func notEmptyStrings(inp []string) []string {
	if len(inp) > 0 {
		return inp
	}
	return []string{""}
}
