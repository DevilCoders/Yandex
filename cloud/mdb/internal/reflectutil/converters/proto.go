package converters

import (
	"fmt"

	"github.com/golang/protobuf/ptypes/wrappers"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func OptionalFromGRPC(i interface{}) interface{} {
	switch v := i.(type) {
	case *wrappers.Int64Value:
		return optional.Int64{Valid: v != nil, Int64: v.GetValue()}
	case *wrappers.UInt64Value:
		return optional.Uint64{Valid: v != nil, Uint64: v.GetValue()}
	case *wrappers.DoubleValue:
		return optional.Float64{Valid: v != nil, Float64: v.GetValue()}
	case *wrappers.FloatValue:
		return optional.Float64{Valid: v != nil, Float64: float64(v.GetValue())}
	case *wrappers.StringValue:
		return optional.String{Valid: v != nil, String: v.GetValue()}
	case *wrappers.BoolValue:
		return optional.Bool{Valid: v != nil, Bool: v.GetValue()}
	default:
		panic(fmt.Sprintf("value: %v of unknown type was passed instead of one from *wrapppers", i))
	}
}

func OptionalToGRPC(field interface{}) interface{} {
	switch v := field.(type) {
	case optional.Int64:
		if !v.Valid {
			return (*wrappers.Int64Value)(nil)
		}
		return &wrappers.Int64Value{Value: v.Int64}
	case optional.Uint64:
		if !v.Valid {
			return (*wrappers.UInt64Value)(nil)
		}
		return &wrappers.UInt64Value{Value: v.Uint64}
	case optional.Float64:
		if !v.Valid {
			return (*wrappers.DoubleValue)(nil)
		}
		return &wrappers.DoubleValue{Value: v.Float64}
	case optional.String:
		if !v.Valid {
			return (*wrappers.StringValue)(nil)
		}
		return &wrappers.StringValue{Value: v.String}
	case optional.Bool:
		if !v.Valid {
			return (*wrappers.BoolValue)(nil)
		}
		return &wrappers.BoolValue{Value: v.Bool}
	default:
		panic(fmt.Sprintf("value: %v of unknown type was passed instead of one from optional package", field))
	}
}
