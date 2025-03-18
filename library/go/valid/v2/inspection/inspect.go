package inspection

import (
	"reflect"
)

// Inspected represents reflected object with various precomputed data
type Inspected struct {
	// Value is a raw reflect value
	Value reflect.Value
	// Type is a type of Value
	Type reflect.Type
	// TypeName is a string representation of Type (e.g. *MyType)
	TypeName string
	// Interface representation of Value
	Interface interface{}

	// Indirect is an underlying value of Value field (e.g. not pointer or interface)
	Indirect reflect.Value
	// IndirectType is a type of Indirect
	IndirectType reflect.Type
	// IndirectTypeName is a string representation of IndirectType (e.g. MyType)
	IndirectTypeName string

	// Fields contains all immediate fields of Indirect if its kind is reflect.Struct
	Fields []*StructField

	// IsZero indicates current state of Value value
	IsZero bool
	// Validate is a pointer to Validate function if value implements Validator interface
	Validate func() error
}

type StructField struct {
	Field reflect.StructField
	Value reflect.Value
}

// IsValid checks if underlying value is valid
func (i *Inspected) IsValid() bool {
	return i.Value.IsValid()
}

// inspect can be used if value has been already reflected before.
// reflect.Indirect param must be actual dereferenced type, not pointer or interface
func inspect(orig, indir reflect.Value, indirType reflect.Type) Inspected {
	valueType := orig.Type()
	valueTypeName := valueType.Name()
	if valueTypeName == "" {
		valueTypeName = orig.Kind().String()
	}

	indirTypeName := indirType.Name()
	if indirTypeName == "" {
		indirTypeName = indir.Kind().String()
	}

	r := Inspected{
		Type:     valueType,
		TypeName: valueTypeName,

		IndirectType:     indirType,
		IndirectTypeName: indirTypeName,
	}

	// collect struct fields data
	if indir.Kind() == reflect.Struct {
		numField := indir.NumField()
		r.Fields = make([]*StructField, numField)
		for i := 0; i < numField; i++ {
			r.Fields[i] = &StructField{
				Field: indirType.Field(i),
			}
		}
	}

	return r
}

// setValue sets value and additional value based data to Inspected type struct
func (i *Inspected) setValue(orig, indir reflect.Value) {
	i.Value = orig
	i.Interface = orig.Interface()
	i.Indirect = indir

	i.IsZero = isZero(i)

	if validator, ok := i.Interface.(interface{ Validate() error }); ok {
		i.Validate = validator.Validate
	}

	// gather runtime data for struct fields
	for idx := range i.Fields {
		i.Fields[idx].Value = indir.Field(idx)
	}
}

// indirectValue returns indirect reflected value of given interface.
// If underlying value is not valid - source value will be returned
func indirectValue(target interface{}) reflect.Value {
	var v reflect.Value
	if vt, ok := target.(reflect.Value); ok {
		v = vt
	} else {
		v = reflect.ValueOf(target)
	}

	// extract underlying value of pointer or interface
	for v.Kind() == reflect.Ptr || v.Kind() == reflect.Interface {
		ve := v.Elem()
		if !ve.IsValid() {
			// return last valid indirect
			return v
		}
		v = ve
	}
	return v
}

// isZero checks if Inspected value is in its zero state
func isZero(r *Inspected) bool {
	if r.Interface != nil {
		if i, ok := r.Interface.(interface{ IsZero() bool }); ok {
			return i.IsZero()
		}
	}
	return r.Value.IsZero() ||
		r.Indirect.IsZero()
}
