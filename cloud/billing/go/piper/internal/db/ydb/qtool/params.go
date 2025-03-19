package qtool

import "a.yandex-team.ru/kikimr/public/sdk/go/ydb"

type structureGenerator interface {
	YDBStruct() ydb.Value
}

type ListBuilder struct {
	items []ydb.Value
}

func ListValues() *ListBuilder {
	return &ListBuilder{}
}

func (lb *ListBuilder) Add(i structureGenerator) *ListBuilder {
	lb.items = append(lb.items, i.YDBStruct())
	return lb
}

func (lb *ListBuilder) List() ydb.Value {
	return ydb.ListValue(lb.items...)
}
