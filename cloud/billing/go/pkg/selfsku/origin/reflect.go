package origin

import (
	"fmt"
	"reflect"
	"strings"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
	"a.yandex-team.ru/library/go/valid"
)

var errorInterface = reflect.TypeOf((*error)(nil)).Elem()

func wrapValidator(f interface{}) valid.ValidatorFunc {
	fv := reflect.ValueOf(f)
	t := fv.Type()
	if t.Kind() != reflect.Func {
		panic(fmt.Sprintf("try to wrap not func: %T", f))
	}
	if t.NumOut() != 1 {
		panic(fmt.Sprintf("try to wrap func with incorrect out count: %T", f))
	}
	if !t.Out(0).Implements(errorInterface) {
		panic(fmt.Sprintf("try to wrap func with incorrect out type: %T", f))
	}

	switch t.NumIn() {
	case 1:
		inType := t.In(0)
		return func(value reflect.Value, _ string) error {
			if value.Type().AssignableTo(inType) {
				par := reflect.New(inType).Elem()
				par.Set(value)
				if errInt := fv.Call([]reflect.Value{par})[0].Interface(); errInt != nil {
					return errInt.(error)
				}
				return nil
			}
			return fmt.Errorf("value has invalid type %s", value.Type().Name())
		}
	case 2:
		inType := t.In(0)
		return func(value reflect.Value, param string) error {
			if value.Type().AssignableTo(inType) {
				par := reflect.New(inType).Elem()
				par.Set(value)
				if errInt := fv.Call([]reflect.Value{par, reflect.ValueOf(param)})[0].Interface(); errInt != nil {
					return errInt.(error)
				}
				return nil
			}
			return fmt.Errorf("value has invalid type %s", value.Type().Name())
		}
	default:
		panic(fmt.Sprintf("try to wrap func with incorrect in count: %T", f))
	}
}

func eachApply(v reflect.Value, validator string) error {
	validateParam := ""
	params := strings.SplitN(validator, "=", 2)
	validator = params[0]
	if len(params) > 1 {
		validateParam = params[1]
	}

	f, ok := vctx.Get(validator)
	if !ok {
		panic("valid: unknown validator '" + validator + "'")
	}
	errs := tools.ValidErrors{}

	switch v.Kind() {
	case reflect.Array, reflect.Slice:
		l := v.Len()
		for i := 0; i < l; i++ {
			err := f(v.Index(i), validateParam)
			errs.CollectWrapped(fmt.Sprintf("item %d: %%w", i), err)
		}
	case reflect.Map:
		for _, k := range v.MapKeys() {
			err := f(v.MapIndex(k), validateParam)
			errs.CollectWrapped(fmt.Sprintf("key %v: %%w", k), err)
		}
	default:
		return fmt.Errorf("unknown type for each check: %s", v.Type().String())
	}

	return errs.Expose()
}

func eachKey(v reflect.Value, validator string) error {
	validateParam := ""
	params := strings.SplitN(validator, "=", 2)
	validator = params[0]
	if len(params) > 1 {
		validateParam = params[1]
	}

	f, ok := vctx.Get(validator)
	if !ok {
		panic("valid: unknown validator '" + validator + "'")
	}
	errs := tools.ValidErrors{}

	switch v.Kind() {
	case reflect.Map:
		for _, k := range v.MapKeys() {
			err := f(k, validateParam)
			errs.CollectWrapped(fmt.Sprintf("key %v: %%w", k), err)
		}
	default:
		return fmt.Errorf("unknown type for each check: %s", v.Type().String())
	}

	return errs.Expose()
}
