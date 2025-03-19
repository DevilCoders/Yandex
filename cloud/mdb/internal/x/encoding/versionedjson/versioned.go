package versionedjson

import (
	"encoding/json"
	"reflect"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type (
	ParserFunc    func(version interface{}, data *fastjson.Value, dst interface{}, parsers Parsers, subparser SubParserFunc) (interface{}, error)
	SubParserFunc func(d *fastjson.Value, dst interface{}, parsers Parsers) (interface{}, error)
	Parsers       map[string]ParserFunc
)

// Parse JSON with versioned attributes. Check tests for examples.
//
// ATTENTION: Does NOT support versioning for attributes nested into types with custom JSON Unmarshaler (on any level
// within those attributes).
func Parse(d []byte, dst interface{}, parsers Parsers) error {
	typ := reflect.TypeOf(dst)
	if typ.Kind() != reflect.Ptr && typ.Kind() != reflect.Interface {
		return xerrors.Errorf("destination must be either pointer or an interface but it is %s", typ.Kind())
	}

	for typ.Kind() == reflect.Ptr || typ.Kind() == reflect.Interface {
		typ = typ.Elem()
	}

	switch kind := typ.Kind(); {
	case kind == reflect.Struct:
	case kind == reflect.Array:
		fallthrough
	case kind == reflect.Slice:
		// TODO: handle versioned arrays/slices
		return json.Unmarshal(d, dst)
	default:
		return json.Unmarshal(d, dst)
	}

	root, err := fastjson.ParseBytes(d)
	if err != nil {
		return xerrors.Errorf("parsing versioned json: %w", err)
	}

	o, err := root.Object()
	if err != nil {
		return xerrors.Errorf("invalid field type: %w", err)
	}

	// If destination has custom unmarshaler, use it. This is useful for attributes that have no versioning whatsoever.
	unmarshaler, ok := dst.(json.Unmarshaler)
	if ok {
		if err := unmarshaler.UnmarshalJSON(d); err != nil {
			return xerrors.Errorf("custom unmarshaler: %w", err)
		}

		return nil
	}

	// Iterate over destination fields parsing version tags and handling non-versioned fields
	versionsByKey := map[string]*versionedFieldsPair{}
	for i := 0; i < typ.NumField(); i++ {
		field := typ.Field(i)

		// Skip private fields.
		if field.PkgPath != "" {
			continue
		}

		vf, err := parseVersionField(field)
		if err != nil {
			return err
		}

		if vf != nil {
			// Versioned field, save it
			pair, ok := versionsByKey[vf.Key]
			if !ok {
				pair = &versionedFieldsPair{Key: vf.Key}
			}

			if vf.Version {
				if pair.VersionField != nil {
					return xerrors.Errorf("double version tag for key %q", vf.Key)
				}
				pair.VersionField = vf
			} else {
				if pair.VersionedField != nil {
					return xerrors.Errorf("double versioned tag for key %q", vf.Key)
				}
				pair.VersionedField = vf
			}

			if !ok {
				versionsByKey[vf.Key] = pair
			}

			if !vf.Version {
				continue
			}
		}

		// Handle non-versioned field
		nonVersionedField := o.Get(jsonFieldName(field))
		if nonVersionedField == nil {
			// Value might not exist
			continue
		}

		buf := nonVersionedField.MarshalTo(nil)
		dstValue := reflect.ValueOf(dst)
		dstValue = dstValue.Elem().Field(i)
		if dstValue.Kind() == reflect.Ptr {
			if dstValue.IsNil() {
				// If a struct embeds a pointer to uninitialized value
				// we should allocate it
				dstValue.Set(reflect.New(dstValue.Type().Elem()))
			}
		} else {
			dstValue = dstValue.Addr()
		}
		dstWritable := dstValue.Interface()

		if err = Parse(buf, dstWritable, parsers); err != nil {
			return xerrors.Errorf("non-versioned field %q: %w", field.Name, err)
		}
	}

	// Iterate over versioned keys
	for key, pair := range versionsByKey {
		// Validate keys
		if pair.VersionField == nil {
			return xerrors.Errorf("key %q missing version field", key)
		}
		if pair.VersionedField == nil {
			return xerrors.Errorf("key %q missing versioned field", key)
		}

		versionReflectValue := reflect.ValueOf(dst).Elem().FieldByName(pair.VersionField.FieldName)
		versionValue := o.Get(pair.VersionField.JSONFieldName)
		versionedValue := o.Get(pair.VersionedField.JSONFieldName)
		if !pair.VersionedField.AllowMissing && versionValue != nil && versionedValue == nil {
			return xerrors.Errorf("has version field %q but not versioned field %q", pair.VersionField.FieldName, pair.VersionedField.FieldName)
		}
		if !pair.VersionField.AllowMissing && versionValue == nil && versionedValue != nil {
			return xerrors.Errorf("has versioned field %q but not version field %q", pair.VersionedField.FieldName, pair.VersionField.FieldName)
		}

		parser, ok := parsers[key]
		if !ok {
			return xerrors.Errorf("no parser for key %q", key)
		}

		dstValue := reflect.ValueOf(dst)
		dstValue = dstValue.Elem().FieldByName(pair.VersionedField.FieldName)
		dstWritable := dstValue.Addr().Interface()
		parsed, err := parser(versionReflectValue.Interface(), versionedValue, dstWritable, parsers, subParser)
		if err != nil {
			return xerrors.Errorf("versioned parser for key %q: %w", key, err)
		}

		// Assign parsed value to destination
		reflect.Indirect(reflect.ValueOf(dstWritable)).Set(reflect.ValueOf(parsed))
	}

	return nil
}

// subParser for values initially parsed by Parse function.
//
// Used inside versioned value parsers for recursive parsing.
func subParser(d *fastjson.Value, dst interface{}, parsers Parsers) (interface{}, error) {
	buf := d.MarshalTo(nil)
	if err := Parse(buf, dst, parsers); err != nil {
		return nil, err
	}

	return dst, nil
}
