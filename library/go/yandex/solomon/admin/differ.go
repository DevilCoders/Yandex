package admin

import (
	"reflect"
	"sort"
)

type DiffKeyer interface {
	DiffKey() string
}

type Differ interface {
	Expected(i int) string
	ExpectedLen() int

	Actual(i int) string
	ActualLen() int

	Equal(i, j int) bool
}

type Update struct {
	From, To int
}

func Diff(differ Differ) (add, remove []int, update []Update) {
	actual := map[string]int{}
	for i := 0; i < differ.ActualLen(); i++ {
		actual[differ.Actual(i)] = i
	}

	for i := 0; i < differ.ExpectedLen(); i++ {
		j, present := actual[differ.Expected(i)]
		if present {
			if !differ.Equal(i, j) {
				update = append(update, Update{From: i, To: j})
			}
		} else {
			add = append(add, i)
		}

		delete(actual, differ.Expected(i))
	}

	for _, i := range actual {
		remove = append(remove, i)
	}
	sort.Ints(remove)

	return
}

type reflectDiffer struct {
	v     interface{}
	equal interface{}
}

func (r reflectDiffer) Expected(i int) string {
	return reflect.ValueOf(r.v).Elem().FieldByName("Expected").Index(i).Interface().(DiffKeyer).DiffKey()
}

func (r reflectDiffer) ExpectedLen() int {
	return reflect.ValueOf(r.v).Elem().FieldByName("Expected").Len()
}

func (r reflectDiffer) Actual(i int) string {
	return reflect.ValueOf(r.v).Elem().FieldByName("Actual").Index(i).Interface().(DiffKeyer).DiffKey()
}

func (r reflectDiffer) ActualLen() int {
	return reflect.ValueOf(r.v).Elem().FieldByName("Actual").Len()
}

func (r reflectDiffer) Equal(i, j int) bool {
	expected := reflect.ValueOf(r.v).Elem().FieldByName("Expected").Index(i)
	actual := reflect.ValueOf(r.v).Elem().FieldByName("Actual").Index(j)

	result := reflect.ValueOf(r.equal).Call([]reflect.Value{expected, actual})
	return result[0].Bool()
}

func ReflectDiff(v interface{}, equal interface{}) Differ {
	return reflectDiffer{v: v, equal: equal}
}
