package reflectutil

import (
	"reflect"

	"a.yandex-team.ru/library/go/slices"
)

func IgnoreMissingFields(missingInSrc bool, missingInDst bool) bool {
	return false
}

func AllFieldsAreValid(_ string, _ interface{}) bool {
	return true
}

func FieldsInListAreValid(fields []string) func(string, interface{}) bool {
	return func(name string, val interface{}) bool {
		return slices.ContainsString(fields, name)
	}
}

func ValidOptionalFieldsAreValid(_ string, field interface{}) bool {
	rv := reflect.ValueOf(field)
	if rv.Kind() != reflect.Struct {
		return false
	}
	fv := rv.FieldByName("Valid")
	if fv.Kind() != reflect.Bool {
		return false
	}
	return fv.Bool()
}

func CopyFieldsAsIs(field interface{}) interface{} {
	return field
}
