package pillars

import (
	"reflect"
	"time"

	fieldmask_utils "github.com/mennanov/fieldmask-utils"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func MapOptionalDurationToPtrSeconds(d optional.Duration) *int64 {
	if !d.Valid {
		return nil
	}
	res := int64(d.Must().Seconds())
	return &res
}

func MapOptionalDurationToPtrDuration(d optional.Duration) *time.Duration {
	if !d.Valid {
		return nil
	}
	return &d.Duration
}

func MapOptionalInt64ToPtrInt64(v optional.Int64) *int64 {
	if !v.Valid {
		return nil
	}
	res := v.Must()
	return &res
}

func MapPtrInt64ToOptionalInt64(v *int64) optional.Int64 {
	if v == nil {
		return optional.Int64{}
	}
	return optional.NewInt64(*v)
}

func MapOptionalBoolToPtrInt64(b optional.Bool) *int64 {
	if !b.Valid {
		return nil
	}
	var res int64
	if b.Must() {
		res = 1
	}
	return &res
}

func MapPtrInt64ToOptionalBool(v *int64) optional.Bool {
	if v == nil {
		return optional.Bool{}
	}
	return optional.NewBool(*v > 0)
}

func MapOptionalBoolToPtrBool(b optional.Bool) *bool {
	if !b.Valid {
		return nil
	}
	return &b.Bool
}

func MapPtrBoolToOptionalBool(v *bool) optional.Bool {
	if v == nil {
		return optional.Bool{}
	}
	return optional.NewBool(*v)
}

func MapPtrStringToOptionalString(v *string) optional.String {
	if v == nil {
		return optional.String{}
	}
	return optional.NewString(*v)
}

func MapOptionalFloat64ToPtrFloat64(b optional.Float64) *float64 {
	if !b.Valid {
		return nil
	}
	return &b.Float64
}

func MapPtrFloat64ToOptionalFloat64(v *float64) optional.Float64 {
	if v == nil {
		return optional.Float64{}
	}
	return optional.NewFloat64(*v)
}

func MapPtrSecondsToOptionalDuration(seconds *int64) optional.Duration {
	if seconds == nil {
		return optional.Duration{}
	}
	return optional.NewDuration(time.Second * time.Duration(*seconds))
}

func MapOptionalStringToPtrString(s optional.String) *string {
	if !s.Valid {
		return nil
	}
	return &s.String
}

// MapModelToPillar maps all optional.{Int64,Duration,Bool,String} or plain fields from model to *{int64, *time.Duration, bool, string} in pillar
func MapModelToPillar(model interface{}, pillar interface{}) error {
	modelVal := reflect.ValueOf(model)
	pillarVal := reflect.ValueOf(pillar)

	if modelVal.Kind() != reflect.Ptr || modelVal.Elem().Kind() != reflect.Struct {
		return xerrors.Errorf("model type is not struct pointer: %+v, %+v", model, modelVal.Type())
	}

	if pillarVal.Kind() != reflect.Ptr || pillarVal.Elem().Kind() != reflect.Struct {
		return xerrors.Errorf("pillar type is not struct pointer: %+v, %+v", pillar, pillarVal.Type())
	}

	modelVal = modelVal.Elem()
	pillarVal = pillarVal.Elem()

	for i := 0; i < modelVal.NumField(); i++ {
		field := modelVal.Type().Field(i)

		if _, ok := pillarVal.Type().FieldByName(field.Name); !ok {
			continue
		}

		modelField := modelVal.Field(i)
		pillarField := pillarVal.FieldByName(field.Name)

		if val, ok := field.Tag.Lookup("mapping"); ok {
			if val == "omit" {
				continue
			}

			if val == "map" {
				if modelField.Kind() != reflect.Struct {
					return xerrors.Errorf("only structs can be mapped, but model field %q has kind %q", field.Name, modelField.Kind().String())
				}

				if pillarField.Kind() != reflect.Struct {
					return xerrors.Errorf("only structs can be mapped, but pillar field %q has kind %q", field.Name, pillarField.Kind().String())
				}

				if err := MapModelToPillar(modelField.Addr().Interface(), pillarField.Addr().Interface()); err != nil {
					return err
				}
				continue
			}
		}

		if !pillarField.CanSet() {
			continue
		}

		if modelField.Kind() == reflect.Struct {
			if _, ok := modelField.Type().FieldByName("Valid"); !ok {
				continue
			}

			if v := modelField.FieldByName("Valid"); !v.Bool() {
				continue
			}

			for j := 0; j < modelField.NumField(); j++ {
				if modelField.Type().Field(j).Name != "Valid" {
					modelField = modelField.Field(j)
					break
				}
			}
		}

		if pillarField.Kind() == reflect.Ptr {
			modelField = modelField.Addr()
		}

		if pillarField.Type() != modelField.Type() {
			return xerrors.Errorf("field %q type mismatch '%+v' != '%+v'", field.Name, pillarField.Type(), modelField.Type())
		}
		pillarField.Set(modelField)
	}

	return nil
}

// MapPillarToModel maps all *{int64, bool, string} from pillar to optional.{Int64,Bool,String} fields from model
func MapPillarToModel(pillar interface{}, model interface{}) error {
	modelVal := reflect.ValueOf(model)
	pillarVal := reflect.ValueOf(pillar)

	if modelVal.Kind() != reflect.Ptr || modelVal.Elem().Kind() != reflect.Struct {
		return xerrors.Errorf("model type is not struct pointer: %+v, %+v", model, modelVal.Type())
	}

	if pillarVal.Kind() != reflect.Ptr || pillarVal.Elem().Kind() != reflect.Struct {
		return xerrors.Errorf("pillar type is not struct pointer: %+v, %+v", pillar, pillarVal.Type())
	}

	modelVal = modelVal.Elem()
	pillarVal = pillarVal.Elem()

	for i := 0; i < pillarVal.NumField(); i++ {
		field := pillarVal.Type().Field(i)
		fieldName := field.Name

		if _, ok := modelVal.Type().FieldByName(fieldName); !ok {
			continue
		}

		pillarField := pillarVal.Field(i)
		modelField := modelVal.FieldByName(fieldName)

		if val, ok := field.Tag.Lookup("mapping"); ok {
			if val == "omit" {
				continue
			}

			if val == "map" {
				if modelField.Kind() != reflect.Struct {
					return xerrors.Errorf("only structs can be mapped, but model field %q has kind %q", field.Name, modelField.Kind().String())
				}

				if pillarField.Kind() != reflect.Struct {
					return xerrors.Errorf("only structs can be mapped, but pillar field %q has kind %q", field.Name, pillarField.Kind().String())
				}

				if err := MapPillarToModel(pillarField.Addr().Interface(), modelField.Addr().Interface()); err != nil {
					return err
				}
				continue
			}
		}

		if !modelField.CanSet() {
			continue
		}

		if pillarField.Kind() == reflect.Ptr {
			if pillarField.IsNil() {
				continue
			}

			pillarField = pillarField.Elem()
		}

		if modelField.Kind() == reflect.Struct {
			if _, ok := modelField.Type().FieldByName("Valid"); !ok {
				continue
			}

			*modelField.FieldByName("Valid").Addr().Interface().(*bool) = true
			for j := 0; j < modelField.NumField(); j++ {
				if modelField.Type().Field(j).Name != "Valid" {
					modelField = modelField.Field(j)
					break
				}
			}
		}

		if pillarField.Type() != modelField.Type() {
			return xerrors.Errorf("field %q type mismatch %q!=%q", fieldName, pillarField.Type().String(), modelField.String())
		}

		modelField.Set(pillarField)
	}

	return nil
}

// UpdatePillarByFieldMask updates `current` value from `update` if it is not filtered out.
func UpdatePillarByFieldMask(current interface{}, update interface{}, filter fieldmask_utils.FieldFilter) error {
	currentVal := reflect.ValueOf(current)
	updateVal := reflect.ValueOf(update)

	if currentVal.Type() != updateVal.Type() {
		return xerrors.Errorf("current and update type mismatch: %q!=%q", currentVal.Type().Name(), updateVal.Type().Name())
	}

	// Check only one, because they are equal
	if currentVal.Kind() != reflect.Ptr || currentVal.Elem().Kind() != reflect.Struct {
		return xerrors.Errorf("current type is not struct pointer: %+v, %+v", current, currentVal.Type())
	}

	currentVal = currentVal.Elem()
	updateVal = updateVal.Elem()

	for i := 0; i < currentVal.NumField(); i++ {
		subFilter, ok := filter.Filter(currentVal.Type().Field(i).Tag.Get("name"))
		if !ok {
			continue
		}

		currentField := currentVal.Field(i)
		updateField := updateVal.Field(i)

		if !currentField.CanSet() {
			continue
		}

		if currentField.Kind() == reflect.Struct {
			if err := UpdatePillarByFieldMask(currentField.Addr().Interface(), updateField.Addr().Interface(), subFilter); err != nil {
				return err
			}
		} else {
			currentField.Set(updateField)
		}
	}

	return nil
}
