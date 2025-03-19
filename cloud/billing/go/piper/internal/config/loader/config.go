package loader

// NOTE: this is fork of confita.Loader
import (
	"context"
	"encoding"
	"errors"
	"fmt"
	"reflect"
	"strings"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
)

// Loader loads configuration keys from backends and stores them is a struct.
type Loader struct {
	backends []backend.Backend

	// Tag specifies the tag name used to parse
	// configuration keys and options.
	// If empty, "config" is used.
	Tag string
}

// NewLoader creates a Loader. If no backend is specified, the loader uses the environment.
func New(backends ...backend.Backend) *Loader {
	l := Loader{
		backends: backends,
	}

	if len(l.backends) == 0 {
		l.backends = append(l.backends, env.NewBackend())
	}

	return &l
}

// Load analyses all the Fields of the given struct for a "config" tag and queries each backend
// in order for the corresponding key. The given context can be used for timeout and cancelation.
func (l *Loader) Load(ctx context.Context, to interface{}) error {
	select {
	case <-ctx.Done():
		return ctx.Err()
	default:
	}

	ref := reflect.ValueOf(to)

	if !ref.IsValid() || ref.Kind() != reflect.Ptr || ref.Elem().Kind() != reflect.Struct {
		return errors.New("provided target must be a pointer to struct")
	}

	ref = ref.Elem()

	s := l.parseStruct(&ref, "")
	s.S = to
	return l.resolve(ctx, s)
}

func (l *Loader) parseStruct(ref *reflect.Value, namespace string) *confita.StructConfig {
	var s confita.StructConfig

	t := ref.Type()

	tagKey := l.Tag
	if tagKey == "" {
		tagKey = "config"
	}

	numFields := ref.NumField()
	for i := 0; i < numFields; i++ {
		field := t.Field(i)
		value := ref.Field(i)
		typ := value.Type()

		// skip if field is unexported
		if field.PkgPath != "" {
			continue
		}

		tag := field.Tag.Get(tagKey)
		if tag == "-" {
			continue
		}

		fieldNamespace := namespace
		if tag != "" {
			tagKey := tag
			if idx := strings.Index(tag, ","); idx != -1 {
				tagKey = tag[:idx]
			}
			fieldNamespace += tagKey + "."
		}

		isUnmarshaler := false
		switch valueInterface(&value).(type) {
		case encoding.TextUnmarshaler:
			isUnmarshaler = true
		case encoding.BinaryUnmarshaler:
			isUnmarshaler = true
		}

		switch {
		case isUnmarshaler:
		case typ.Kind() == reflect.Struct:
			s.Fields = append(s.Fields, l.parseStruct(&value, fieldNamespace).Fields...)
			continue
		case typ.Kind() == reflect.Map && typ.Key().Kind() == reflect.String:
			iter := value.MapRange()
			for iter.Next() {
				mVal := value.MapIndex(iter.Key())
				if mVal.Kind() != reflect.Ptr || mVal.Elem().Kind() != reflect.Struct {
					continue
				}
				key := iter.Key().String()
				eVal := mVal.Elem()
				s.Fields = append(s.Fields, l.parseStruct(&eVal, fieldNamespace+key+".").Fields...)
			}
			continue
		}

		// empty tag or no tag, skip the field
		if tag == "" {
			continue
		}

		// if value can not be setted then skip
		if !value.CanSet() {
			continue
		}

		f := confita.FieldConfig{
			Name:  field.Name,
			Key:   namespace + tag,
			Value: &value,
		}

		// copying field content to a new value
		clone := reflect.Indirect(reflect.New(f.Value.Type()))
		clone.Set(*f.Value)
		f.Default = &clone

		if idx := strings.Index(tag, ","); idx != -1 {
			f.Key = tag[:idx]
			opts := strings.Split(tag[idx+1:], ",")

			for _, opt := range opts {
				if opt == "required" {
					f.Required = true
				}

				if strings.HasPrefix(opt, "backend=") {
					f.Backend = opt[len("backend="):]
				}
			}
		}

		s.Fields = append(s.Fields, &f)
	}

	return &s
}

func (l *Loader) resolve(ctx context.Context, s *confita.StructConfig) error {
	for _, f := range s.Fields {
		if f.Backend != "" {
			var found bool
			for _, b := range l.backends {
				if b.Name() == f.Backend {
					found = true
					break
				}
			}

			if !found {
				return fmt.Errorf("the backend: '%s' is not supported", f.Backend)
			}
		}
	}

	for _, b := range l.backends {
		select {
		case <-ctx.Done():
			return ctx.Err()
		default:
		}

		if u, ok := b.(confita.Unmarshaler); ok {
			if err := u.Unmarshal(ctx, s.S); err != nil {
				return err
			}

			continue
		}

		if u, ok := b.(confita.StructLoader); ok {
			if err := u.LoadStruct(ctx, s); err != nil {
				return err
			}

			continue
		}

		for _, f := range s.Fields {
			if f.Backend != "" && f.Backend != b.Name() {
				continue
			}

			raw, err := b.Get(ctx, f.Key)
			if err != nil {
				if err == backend.ErrNotFound {
					continue
				}

				return err
			}

			switch um := valueInterface(f.Value).(type) {
			case encoding.TextUnmarshaler:
				err = um.UnmarshalText(raw)
			case encoding.BinaryUnmarshaler:
				err = um.UnmarshalBinary(raw)
			default:
				err = f.Set(string(raw))
			}

			if err != nil {
				return err
			}
		}
	}

	for _, f := range s.Fields {
		if f.Required && isZero(f.Value) {
			return fmt.Errorf("required key '%s' for field '%s' not found", f.Key, f.Name)
		}
	}

	return nil
}

func isZero(v *reflect.Value) bool {
	zero := reflect.Zero(v.Type()).Interface()
	current := v.Interface()
	return reflect.DeepEqual(current, zero)
}

func valueInterface(v *reflect.Value) interface{} {
	if v.Kind() == reflect.Ptr {
		return v.Interface()
	}
	if v.CanAddr() {
		return v.Addr().Interface()
	}
	return v.Interface()
}
