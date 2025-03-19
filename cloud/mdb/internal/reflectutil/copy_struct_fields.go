package reflectutil

import (
	"fmt"
	"reflect"
	"strings"
)

type CopyStructFieldsConfig struct {
	IsValidField        func(string, interface{}) bool
	Convert             func(interface{}) interface{}
	PanicOnMissingField func(missingInSrc bool, missingInDst bool) bool
	CaseInsensitive     bool
}

// CopyStructFields copies "valid" fields from src struct and overwrites fields with matched names in dst struct.
// If CaseInsensitive == true then fields match by name in lowercase.
// Non-exported fields are ignored.
// "valid" fields are fields where IsValidField(field) == true
// Convert(field) converts matched fields from src type of the field to dst type of the field
// It panics if:
// - src's and dst's Kinds are not pointer
// - src's and dst's Kinds of element are not struct
// - Convert returns wrong type value for a field
// - src or dst contains distinct fields and PanicOnMissingField returns true for the corresponding case
func CopyStructFields(src, dst interface{}, config CopyStructFieldsConfig) {
	// Set some reasonable defaults in order to avoid panics
	if config.IsValidField == nil {
		config.IsValidField = AllFieldsAreValid
	}
	if config.Convert == nil {
		config.Convert = CopyFieldsAsIs
	}
	if config.PanicOnMissingField == nil {
		config.PanicOnMissingField = IgnoreMissingFields
	}

	if reflect.ValueOf(src).Kind() != reflect.Ptr {
		panic("src argument is not a pointer")
	}
	if reflect.ValueOf(dst).Kind() != reflect.Ptr {
		panic("dst argument is not a pointer")
	}
	valueOfSrc := reflect.ValueOf(src).Elem()
	valueOfDst := reflect.ValueOf(dst).Elem()
	if valueOfSrc.Kind() != reflect.Struct {
		panic(fmt.Sprintf("provided src value is not a struct: %+v", valueOfSrc))
	}
	if valueOfDst.Kind() != reflect.Struct {
		panic(fmt.Sprintf("provided dst value is not a struct: %+v", valueOfDst))
	}
	validFields := make(map[string]reflect.Value)
	for i := 0; i < valueOfSrc.NumField(); i++ {
		fieldInfo := valueOfSrc.Type().Field(i)
		value := valueOfSrc.Field(i)
		if !value.CanInterface() {
			continue
		}
		if config.IsValidField(fieldInfo.Name, value.Interface()) {
			fieldName := fieldInfo.Name
			if config.CaseInsensitive {
				fieldName = strings.ToLower(fieldName)
			}
			validFields[fieldName] = value
		}
	}
	for i := 0; i < valueOfDst.NumField(); i++ {
		if !valueOfDst.Field(i).CanInterface() {
			continue
		}
		fieldInfo := valueOfDst.Type().Field(i)
		fieldName := fieldInfo.Name
		if config.CaseInsensitive {
			fieldName = strings.ToLower(fieldName)
		}
		srcValue, ok := validFields[fieldName]
		if !ok {
			if config.PanicOnMissingField(true, false) {
				panic(fmt.Sprintf("field %q is present within destination struct of type %s"+
					" but missing from source struct of type %s", fieldInfo.Name, valueOfDst.Type().Name(),
					valueOfSrc.Type().Name()))
			}
			continue
		}
		if !srcValue.CanInterface() {
			continue
		}
		delete(validFields, fieldName)
		valueOfConverted := reflect.ValueOf(config.Convert(srcValue.Interface()))
		valueOfDst.Field(i).Set(valueOfConverted)
	}
	for name := range validFields {
		if config.PanicOnMissingField(false, true) {
			panic(fmt.Sprintf("field %q is present within source struct of type %s"+
				" but missing from destination struct of type %s", name, valueOfSrc.Type().Name(),
				valueOfDst.Type().Name()))
		}
	}
}

func CopyStructFieldsStrict(src, dst interface{}) {
	panicOnMissingField := func(missingInSrc bool, missingInDst bool) bool {
		return missingInSrc || missingInDst
	}
	CopyStructFields(src, dst, CopyStructFieldsConfig{
		IsValidField:        AllFieldsAreValid,
		Convert:             CopyFieldsAsIs,
		PanicOnMissingField: panicOnMissingField,
		CaseInsensitive:     false,
	})
}
