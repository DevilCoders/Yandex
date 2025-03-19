package structs

// NOTE: Temporary stub to generate string to string map to be used in custom License Rules,
// NOTE: should be refactored into static set of strict rules after bisness logic will be stabilized.

import (
	"fmt"
	"reflect"
	"regexp"
	"strconv"
	"strings"

	"github.com/iancoleman/strcase"

	"a.yandex-team.ru/cloud/marketplace/pkg/errtools"
)

const snakeCaseMarker = "_snake"

type Mapping map[string]interface{}

var structFieldMarker = struct{}{}

// NewMapping construct a map of struct path to string value appropriate to
// represent plain python dictionary obtained from external resources.
func NewMapping(s interface{}) (Mapping, error) {
	m := make(Mapping)

	if err := m.recProcessStruct("", s); err != nil {
		return nil, err
	}

	return m, nil
}

func NewStringsMapping(tag string, keys ...string) (out Mapping) {
	out = make(Mapping)

	for _, key := range keys {
		out[key] = tag
	}

	return out
}

func (m Mapping) ContainsPath(path string) bool {
	return m.containsPath(path) || m.containsPath(path+snakeCaseMarker)
}

func (m Mapping) containsPath(path string) bool {
	_, ok := m[path]
	return ok
}

func (m Mapping) ContainsPathWithValue(path string) bool {
	return m.containsPathWithValue(path) || m.containsPathWithValue(path+snakeCaseMarker)
}

func (m Mapping) containsPathWithValue(path string) bool {
	value, ok := m[path]
	if !ok {
		return false
	}

	switch typedValue := value.(type) {
	case int:
		return typedValue != 0
	case int64:
		return typedValue != 0
	case float64: // useless, used only for comprehension.
		return typedValue != 0.
	case string:
		return typedValue != ""
	default: // slice types not supported
		return false
	}
}

func (m Mapping) InRange(path string, low, hi float64) bool {
	return m.inRange(path, low, hi) || m.inRange(path+snakeCaseMarker, low, hi)
}

func (m Mapping) inRange(path string, low, hi float64) bool {
	value, ok := m[path]
	if !ok {
		return false
	}

	switch typedValue := value.(type) {
	case int:
		asFloat := float64(typedValue)
		return low <= asFloat && asFloat < hi
	case int64:
		asFloat := float64(typedValue)
		return low <= asFloat && asFloat < hi
	case float64:
		return low <= typedValue && typedValue < hi
	case string:
		asFloat, err := strconv.ParseFloat(typedValue, 64)
		if err != nil {
			return false
		}

		return low <= asFloat && asFloat < hi
	default: // slice types not supported
	}

	return false
}

func (m Mapping) Match(path, pattern string) bool {
	return m.match(path, pattern) || m.match(path+snakeCaseMarker, pattern)
}

func (m Mapping) match(path, pattern string) bool {
	storedValue, ok := m[path]
	if !ok {
		return false
	}

	var strValue string
	switch typedValue := storedValue.(type) {
	case int64:
		strValue = strconv.FormatInt(typedValue, 10)
	case float64: // useless, used only for comprehension.
		strValue = strconv.FormatFloat(typedValue, 'E', -1, 64)
	case string:
		strValue = typedValue
	default: // slice, struct types not supported
		return false
	}

	ok, err := regexp.MatchString(pattern, strValue)
	if err != nil {
		return false
	}

	return ok
}

func (m Mapping) Value(path string) interface{} {
	v := m.value(path)
	if v == nil {
		return m.value(path + snakeCaseMarker)
	}

	return v
}

func (m Mapping) value(path string) interface{} {
	storedValue, ok := m[path]
	if !ok {
		return nil
	}

	return storedValue
}

func (m Mapping) Equals(path string, v interface{}) bool {
	return m.equals(path, v) || m.equals(path+snakeCaseMarker, v)
}

func (m Mapping) equals(path string, v interface{}) bool {
	storedValue, ok := m[path]
	if !ok {
		return false
	}

	switch typedValue := storedValue.(type) {
	case int64:
		if i, ok := v.(int64); ok {
			return typedValue == i
		}
	case float64:
		if f, ok := v.(float64); ok {
			return typedValue == f
		}
	case string:
		if s, ok := v.(string); ok {
			return typedValue == s
		}
	default:
		return reflect.DeepEqual(typedValue, v)
	}

	return false
}

func (m Mapping) markStructTag(tagPath string) {
	m[tagPath] = structFieldMarker
}

func (m Mapping) recProcessStruct(prefix string, s interface{}) error {
	rt, rv := reflect.TypeOf(s), reflect.ValueOf(s)

	if rt.Kind() != reflect.Ptr && rt.Kind() != reflect.Struct {
		return fmt.Errorf("unexpected struct type %T kind of %s", s, rt.Kind().String())
	}

	if rt.Kind() == reflect.Ptr {
		if rv.IsNil() {
			return nil
		}

		rv = reflect.Indirect(rv)
		rt = rv.Type()
	}

	for i := 0; i < rt.NumField(); i++ {
		field, fieldValue := rt.Field(i), rv.Field(i)

		jsonTag := field.Tag.Get("json")
		if jsonTag == "" {
			continue
		}

		tagPath := strings.Split(jsonTag, ",")[0] // use split to ignore tag "options" like omitempty, etc.
		tagPath = prefix + strcase.ToSnake(tagPath)

		if strings.HasSuffix(field.Name, "Snake") {
			// NOTE: same Balance fields for various Billing Accounts could be either in snake case either
			// in camelCase depanding in billing account id.
			tagPath += snakeCaseMarker
		}

		fieldType := field.Type
		if fieldType.Kind() == reflect.Ptr {
			if fieldValue.IsNil() {
				continue
			}

			fieldValue = reflect.Indirect(fieldValue)
			fieldType = fieldValue.Type()
		}

		if fieldType.Kind() == reflect.Struct {
			value := fieldValue.Interface()

			if anyStringer, ok := value.(fmt.Stringer); ok {
				if err := m.setValue(tagPath, reflect.String, reflect.ValueOf(anyStringer.String())); err != nil {
					return errtools.WithCurCaller(err)
				}

				continue
			}

			m.markStructTag(tagPath)
			if err := m.recProcessStruct(tagPath+".", value); err != nil {
				return err
			}

			continue
		}

		if err := m.setValue(tagPath, fieldType.Kind(), fieldValue); err != nil {
			return errtools.WithCurCaller(err)
		}
	}

	return nil
}

func (m Mapping) setValue(tag string, kind reflect.Kind, v reflect.Value) error {
	switch kind {
	case reflect.Bool:
		m[tag] = v.Bool()
	case reflect.Float64:
		m[tag] = v.Float()
	case reflect.Int64, reflect.Int:
		m[tag] = v.Int()
	case reflect.String:
		m[tag] = v.String()
	case reflect.Slice:
		return m.setSlice(tag, v)
	default:
		return fmt.Errorf("unsupported type %s", kind.String())
	}

	return nil
}

func (m Mapping) setSlice(tag string, v reflect.Value) error {
	switch slice := v.Interface().(type) {
	case []string, []int64, []int, []float64, []float32:
		m[tag] = slice
	default:
		return fmt.Errorf("unsupported type slice %T", v.Interface())
	}

	return nil
}
