package secret

import (
	"encoding/json"
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

var maskedStringReflected = reflect.ValueOf(maskedString)

var DefaultSecretKeys = []string{
	"access_token",
	"accesstoken",
	"api_key",
	"apikey",
	"authorization",
	"passwd",
	"password",
	"secret",
}

func MaskKeysInJSON(input []byte, keys []string) ([]byte, error) {
	var v interface{}
	if err := json.Unmarshal(input, &v); err != nil {
		return input, xerrors.Errorf("can't mask keys in json: json unmarshal: %w", err)
	}

	maskKeysInObject(v, keys)

	res, err := json.MarshalIndent(v, "", "\t")
	if err != nil {
		return input, xerrors.Errorf("can't mask keys in json: json marshal: %w", err)
	}

	return res, nil
}

func maskKeysInObject(input interface{}, keys []string) {
	for i := range keys {
		keys[i] = strings.ToLower(keys[i])
	}

	maskKeys(reflect.ValueOf(input), keys, false)
}

func maskKeys(v reflect.Value, keys []string, maskEverything bool) {
	switch v.Kind() {
	case reflect.Map:
		maskMapKeys(v, keys, maskEverything)
	case reflect.Slice:
		maskSliceKeys(v, keys, maskEverything)
		// We handle maps and slices here, 'pure' values are handled inside container handlers
	}
}

func maskMapKeys(mapValue reflect.Value, keys []string, maskEverything bool) {
	iter := mapValue.MapRange()
	for iter.Next() {
		key := iter.Key()
		value := iter.Value()
		valueKind := value.Kind()
		if valueKind == reflect.Interface {
			value = value.Elem()
		}

		maskSubtree := maskEverything
		if !maskSubtree && // Do we need to recheck?
			key.Type().Kind() == reflect.String && // Is it a string?
			slices.ContainsString(keys, strings.ToLower(key.String())) { // Does it contain what we need?
			maskSubtree = true
		}

		if maskSubtree {
			switch value.Kind() {
			case reflect.Struct:
			case reflect.Map:
			case reflect.Array:
			case reflect.Slice:
			case reflect.String:
				mapValue.SetMapIndex(key, maskedStringReflected)
			case reflect.Invalid:
			// This means its null, do nothing
			default:
				mapValue.SetMapIndex(key, reflect.Zero(value.Type()))
			}
		}

		maskKeys(value, keys, maskSubtree)
	}
}

func maskSliceKeys(sliceValue reflect.Value, keys []string, maskEverything bool) {
	for i := 0; i < sliceValue.Len(); i++ {
		value := sliceValue.Index(i)
		if value.Kind() == reflect.Interface {
			value = value.Elem()
		}

		if maskEverything {
			switch value.Kind() {
			case reflect.Struct:
			case reflect.Map:
			case reflect.Array:
			case reflect.Slice:
			case reflect.String:
				sliceValue.Index(i).Set(maskedStringReflected)
			case reflect.Invalid:
			// This means its null, do nothing
			default:
				sliceValue.Index(i).Set(reflect.Zero(value.Type()))
			}
		}

		maskKeys(value, keys, maskEverything)
	}
}
