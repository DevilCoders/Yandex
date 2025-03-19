package optional

import (
	"fmt"
	"reflect"
)

// ApplyUpdate updates base object with values from update-object and returns true if changes were made.
//
// This function is useful when implementing Update method of GRPC service.
// In this case we usually have some model class Model and corresponding update-class ModelUpdate
// which has same fields as Model but their types are replaced with corresponding optional versions.
// We want to apply changes from instance of ModelUpdate (say model_update) to the instance of Model
// (say model), check if model is actually changed, and process updated object somehow (eg. save it
// to the database).
// Example:
//
//  type Model struct {
//    String string
//    Int64  int64
//  }
//
//  type ModelUpdate struct {
//    String optional.String
//    Int64  optional.Int64
//  }
//
//  model := Model{String: "old-value", Int64: 0}
//  update := ModelUpdate{String: optional.NewString("new-value"), Int64: optional.NewInt64(1)}
//  if ApplyUpdate(&model, &update) {
//    // eg. save updated value of model to database
//  }
func ApplyUpdate(base, updateWithOptionals interface{}) bool {
	if reflect.ValueOf(base).Kind() != reflect.Ptr {
		panic("base argument is not a pointer")
	}
	if reflect.ValueOf(updateWithOptionals).Kind() != reflect.Ptr {
		panic("update argument is not a pointer")
	}
	valueOfBase := reflect.ValueOf(base).Elem()
	valueOfUpdate := reflect.ValueOf(updateWithOptionals).Elem()
	if valueOfBase.Kind() != reflect.Struct {
		panic(fmt.Sprintf("provided base value is not a struct: %+v", valueOfBase))
	}
	if valueOfUpdate.Kind() != reflect.Struct {
		panic(fmt.Sprintf("provided update value is not a struct: %+v", valueOfUpdate))
	}

	hasChanges := false
	for i := 0; i < valueOfUpdate.NumField(); i++ {
		fieldInfo := valueOfUpdate.Type().Field(i)
		isValid := valueOfUpdate.Field(i).FieldByName("Valid").Interface().(bool)
		if isValid {
			baseField := valueOfBase.FieldByName(fieldInfo.Name)
			if !baseField.IsValid() {
				panic(fmt.Sprintf("field %q is present within update type %s but not found within base type %s",
					fieldInfo.Name, valueOfUpdate.Type().Name(), valueOfBase.Type().Name()))
			}
			updatedValue := valueOfUpdate.Field(i).Field(0)
			if !reflect.DeepEqual(updatedValue.Interface(), baseField.Interface()) {
				hasChanges = true
				baseField.Set(updatedValue)
			}
		}
	}
	return hasChanges
}
