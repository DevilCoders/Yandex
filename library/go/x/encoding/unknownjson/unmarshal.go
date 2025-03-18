// Package unknownjson allows working with json structures without knowing full schema.
//
// Unlike encoding/json, unknownjson does not discard unknown fields.  Instead, unknown map fields are stored in
// inside special field.
//
// NOTE: right now, this package supports limited number of types. Decoding into interface{} is not supported right now.
package unknownjson

import (
	"encoding/json"
	"fmt"
	"reflect"

	"github.com/valyala/fastjson"
)

var (
	unmarshalerType = reflect.ValueOf(new(json.Unmarshaler)).Elem().Type()
)

func decodeValue(js *fastjson.Value, v reflect.Value) error {
	if v.Type().Implements(unmarshalerType) {
		b := js.MarshalTo(nil)
		return v.Interface().(json.Unmarshaler).UnmarshalJSON(b)
	}

	switch v.Kind() {
	case reflect.String:
		s, err := js.StringBytes()
		if err != nil {
			return err
		}
		v.SetString(string(s))
		return nil

	case reflect.Bool:
		b, err := js.Bool()
		if err != nil {
			return err
		}

		v.SetBool(b)
		return nil

	case reflect.Int, reflect.Int32, reflect.Int64:
		i, err := js.Int64()
		if err != nil {
			return err
		}
		if v.OverflowInt(i) {
			return fmt.Errorf("value %d overflows type %s", i, v.Type())
		}

		v.SetInt(i)
		return nil

	case reflect.Uint, reflect.Uint32, reflect.Uint64:
		u, err := js.Uint64()
		if err != nil {
			return err
		}
		if v.OverflowUint(u) {
			return fmt.Errorf("value %d overflows type %s", u, v.Type())
		}

		v.SetUint(u)
		return nil

	case reflect.Slice:
		arr, err := js.Array()
		if err != nil {
			return err
		}

		v.Set(reflect.Zero(v.Type()))
		for i := 0; i < len(arr); i++ {
			elem := reflect.New(v.Type().Elem())
			if err = decodeValue(arr[i], elem); err != nil {
				return err
			}

			v.Set(reflect.Append(v, elem.Elem()))
		}

		return nil

	case reflect.Map:
		if v.Type().Key() != reflect.TypeOf("") {
			return fmt.Errorf("unsupported map type: %s", v.Type())
		}

		m, err := js.Object()
		if err != nil {
			return err
		}

		v.Set(reflect.MakeMap(v.Type()))
		m.Visit(func(key []byte, js *fastjson.Value) {
			if err != nil {
				return
			}

			elem := reflect.New(v.Type().Elem())
			if err = decodeValue(js, elem); err != nil {
				return
			}

			v.SetMapIndex(reflect.ValueOf(string(key)), elem.Elem())
		})
		return err
	}

	switch {
	case v.Kind() == reflect.Struct:
		o, err := js.Object()
		if err != nil {
			return err
		}
		return decodeStruct(o, v)

	case v.Kind() == reflect.Ptr:
		if js.Type() == fastjson.TypeNull {
			return nil
		}

		if v.IsZero() {
			v.Set(reflect.New(v.Type().Elem()))
		}

		return decodeValue(js, v.Elem())

	default:
		return fmt.Errorf("unsupported type %s", v.Type().String())
	}
}

func decodeStruct(js *fastjson.Object, v reflect.Value) error {
	st, err := getTyp(v.Type())
	if err != nil {
		return err
	}

	var storeField reflect.Value
	if len(st.storeField) != 0 {
		storeField = v.FieldByIndex(st.storeField)
	}

	js.Visit(func(key []byte, js *fastjson.Value) {
		if err != nil {
			return
		}

		// TODO(prime@): this is broken when embedded field is a pointer.
		f, ok := st.fields[string(key)]
		if ok {
			fv := v.FieldByIndex(f.index)

			if f.stringFlag {
				if js.Type() != fastjson.TypeString {
					err = fmt.Errorf(`",string" flagged fiels is not a string`)
					return
				}

				b, _ := js.StringBytes()
				js, err = fastjson.ParseBytes(b)
				if err != nil {
					return
				}
			}

			err = decodeValue(js, fv)
		} else if storeField.IsValid() {
			var store map[string]json.RawMessage

			if st.storeFieldType == mapStoreType {
				if storeField.IsNil() {
					store = make(map[string]json.RawMessage)
					storeField.Set(reflect.ValueOf(store))
				} else {
					store = storeField.Interface().(map[string]json.RawMessage)
				}
			} else {
				privateStore := storeField.Interface().(Store)
				if privateStore.m == nil {
					store = make(map[string]json.RawMessage)
					privateStore.m = store
					storeField.Set(reflect.ValueOf(privateStore))
				} else {
					store = privateStore.m
				}
			}

			b := js.MarshalTo(nil)
			store[string(key)] = b
		}
	})
	return err
}

func Unmarshal(data []byte, value interface{}) error {
	js, err := fastjson.ParseBytes(data)
	if err != nil {
		return err
	}

	return decodeValue(js, reflect.ValueOf(value))
}
