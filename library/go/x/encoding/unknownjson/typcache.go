package unknownjson

import (
	"encoding/json"
	"fmt"
	"reflect"
	"strings"
	"sync"

	"github.com/valyala/fastjson"
)

type structField struct {
	omitempty  bool
	stringFlag bool
	index      []int
}

type structTyp struct {
	fields map[string]structField

	storeField     []int
	storeFieldType reflect.Type
}

func isMarked(unknownTag string) bool {
	flags := strings.Split(unknownTag, ",")[1:]
	for _, f := range flags {
		if f == "store" {
			return true
		}
	}

	return false
}

func supportsStringFlag(typ fastjson.Type) bool {
	switch typ {
	case fastjson.TypeTrue, fastjson.TypeFalse, fastjson.TypeNumber, fastjson.TypeString:
		return true
	default:
		return false
	}
}

func parseJSONTag(tag, fieldName string) (name string, omitempty, stringFlag, skip bool) {
	flags := strings.Split(tag, ",")
	if flags[0] == "-" {
		skip = true
		return
	} else if flags[0] != "" {
		name = flags[0]
	} else {
		name = fieldName
	}

	for _, f := range flags[1:] {
		if f == "omitempty" {
			omitempty = true
		} else if f == "string" {
			stringFlag = true
		}
	}

	return
}

var (
	mapStoreType     = reflect.TypeOf(map[string]json.RawMessage(nil))
	privateStoreType = reflect.TypeOf(Store{})
)

func makeTyp(t reflect.Type) (*structTyp, error) {
	st := structTyp{
		fields: map[string]structField{},
	}

	for i := 0; i < t.NumField(); i++ {
		field := t.Field(i)

		// Skip private fields.
		if field.PkgPath != "" {
			continue
		}

		unknownTag, ok := field.Tag.Lookup("unknown")
		if ok && isMarked(unknownTag) {
			if field.Type != mapStoreType && field.Type != privateStoreType {
				return nil, fmt.Errorf("store field must have type %s, got %s", privateStoreType, field.Type)
			}

			st.storeField = field.Index
			st.storeFieldType = field.Type
			continue
		}

		jsonTag := field.Tag.Get("json")
		name, omitempty, stringFlag, skip := parseJSONTag(jsonTag, field.Name)
		if skip {
			continue
		}

		st.fields[name] = structField{
			index:      field.Index,
			omitempty:  omitempty,
			stringFlag: stringFlag,
		}
	}

	return &st, nil
}

var typCache sync.Map

func getTyp(t reflect.Type) (*structTyp, error) {
	v, ok := typCache.Load(t)
	if ok {
		return v.(*structTyp), nil
	}

	typ, err := makeTyp(t)
	if err != nil {
		return nil, err
	}
	typCache.Store(t, typ)
	return typ, nil
}
