package unknownjson

import (
	"encoding/json"
	"fmt"
	"reflect"
	"strconv"

	"github.com/valyala/fastjson"
)

var (
	marshalerType = reflect.ValueOf(new(json.Marshaler)).Elem().Type()
	arenaPool     fastjson.ArenaPool
)

func encodeValue(a *fastjson.Arena, v reflect.Value) (*fastjson.Value, error) {
	if v.Type().Implements(marshalerType) {
		b, err := v.Interface().(json.Marshaler).MarshalJSON()
		if err != nil {
			return nil, err
		}
		return fastjson.ParseBytes(b)
	}

	switch v.Kind() {
	case reflect.Ptr:
		if v.IsNil() {
			return a.NewNull(), nil
		}
		return encodeValue(a, v.Elem())

	case reflect.String:
		return a.NewString(v.String()), nil

	case reflect.Bool:
		if v.Bool() {
			return a.NewTrue(), nil
		} else {
			return a.NewFalse(), nil
		}

	case reflect.Int:
		return a.NewNumberString(strconv.FormatInt(v.Int(), 10)), nil

	case reflect.Uint:
		return a.NewNumberString(strconv.FormatUint(v.Uint(), 10)), nil

	case reflect.Slice:
		arr := a.NewArray()
		for i := 0; i < v.Len(); i++ {
			elem, err := encodeValue(a, v.Index(i))
			if err != nil {
				return nil, err
			}
			arr.SetArrayItem(i, elem)
		}
		return arr, nil

	case reflect.Map:
		if v.Type().Key() != reflect.TypeOf("") {
			return nil, fmt.Errorf("unsupported map type: %s", v.Type())
		}

		o := a.NewObject()

		it := v.MapRange()
		for it.Next() {
			k := it.Key().String()
			elem, err := encodeValue(a, it.Value())
			if err != nil {
				return nil, err
			}

			o.Set(k, elem)
		}

		return o, nil
	}

	switch {
	case v.Kind() == reflect.Struct:
		return encodeStruct(a, v)

	default:
		return nil, fmt.Errorf("unsupported type %s", v.Type().String())
	}
}

func encodeStruct(a *fastjson.Arena, v reflect.Value) (*fastjson.Value, error) {
	st, err := getTyp(v.Type())
	if err != nil {
		return nil, err
	}

	o := a.NewObject()

	for name, field := range st.fields {
		// TODO(prime@): this is broken for embedded fields that are pointers
		f := v.FieldByIndex(field.index)

		if f.IsZero() && field.omitempty {
			continue
		}

		elem, err := encodeValue(a, f)
		if err != nil {
			return nil, err
		}

		if field.stringFlag {
			if !supportsStringFlag(elem.Type()) {
				return nil, fmt.Errorf(`",string" flag is supported only for primitive types`)
			}

			elem = a.NewStringBytes(elem.MarshalTo(nil))
		}

		o.Set(name, elem)
	}

	if len(st.storeField) != 0 {
		storeValue := v.FieldByIndex(st.storeField).Interface()

		var store map[string]json.RawMessage
		if st.storeFieldType == mapStoreType {
			store = storeValue.(map[string]json.RawMessage)
		} else {
			store = storeValue.(Store).m
		}

		for k, v := range store {
			elem, err := fastjson.ParseBytes(v)
			if err != nil {
				return nil, err
			}

			o.Set(k, elem)
		}
	}

	return o, nil
}

func Marshal(value interface{}) ([]byte, error) {
	a := arenaPool.Get()
	defer arenaPool.Put(a)

	v, err := encodeValue(a, reflect.ValueOf(value))
	if err != nil {
		return nil, err
	}

	return v.MarshalTo(nil), nil
}
