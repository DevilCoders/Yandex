package valid

import (
	"fmt"
	"reflect"
	"strconv"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	validationTag     = "valid"
	validationTagOmit = "omitempty"
)

// Struct performs struct reflection based validation using
// validation context, struct tags and Validator interface.
// This function always returns valid.Errors type.
// Example:
//     vctx := valid.NewValidationCtx()
//     vctx.Add("uuid4", valid.WrapStringValidator(valid.UUIDv4))
//     errs := valid.Struct(vctx, struct{
//         ID string `valid:"uuid4"`
//	   }{
//         ID: userInput.ID,
//	   })
//     // errs might output something like `[]errors{valid.ErrInvalidCharsSequence}`
func Struct(ctx *ValidationCtx, s interface{}) error {
	if s == nil {
		return nil
	}

	if ctx == nil {
		panic("valid: nil validation context given")
	}

	val := unrefValue(reflect.ValueOf(s))

	// we only accept structs
	if val.Kind() != reflect.Struct {
		return Errors{ErrStructExpected}
	}

	verrs := validateField(ctx, val, "", "", "")
	if len(verrs) > 0 {
		return verrs
	}
	return nil
}

func validateField(ctx *ValidationCtx, val reflect.Value, name, path, tag string) Errors {
	var errs Errors

	// skip marked field | check field zero value for "omitempty" tags
	if tag == "-" || (strings.Contains(tag, validationTagOmit) && zeroValue(val)) {
		return nil
	}

	// call Validate if struct implements Validator interface
	if vv, ok := val.Interface().(Validator); ok {
		proceed, verr := vv.Validate(ctx)
		if verr != nil {
			// convert errors to FieldErrors
			for _, e := range unfoldErrors(verr) {
				var ferr FieldError
				if xerrors.As(e, &ferr) {
					errs = append(errs, FieldError{
						field: ferr.field,
						path:  path + "." + name + ferr.Path(),
						err:   ferr.err,
					})
				} else {
					errs = append(errs, FieldError{
						field: name,
						path:  path,
						err:   e,
					})
				}

			}
		}
		if !proceed {
			return errs
		}
	}

	// run field validation using struct tag
	if tag != "" && len(ctx.validators) > 0 {
		var validatorName, param string

		for _, tv := range strings.Split(tag, ",") {
			parts := strings.SplitN(tv, "=", 2)
			validatorName = strings.TrimSpace(parts[0])
			if len(parts) == 2 {
				param = parts[1]
			}

			// do not trait `omitempty` as validator name
			if validatorName == validationTagOmit {
				continue
			}

			// load validation function
			vf, ok := ctx.Get(validatorName)
			if !ok {
				panic("valid: unknown validator '" + validatorName + "'")
			}

			// call struct tag validation func
			if verr := vf(val, param); verr != nil {
				// convert errors to FieldErrors
				for _, e := range unfoldErrors(verr) {
					errs = append(errs, FieldError{
						field: name,
						path:  path,
						err:   e,
					})
				}
			}

		}
	}

	// cleanup value before recursion
	val = unrefValue(val)

	// calculate "path" to current field
	curPath := name
	if path != "" {
		curPath = path + "." + curPath
	}

	// validate supported field types
	switch val.Kind() {

	case reflect.Struct:
		for i := 0; i < val.NumField(); i++ {
			typeField := val.Type().Field(i)
			if typeField.PkgPath != "" {
				continue // skip private field
			}

			name := typeField.Name
			verrs := validateField(ctx, val.Field(i), name, curPath, typeField.Tag.Get(validationTag))
			if verrs != nil {
				errs = append(errs, verrs...)
			}
		}

	case reflect.Slice:
		for i := 0; i < val.Len(); i++ { // skip marked field
			name := strconv.FormatInt(int64(i), 10)
			verrs := validateField(ctx, val.Index(i), name, curPath, "") // do not propagate tag on slice values
			if verrs != nil {
				errs = append(errs, verrs...)
			}
		}

	case reflect.Map:
		mapIter := val.MapRange()
		for mapIter.Next() {
			name := fmt.Sprintf("%v", mapIter.Key().Interface())
			verrs := validateField(ctx, mapIter.Value(), name, curPath, "") // do not propagate tag on map values
			if verrs != nil {
				errs = append(errs, verrs...)
			}
		}
	}

	return errs
}

func unrefValue(val reflect.Value) reflect.Value {
	for val.Kind() == reflect.Ptr || val.Kind() == reflect.Interface {
		val = val.Elem()
	}
	return val
}

func unfoldErrors(err error) Errors {
	switch et := err.(type) {
	case Errors:
		return et
	default:
		return Errors{err}
	}
}

func zeroValue(value reflect.Value) bool {
	switch value.Kind() {
	case reflect.Slice, reflect.Map:
		return value.IsNil()
	}

	valInterface := value.Interface()

	if zv, ok := valInterface.(interface{ IsValid() bool }); ok {
		return !zv.IsValid()
	}
	if zv, ok := valInterface.(interface{ IsZero() bool }); ok {
		return zv.IsZero()
	}

	return valInterface == reflect.Zero(value.Type()).Interface()
}
