package jmesengine

import (
	"bytes"
	"encoding/json"
	"errors"

	lru "github.com/hashicorp/golang-lru"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

var (
	trueValue  = *fastjson.MustParse("true")
	falseValue = *fastjson.MustParse("false")
	nullValue  = *fastjson.MustParse("null")

	nilValue = ExecValue{value: &nullValue}

	nanDecimal = decimal.Must(decimal.FromString("NaN"))
)

// IsFalse determines if an object is false based on the JMESPath spec.
// JMESPath defines false values to be any of:
// - An empty string array, or hash.
// - The boolean value false.
// - nil
func IsFalse(ev ExecValue) bool {
	switch ev.valueType() {
	case fastjson.TypeTrue:
		return false
	case fastjson.TypeFalse:
		return true
	case fastjson.TypeArray:
		return len(ev.value.GetArray()) == 0
	case fastjson.TypeObject:
		return ev.value.GetObject().Len() == 0
	case fastjson.TypeString:
		v := ev.value.GetStringBytes()
		return len(v) == 0
	case fastjson.TypeNull:
		return true
	}
	return false
}

func objsEqual(left *fastjson.Value, right *fastjson.Value) bool {
	if left == nil {
		left = &nullValue
	}
	if right == nil {
		right = &nullValue
	}

	switch left.Type() {
	case fastjson.TypeNull:
		return right.Type() == fastjson.TypeNull
	case fastjson.TypeTrue:
		return right.Type() == fastjson.TypeTrue
	case fastjson.TypeFalse:
		return right.Type() == fastjson.TypeFalse

	case fastjson.TypeString:
		switch right.Type() {
		case fastjson.TypeNumber:
			return stringAndNumberEquals(left, right)
		case fastjson.TypeString:
			return bytes.Equal(left.GetStringBytes(), right.GetStringBytes())
		default:
			return false
		}
	case fastjson.TypeNumber:
		switch right.Type() {
		case fastjson.TypeString:
			return stringAndNumberEquals(right, left)
		case fastjson.TypeNumber:
			ld := decimalFromNumber(left)
			rd := decimalFromNumber(right)
			return ld.IsFinite() && rd.IsFinite() && ld.Cmp(rd) == 0
		default:
			return false
		}

	case fastjson.TypeObject:
		switch right.Type() {
		case fastjson.TypeObject:
			lo := left.GetObject()
			ro := right.GetObject()
			if lo.Len() != ro.Len() {
				return false
			}
			eq := true
			lo.Visit(func(key []byte, v *fastjson.Value) {
				if !eq {
					return
				}
				eq = objsEqual(v, ro.Get(string(key)))
			})
			return eq
		default:
			return false
		}
	case fastjson.TypeArray:
		switch right.Type() {
		case fastjson.TypeArray:
			la := left.GetArray()
			ra := right.GetArray()
			if len(la) != len(ra) {
				return false
			}
			for i := range la {
				if !objsEqual(la[i], ra[i]) {
					return false
				}
			}
			return true
		default:
			return false
		}
	}
	return false
}

func stringAndNumberEquals(str *fastjson.Value, num *fastjson.Value) bool {
	strDec := decimalFromString(str)
	numDec := decimalFromNumber(num)
	return strDec.IsFinite() && numDec.IsFinite() && strDec.Cmp(numDec) == 0
}

func decimalFromNumber(num *fastjson.Value) (d decimal.Decimal128) {
	var buf [64]byte
	val := string(num.MarshalTo(buf[:0]))
	d, _ = decimal.FromString(val)
	return d
}

func decimalFromString(str *fastjson.Value) (d decimal.Decimal128) {
	val := string(str.GetStringBytes())
	d, _ = decimal.FromString(val)
	return d
}

func decimalFromAny(v *fastjson.Value) (d decimal.Decimal128) {
	if v == nil {
		return nanDecimal
	}
	switch v.Type() {
	default:
		d = nanDecimal
	case fastjson.TypeNumber:
		return decimalFromNumber(v)
	case fastjson.TypeString:
		return decimalFromString(v)
	}
	return
}

func boolToValue(b bool) ExecValue {
	if b {
		return ExecValue{value: &trueValue}
	}
	return ExecValue{value: &falseValue}
}

func arrayToValue(arr []*fastjson.Value) ExecValue {
	a := fastjson.Arena{}
	r := a.NewArray()
	for i, v := range arr {
		r.SetArrayItem(i, v)
	}
	return ExecValue{value: r}
}

func objectValue() *fastjson.Value {
	a := fastjson.Arena{}
	return a.NewObject()
}

func stringValue(v string) *fastjson.Value {
	a := fastjson.Arena{}
	return a.NewString(v)
}

var (
	strCache, _   = lru.New(1024)
	floatCache, _ = lru.New(1024)
)

func anyToValue(v interface{}) *fastjson.Value {
	if v == nil {
		return &nullValue
	}

	switch vv := v.(type) {
	case bool:
		if vv {
			return &trueValue
		}
		return &falseValue
	case string:
		if len(vv) > 64 {
			return stringValue(vv)
		}
		if cVal, ok := strCache.Get(vv); ok {
			return cVal.(*fastjson.Value)
		}
		result := stringValue(vv)
		strCache.ContainsOrAdd(vv, result)
		return result
	case float64:
		if cVal, ok := floatCache.Get(vv); ok {
			return cVal.(*fastjson.Value)
		}
		result := anyToValueSlow(v)
		floatCache.ContainsOrAdd(vv, result)
		return result
	}
	return anyToValueSlow(v)
}

func anyToValueSlow(v interface{}) *fastjson.Value {
	data, err := json.Marshal(v)
	if err != nil {
		panic(err)
	}

	return fastjson.MustParseBytes(data)
}

func copyValue(v *fastjson.Value) *fastjson.Value {
	if v == nil {
		return nil
	}
	switch v.Type() {
	case fastjson.TypeArray, fastjson.TypeObject: // only this types has mutable parts
		return fastjson.MustParseBytes(v.MarshalTo(nil))
	}
	result := *v
	return &result
}

// SliceParam refers to a single part of a slice.
// A slice consists of a start, a stop, and a step, similar to
// python slices.
type sliceParam struct {
	N         int
	Specified bool
}

// Slice supports [start:stop:step] style slicing that's supported in JMESPath.
func slice(slice []*fastjson.Value, parts []sliceParam) ([]*fastjson.Value, error) {
	computed, err := computeSliceParams(len(slice), parts)
	if err != nil {
		return nil, err
	}
	start, stop, step := computed[0], computed[1], computed[2]
	result := []*fastjson.Value{}
	if step > 0 {
		for i := start; i < stop; i += step {
			result = append(result, slice[i])
		}
	} else {
		for i := start; i > stop; i += step {
			result = append(result, slice[i])
		}
	}
	return result, nil
}

func computeSliceParams(length int, parts []sliceParam) ([]int, error) {
	var start, stop, step int
	if !parts[2].Specified {
		step = 1
	} else if parts[2].N == 0 {
		return nil, errors.New("invalid slice, step cannot be 0")
	} else {
		step = parts[2].N
	}
	var stepValueNegative bool
	if step < 0 {
		stepValueNegative = true
	} else {
		stepValueNegative = false
	}

	if !parts[0].Specified {
		if stepValueNegative {
			start = length - 1
		} else {
			start = 0
		}
	} else {
		start = capSlice(length, parts[0].N, step)
	}

	if !parts[1].Specified {
		if stepValueNegative {
			stop = -1
		} else {
			stop = length
		}
	} else {
		stop = capSlice(length, parts[1].N, step)
	}
	return []int{start, stop, step}, nil
}

func capSlice(length int, actual int, step int) int {
	if actual < 0 {
		actual += length
		if actual < 0 {
			if step < 0 {
				actual = -1
			} else {
				actual = 0
			}
		}
	} else if actual >= length {
		if step < 0 {
			actual = length - 1
		} else {
			actual = length
		}
	}
	return actual
}

// ToArrayNum converts an empty interface type to a slice of float64.
// If any element in the array cannot be converted, then nil is returned
// along with a second value of false.
func toArrayNum(data *fastjson.Value) ([]decimal.Decimal128, bool) {
	if data == nil || data.Type() != fastjson.TypeArray {
		return nil, false
	}
	arr := data.GetArray()
	result := make([]decimal.Decimal128, 0, len(arr))
	for _, v := range arr {
		d := decimalFromAny(v)
		if !d.IsFinite() {
			return nil, false
		}
		result = append(result, d)
	}
	return result, true
}

// ToArrayStr converts an empty interface type to a slice of strings.
// If any element in the array cannot be converted, then nil is returned
// along with a second value of false.  If the input data could be entirely
// converted, then the converted data, along with a second value of true,
// will be returned.
func toArrayStr(data *fastjson.Value) ([]string, bool) {
	if data == nil || data.Type() != fastjson.TypeArray {
		return nil, false
	}
	arr := data.GetArray()
	result := make([]string, 0, len(arr))
	for _, v := range arr {
		if v.Type() != fastjson.TypeString {
			return nil, false
		}
		result = append(result, string(v.GetStringBytes()))
	}
	return result, true
}
