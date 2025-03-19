package reflectutil

import (
	"fmt"
	"reflect"
)

// MergeStructs copy "valid" fields from add struct and overwrite the same fields in base struct.
// Non-exported fields are ignored.
// "valid" fields is fields where isValidField(field) == true
// It panics if:
// - src's and dst's Kinds are not pointer
// - src's and dst's Kinds of element are not struct
// - src's and dst's types are not the same
func MergeStructs(base, add interface{}, isValidField func(string, interface{}) bool) {
	valueOfBase := reflect.ValueOf(base)
	valueOfAdd := reflect.ValueOf(add)
	if valueOfBase.Kind() != reflect.Ptr {
		panic("base argument is not a pointer")
	}
	if valueOfAdd.Kind() != reflect.Ptr {
		panic("add argument is not a pointer")
	}
	valueOfBase = valueOfBase.Elem()
	valueOfAdd = valueOfAdd.Elem()
	if valueOfBase.Kind() != reflect.Struct {
		panic(fmt.Sprintf("provided base value is not a struct: %+v", valueOfBase))
	}
	if valueOfAdd.Kind() != reflect.Struct {
		panic(fmt.Sprintf("provided add value is not a struct: %+v", valueOfAdd))
	}
	typeOfBase := valueOfBase.Type()
	typeOfAdd := valueOfAdd.Type()
	if typeOfBase != typeOfAdd {
		panic(fmt.Sprintf("base and add arguments should have the same type, actual base type: %+v, and add type: %+v",
			valueOfBase.Type(), valueOfAdd.Type()))
	}
	for i := 0; i < valueOfBase.NumField(); i++ {
		if !valueOfAdd.Field(i).CanInterface() {
			continue
		}
		if isValidField(typeOfAdd.Field(i).Name, valueOfAdd.Field(i).Interface()) {
			valueOfBase.Field(i).Set(valueOfAdd.Field(i))
		}
	}
}
