package tracing

import (
	olog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/library/go/core/log"
)

func convertLogField(field log.Field) olog.Field {
	switch field.Type() {
	case log.FieldTypeString:
		return olog.String(field.Key(), field.String())
	case log.FieldTypeBoolean:
		return olog.Bool(field.Key(), field.Bool())
	case log.FieldTypeSigned:
		return olog.Int64(field.Key(), field.Signed())
	case log.FieldTypeUnsigned:
		return olog.Uint64(field.Key(), field.Unsigned())
	case log.FieldTypeFloat:
		return olog.Float64(field.Key(), field.Float())
	case log.FieldTypeTime:
		return olog.String(field.Key(), field.Time().String())
	case log.FieldTypeDuration:
		return olog.String(field.Key(), field.Duration().String())
	case log.FieldTypeError:
		return olog.Error(field.Error())
	default:
		return olog.Object(field.Key(), field.Any())
	}
}
