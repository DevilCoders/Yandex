package jmesengine

import (
	"errors"
	"fmt"
	"sort"
	"strings"
	"unicode/utf8"

	"github.com/OneOfOne/xxhash"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/jmesparse"
)

type jpFunction func(arguments []ExecValue) (ExecValue, error)

type jpType string

const (
	// jpUnknown     jpType = "unknown"
	jpNumber      jpType = "number"
	jpString      jpType = "string"
	jpArray       jpType = "array"
	jpObject      jpType = "object"
	jpArrayNumber jpType = "array[number]"
	jpArrayString jpType = "array[string]"
	jpExpref      jpType = "expref"
	jpAny         jpType = "any"

	jpBoolean jpType = "boolean"
	jpNull    jpType = "null"
)

type functionEntry struct {
	arguments []argSpec
	handler   jpFunction
	hasExpRef bool
}

type argSpec struct {
	types    []jpType
	variadic bool
}

type byExprString struct {
	intr     *TreeInterpreter
	node     jmesparse.ASTNode
	items    []*fastjson.Value
	hasError bool
}

func (a *byExprString) Len() int {
	return len(a.items)
}

func (a *byExprString) Swap(i, j int) {
	a.items[i], a.items[j] = a.items[j], a.items[i]
}

func (a *byExprString) Less(i, j int) bool {
	first, err := a.intr.Execute(a.node, Value(a.items[i]))
	if err != nil {
		a.hasError = true
		// Return a dummy value.
		return true
	}
	if first.valueType() != fastjson.TypeString {
		a.hasError = true
		return true
	}

	second, err := a.intr.Execute(a.node, Value(a.items[j]))
	if err != nil {
		a.hasError = true
		// Return a dummy value.
		return true
	}
	if second.valueType() != fastjson.TypeString {
		a.hasError = true
		return true
	}

	return string(first.value.GetStringBytes()) < string(second.value.GetStringBytes())
}

type byExprNumber struct {
	intr     *TreeInterpreter
	node     jmesparse.ASTNode
	items    []*fastjson.Value
	hasError bool
}

func (a *byExprNumber) Len() int {
	return len(a.items)
}

func (a *byExprNumber) Swap(i, j int) {
	a.items[i], a.items[j] = a.items[j], a.items[i]
}

func (a *byExprNumber) Less(i, j int) bool {
	first, err := a.intr.Execute(a.node, Value(a.items[i]))
	if err != nil {
		a.hasError = true
		// Return a dummy value.
		return true
	}
	ith := decimalFromAny(first.value)
	if !ith.IsFinite() {
		a.hasError = true
		return true
	}
	second, err := a.intr.Execute(a.node, Value(a.items[j]))
	if err != nil {
		a.hasError = true
		// Return a dummy value.
		return true
	}
	jth := decimalFromAny(second.value)
	if !jth.IsFinite() {
		a.hasError = true
		return true
	}
	return ith.Cmp(jth) < 0
}

type sortNumber []decimal.Decimal128

func (a sortNumber) Len() int {
	return len(a)
}

func (a sortNumber) Swap(i, j int) {
	a[i], a[j] = a[j], a[i]
}

func (a sortNumber) Less(i, j int) bool {
	return a[i].Cmp(a[j]) < 0
}

type functionCaller struct {
	functionTable map[string]functionEntry
}

func newFunctionCaller() *functionCaller {
	caller := &functionCaller{}
	caller.functionTable = map[string]functionEntry{
		"length": {
			arguments: []argSpec{
				{types: []jpType{jpString, jpArray, jpObject}},
			},
			handler: jpfLength,
		},
		"starts_with": {
			arguments: []argSpec{
				{types: []jpType{jpString}},
				{types: []jpType{jpString}},
			},
			handler: jpfStartsWith,
		},
		"abs": {
			arguments: []argSpec{
				{types: []jpType{jpNumber}},
			},
			handler: jpfAbs,
		},
		"avg": {
			arguments: []argSpec{
				{types: []jpType{jpArrayNumber}},
			},
			handler: jpfAvg,
		},
		"ceil": {
			arguments: []argSpec{
				{types: []jpType{jpNumber}},
			},
			handler: jpfCeil,
		},
		"contains": {
			arguments: []argSpec{
				{types: []jpType{jpArray, jpString}},
				{types: []jpType{jpAny}},
			},
			handler: jpfContains,
		},
		"ends_with": {
			arguments: []argSpec{
				{types: []jpType{jpString}},
				{types: []jpType{jpString}},
			},
			handler: jpfEndsWith,
		},
		"floor": {
			arguments: []argSpec{
				{types: []jpType{jpNumber}},
			},
			handler: jpfFloor,
		},
		"map": {
			arguments: []argSpec{
				{types: []jpType{jpExpref}},
				{types: []jpType{jpArray}},
			},
			handler:   jpfMap,
			hasExpRef: true,
		},
		"max": {
			arguments: []argSpec{
				{types: []jpType{jpArrayNumber, jpArrayString}},
			},
			handler: jpfMax,
		},
		"merge": {
			arguments: []argSpec{
				{types: []jpType{jpObject}, variadic: true},
			},
			handler: jpfMerge,
		},
		"max_by": {
			arguments: []argSpec{
				{types: []jpType{jpArray}},
				{types: []jpType{jpExpref}},
			},
			handler:   jpfMaxBy,
			hasExpRef: true,
		},
		"sum": {
			arguments: []argSpec{
				{types: []jpType{jpArrayNumber}},
			},
			handler: jpfSum,
		},
		"min": {
			arguments: []argSpec{
				{types: []jpType{jpArrayNumber, jpArrayString}},
			},
			handler: jpfMin,
		},
		"min_by": {
			arguments: []argSpec{
				{types: []jpType{jpArray}},
				{types: []jpType{jpExpref}},
			},
			handler:   jpfMinBy,
			hasExpRef: true,
		},
		"type": {
			arguments: []argSpec{
				{types: []jpType{jpAny}},
			},
			handler: jpfType,
		},
		"keys": {
			arguments: []argSpec{
				{types: []jpType{jpObject}},
			},
			handler: jpfKeys,
		},
		"values": {
			arguments: []argSpec{
				{types: []jpType{jpObject}},
			},
			handler: jpfValues,
		},
		"sort": {
			arguments: []argSpec{
				{types: []jpType{jpArrayString, jpArrayNumber}},
			},
			handler: jpfSort,
		},
		"sort_by": {
			arguments: []argSpec{
				{types: []jpType{jpArray}},
				{types: []jpType{jpExpref}},
			},
			handler:   jpfSortBy,
			hasExpRef: true,
		},
		"join": {
			arguments: []argSpec{
				{types: []jpType{jpString}},
				{types: []jpType{jpArrayString}},
			},
			handler: jpfJoin,
		},
		"reverse": {
			arguments: []argSpec{
				{types: []jpType{jpArray, jpString}},
			},
			handler: jpfReverse,
		},
		"to_array": {
			arguments: []argSpec{
				{types: []jpType{jpAny}},
			},
			handler: jpfToArray,
		},
		"to_string": {
			arguments: []argSpec{
				{types: []jpType{jpAny}},
			},
			handler: jpfToString,
		},
		"to_number": {
			arguments: []argSpec{
				{types: []jpType{jpAny}},
			},
			handler: jpfToNumber,
		},
		"not_null": {
			arguments: []argSpec{
				{types: []jpType{jpAny}, variadic: true},
			},
			handler: jpfNotNull,
		},

		// Custom functions
		"add": {
			arguments: []argSpec{
				{types: []jpType{jpNumber, jpNull}},
				{types: []jpType{jpNumber, jpNull}},
			},
			handler: jpfAdd,
		},
		"mul": {
			arguments: []argSpec{
				{types: []jpType{jpNumber, jpNull}},
				{types: []jpType{jpNumber, jpNull}},
			},
			handler: jpfMul,
		},
		"quantum": {
			arguments: []argSpec{
				{types: []jpType{jpNumber, jpNull}},
				{types: []jpType{jpNumber, jpNull}},
			},
			handler: jpfQuantum,
		},
		"hash_mod": {
			arguments: []argSpec{
				{types: []jpType{jpString, jpNull}},
				{types: []jpType{jpNumber, jpNull}},
			},
			handler: jpfHashMod,
		},
		"div": {
			arguments: []argSpec{
				{types: []jpType{jpNumber, jpNull}},
				{types: []jpType{jpNumber, jpNull}},
			},
			handler: jpfDiv,
		},
		"sub": {
			arguments: []argSpec{
				{types: []jpType{jpNumber, jpNull}},
				{types: []jpType{jpNumber, jpNull}},
			},
			handler: jpfSub,
		},
		"if": {
			arguments: []argSpec{
				{types: []jpType{jpBoolean, jpString}},
				{types: []jpType{jpAny}},
				{types: []jpType{jpAny}},
			},
			handler: jpfIf,
		},
		"decimal": {
			arguments: []argSpec{
				{types: []jpType{jpAny}},
			},
			handler: jpfToNumber,
		},
	}
	return caller
}

func (e *functionEntry) resolveArgs(arguments []ExecValue) ([]ExecValue, error) {
	if len(e.arguments) == 0 {
		return arguments, nil
	}
	if !e.arguments[len(e.arguments)-1].variadic {
		if len(e.arguments) != len(arguments) {
			return nil, errors.New("incorrect number of args")
		}
		for i, spec := range e.arguments {
			userArg := arguments[i]
			err := spec.typeCheck(userArg)
			if err != nil {
				return nil, err
			}
		}
		return arguments, nil
	}
	if len(arguments) < len(e.arguments) {
		return nil, errors.New("invalid arity")
	}
	return arguments, nil
}

func (a *argSpec) typeCheck(arg ExecValue) error {
	for _, t := range a.types {
		switch t {
		case jpNumber:
			d := decimalFromAny(arg.value)
			if d.IsFinite() {
				return nil
			}
		case jpString:
			if arg.valueType() == fastjson.TypeString {
				return nil
			}
		case jpArray:
			if arg.valueType() == fastjson.TypeArray {
				return nil
			}
		case jpObject:
			if arg.valueType() == fastjson.TypeObject {
				return nil
			}
		case jpArrayNumber:
			if _, ok := toArrayNum(arg.value); ok {
				return nil
			}
		case jpArrayString:
			if _, ok := toArrayStr(arg.value); ok {
				return nil
			}
		case jpAny:
			return nil
		case jpExpref:
			if arg.ref.NodeType != jmesparse.ASTEmpty {
				return nil
			}
		case jpBoolean:
			t := arg.valueType()
			if t == fastjson.TypeFalse || t == fastjson.TypeTrue {
				return nil
			}
		case jpNull:
			if arg.valueType() == fastjson.TypeNull {
				return nil
			}
		}
	}
	return fmt.Errorf("invalid type for: %s, expected: %#v", arg.String(), a.types)
}

func (f *functionCaller) CallFunction(name string, arguments []ExecValue, intr *TreeInterpreter) (ExecValue, error) {
	entry, ok := f.functionTable[name]
	if !ok {
		return nilValue, errors.New("unknown function: " + name)
	}
	resolvedArgs, err := entry.resolveArgs(arguments)
	if err != nil {
		return nilValue, err
	}
	if entry.hasExpRef {
		var extra []ExecValue
		extra = append(extra, intrParam(intr))
		resolvedArgs = append(extra, resolvedArgs...)
	}
	return entry.handler(resolvedArgs)
}

func jpfAbs(arguments []ExecValue) (ExecValue, error) {
	num := decimalFromAny(arguments[0].value)
	return Value(anyToValue(num.Abs())), nil
}

func jpfLength(arguments []ExecValue) (ExecValue, error) {
	arg := arguments[0]
	switch arg.valueType() {
	case fastjson.TypeString:
		len := utf8.RuneCountInString(string(arg.value.GetStringBytes()))
		return Value(anyToValue(len)), nil
	case fastjson.TypeArray:
		return Value(anyToValue(len(arg.value.GetArray()))), nil
	case fastjson.TypeObject:
		return Value(anyToValue(arg.value.GetObject().Len())), nil
	}
	return nilValue, errors.New("could not compute length()")
}

func jpfStartsWith(arguments []ExecValue) (ExecValue, error) {
	search := string(arguments[0].value.GetStringBytes())
	prefix := string(arguments[1].value.GetStringBytes())
	return boolToValue(strings.HasPrefix(search, prefix)), nil
}

func jpfAvg(arguments []ExecValue) (ExecValue, error) {
	// We've already type checked the value so we can safely use
	// type assertions.
	args := arguments[0].value.GetArray()
	length := decimal.Must(decimal.FromInt64(int64(len(args))))
	numerator := decimal.Decimal128{}
	for _, n := range args {
		numerator = numerator.Add(decimalFromAny(n))
	}
	result := numerator.Div(length)
	return Value(anyToValue(result)), nil
}

func jpfCeil(arguments []ExecValue) (ExecValue, error) {
	val := decimalFromAny(arguments[0].value)
	result := val.Ceil()
	return Value(anyToValue(result)), nil
}

func jpfContains(arguments []ExecValue) (ExecValue, error) {
	search := arguments[0].value
	el := arguments[1].value

	switch search.Type() {
	case fastjson.TypeString:
		if el.Type() != fastjson.TypeString {
			return boolToValue(false), nil
		}
		searchStr := string(search.GetStringBytes())
		elStr := string(el.GetStringBytes())
		r := strings.Contains(searchStr, elStr)
		return boolToValue(r), nil
	case fastjson.TypeArray:
		arr := search.GetArray()
		for _, item := range arr {
			if objsEqual(item, el) {
				return boolToValue(true), nil
			}
		}
	}
	return boolToValue(false), nil
}

func jpfEndsWith(arguments []ExecValue) (ExecValue, error) {
	search := string(arguments[0].value.GetStringBytes())
	suffix := string(arguments[1].value.GetStringBytes())
	return boolToValue(strings.HasSuffix(search, suffix)), nil
}

func jpfFloor(arguments []ExecValue) (ExecValue, error) {
	val := decimalFromAny(arguments[0].value)
	result := val.Floor()
	return Value(anyToValue(result)), nil
}

func jpfMap(arguments []ExecValue) (ExecValue, error) {
	intr := arguments[0].intr
	node := arguments[1].ref
	arr := arguments[2].value.GetArray()
	mapped := make([]*fastjson.Value, 0, len(arr))
	for _, value := range arr {
		current, err := intr.Execute(node, Value(value))
		if err != nil {
			return nilValue, err
		}
		mapped = append(mapped, current.value)
	}
	return arrayToValue(mapped), nil
}

func jpfMax(arguments []ExecValue) (ExecValue, error) {
	if items, ok := toArrayNum(arguments[0].value); ok {
		if len(items) == 0 {
			return nilValue, nil
		}
		if len(items) == 1 {
			return Value(anyToValue(items[0])), nil
		}
		best := items[0]
		for _, item := range items[1:] {
			if item.Cmp(best) > 0 {
				best = item
			}
		}
		return Value(anyToValue(best)), nil
	}
	// Otherwise we're dealing with a max() of strings.
	items, _ := toArrayStr(arguments[0].value)
	if len(items) == 0 {
		return nilValue, nil
	}
	if len(items) == 1 {
		return Value(anyToValue(items[0])), nil
	}
	best := items[0]
	for _, item := range items[1:] {
		if item > best {
			best = item
		}
	}
	return Value(anyToValue(best)), nil
}

func jpfMerge(arguments []ExecValue) (ExecValue, error) {
	final := objectValue()
	for _, m := range arguments {
		mapped := m.value.GetObject()
		mapped.Visit(func(key []byte, v *fastjson.Value) {
			final.Set(string(key), copyValue(v))
		})
	}
	return Value(final), nil
}

func jpfMaxBy(arguments []ExecValue) (ExecValue, error) {
	intr := arguments[0].intr
	arr := arguments[1].value.GetArray()
	node := arguments[2].ref
	if len(arr) == 0 {
		return nilValue, nil
	} else if len(arr) == 1 {
		return Value(arr[0]), nil
	}
	start, err := intr.Execute(node, Value(arr[0]))
	if err != nil {
		return nilValue, err
	}
	startNum := decimalFromAny(start.value)
	switch {
	case startNum.IsFinite():
		bestVal := startNum
		bestItem := arr[0]
		for _, item := range arr[1:] {
			result, err := intr.Execute(node, Value(item))
			if err != nil {
				return nilValue, err
			}
			current := decimalFromAny(result.value)
			if !current.IsFinite() {
				return nilValue, errors.New("invalid type, must be number")
			}
			if current.Cmp(bestVal) > 0 {
				bestVal = current
				bestItem = item
			}
		}
		return Value(bestItem), nil
	case start.valueType() == fastjson.TypeString:
		bestVal := string(start.value.GetStringBytes())
		bestItem := arr[0]
		for _, item := range arr[1:] {
			result, err := intr.Execute(node, Value(item))
			if err != nil {
				return nilValue, err
			}
			if result.valueType() != fastjson.TypeString {
				return nilValue, errors.New("invalid type, must be string")
			}
			current := string(result.value.GetStringBytes())
			if current > bestVal {
				bestVal = current
				bestItem = item
			}
		}
		return Value(bestItem), nil
	default:
		return nilValue, errors.New("invalid type, must be number of string")
	}
}

func jpfSum(arguments []ExecValue) (ExecValue, error) {
	items, _ := toArrayNum(arguments[0].value)
	sum := decimal.Decimal128{}
	for _, item := range items {
		sum = sum.Add(item)
	}
	return Value(anyToValue(sum)), nil
}

func jpfMin(arguments []ExecValue) (ExecValue, error) {
	if items, ok := toArrayNum(arguments[0].value); ok {
		if len(items) == 0 {
			return nilValue, nil
		}
		if len(items) == 1 {
			return Value(anyToValue(items[0])), nil
		}
		best := items[0]
		for _, item := range items[1:] {
			if item.Cmp(best) < 0 {
				best = item
			}
		}
		return Value(anyToValue(best)), nil
	}
	items, _ := toArrayStr(arguments[0].value)
	if len(items) == 0 {
		return nilValue, nil
	}
	if len(items) == 1 {
		return Value(anyToValue(items[0])), nil
	}
	best := items[0]
	for _, item := range items[1:] {
		if item < best {
			best = item
		}
	}
	return Value(anyToValue(best)), nil
}

func jpfMinBy(arguments []ExecValue) (ExecValue, error) {
	intr := arguments[0].intr
	arr := arguments[1].value.GetArray()
	node := arguments[2].ref
	if len(arr) == 0 {
		return nilValue, nil
	} else if len(arr) == 1 {
		return Value(arr[0]), nil
	}
	start, err := intr.Execute(node, Value(arr[0]))
	if err != nil {
		return nilValue, err
	}
	startNum := decimalFromAny(start.value)
	switch {
	case startNum.IsFinite():
		bestVal := startNum
		bestItem := arr[0]
		for _, item := range arr[1:] {
			result, err := intr.Execute(node, Value(item))
			if err != nil {
				return nilValue, err
			}
			current := decimalFromAny(result.value)
			if !current.IsFinite() {
				return nilValue, errors.New("invalid type, must be number")
			}
			if current.Cmp(bestVal) < 0 {
				bestVal = current
				bestItem = item
			}
		}
		return Value(bestItem), nil
	case start.valueType() == fastjson.TypeString:
		bestVal := string(start.value.GetStringBytes())
		bestItem := arr[0]
		for _, item := range arr[1:] {
			result, err := intr.Execute(node, Value(item))
			if err != nil {
				return nilValue, err
			}
			if result.valueType() != fastjson.TypeString {
				return nilValue, errors.New("invalid type, must be string")
			}
			current := string(result.value.GetStringBytes())
			if current < bestVal {
				bestVal = current
				bestItem = item
			}
		}
		return Value(bestItem), nil
	default:
		return nilValue, errors.New("invalid type, must be number of string")
	}
}

func jpfType(arguments []ExecValue) (ExecValue, error) {
	arg := arguments[0]
	if arg.valueType() == fastjson.TypeNull {
		return Value(anyToValue("null")), nil
	}
	if d := decimalFromAny(arg.value); d.IsFinite() {
		return Value(anyToValue("number")), nil
	}
	if arg.valueType() == fastjson.TypeString {
		return Value(anyToValue("string")), nil
	}
	if arg.valueType() == fastjson.TypeArray {
		return Value(anyToValue("array")), nil
	}
	if arg.valueType() == fastjson.TypeObject {
		return Value(anyToValue("object")), nil
	}
	if t := arg.valueType(); t == fastjson.TypeFalse || t == fastjson.TypeTrue {
		return Value(anyToValue("boolean")), nil
	}
	return nilValue, errors.New("unknown type")
}

func jpfKeys(arguments []ExecValue) (ExecValue, error) {
	arg := arguments[0].value.GetObject()
	collected := make([]*fastjson.Value, 0, arg.Len())
	arg.Visit(func(key []byte, _ *fastjson.Value) {
		collected = append(collected, anyToValue(string(key)))
	})
	return arrayToValue(collected), nil
}

func jpfValues(arguments []ExecValue) (ExecValue, error) {
	arg := arguments[0].value.GetObject()
	collected := make([]*fastjson.Value, 0, arg.Len())
	arg.Visit(func(_ []byte, v *fastjson.Value) {
		collected = append(collected, copyValue(v))
	})
	return arrayToValue(collected), nil
}

func jpfSort(arguments []ExecValue) (ExecValue, error) {
	if items, ok := toArrayNum(arguments[0].value); ok {
		d := sortNumber(items)
		sort.Stable(d)
		final := make([]*fastjson.Value, len(d))
		for i, val := range d {
			final[i] = anyToValue(val)
		}
		return arrayToValue(final), nil
	}
	// Otherwise we're dealing with sort()'ing strings.
	items, _ := toArrayStr(arguments[0].value)
	d := sort.StringSlice(items)
	sort.Stable(d)
	final := make([]*fastjson.Value, len(d))
	for i, val := range d {
		final[i] = anyToValue(val)
	}
	return arrayToValue(final), nil
}

func jpfSortBy(arguments []ExecValue) (ExecValue, error) {
	intr := arguments[0].intr
	arr := arguments[1].value.GetArray()
	node := arguments[2].ref
	if len(arr) <= 1 {
		return arrayToValue(arr), nil
	}
	start, err := intr.Execute(node, Value(arr[0]))
	if err != nil {
		return nilValue, err
	}
	startNum := decimalFromAny(start.value)
	switch {
	case startNum.IsFinite():
		sortable := &byExprNumber{intr, node, arr, false}
		sort.Stable(sortable)
		if sortable.hasError {
			return nilValue, errors.New("error in sort_by comparison")
		}
		return arrayToValue(arr), nil
	case start.valueType() == fastjson.TypeString:
		sortable := &byExprString{intr, node, arr, false}
		sort.Stable(sortable)
		if sortable.hasError {
			return nilValue, errors.New("error in sort_by comparison")
		}
		return arrayToValue(arr), nil
	default:
		return nilValue, errors.New("invalid type, must be number or string")
	}
}

func jpfJoin(arguments []ExecValue) (ExecValue, error) {
	sep := string(arguments[0].value.GetStringBytes())

	arrayStr, ok := toArrayStr(arguments[1].value)
	if !ok {
		return nilValue, errors.New("invalid type, must be string")
	}
	return Value(anyToValue(strings.Join(arrayStr, sep))), nil
}

func jpfReverse(arguments []ExecValue) (ExecValue, error) {
	if arguments[0].valueType() == fastjson.TypeString {
		r := []rune(string(arguments[0].value.GetStringBytes()))
		for i, j := 0, len(r)-1; i < len(r)/2; i, j = i+1, j-1 {
			r[i], r[j] = r[j], r[i]
		}
		return Value(anyToValue(string(r))), nil
	}
	items := arguments[0].value.GetArray()
	length := len(items)
	reversed := make([]*fastjson.Value, length)
	for i, item := range items {
		reversed[length-(i+1)] = item
	}
	return arrayToValue(reversed), nil
}

func jpfToArray(arguments []ExecValue) (ExecValue, error) {
	if arguments[0].valueType() == fastjson.TypeArray {
		return arguments[0], nil
	}
	arr := []*fastjson.Value{arguments[0].value}
	return arrayToValue(arr), nil
}

func jpfToString(arguments []ExecValue) (ExecValue, error) {
	if arguments[0].valueType() == fastjson.TypeNull {
		return Value(anyToValue("null")), nil
	}
	if arguments[0].valueType() == fastjson.TypeString {
		s := string(arguments[0].value.GetStringBytes())
		return Value(anyToValue(s)), nil
	}
	s := string(arguments[0].value.MarshalTo(nil))
	return Value(anyToValue(s)), nil
}

func jpfToNumber(arguments []ExecValue) (ExecValue, error) {
	arg := arguments[0]
	if arg.valueType() == fastjson.TypeNull {
		return nilValue, nil
	}

	d := decimalFromAny(arg.value)
	if d.IsFinite() {
		return Value(anyToValue(d)), nil
	}

	return nilValue, nil
}

func jpfNotNull(arguments []ExecValue) (ExecValue, error) {
	for _, arg := range arguments {
		if arg.valueType() != fastjson.TypeNull {
			return arg, nil
		}
	}
	return nilValue, nil
}

func jpfAdd(arguments []ExecValue) (ExecValue, error) {
	l := decimalFromAny(arguments[0].value)
	if !l.IsFinite() {
		l = decimal.Decimal128{}
	}
	r := decimalFromAny(arguments[1].value)
	if !r.IsFinite() {
		r = decimal.Decimal128{}
	}

	return Value(anyToValue(l.Add(r))), nil
}

func jpfMul(arguments []ExecValue) (ExecValue, error) {
	l := decimalFromAny(arguments[0].value)
	r := decimalFromAny(arguments[1].value)
	if !l.IsFinite() || !r.IsFinite() {
		return Value(anyToValue(0)), nil
	}
	return Value(anyToValue(l.Mul(r))), nil
}

func jpfQuantum(arguments []ExecValue) (ExecValue, error) {
	r := decimalFromAny(arguments[1].value)
	if !r.IsFinite() {
		return nilValue, nil
	}

	l := decimalFromAny(arguments[0].value)
	if !l.IsFinite() {
		return Value(anyToValue(0)), nil
	}

	qnt := l.Div(r).Floor().Mul(r)
	if l.Cmp(qnt) > 0 {
		qnt = qnt.Add(r)
	}

	return Value(anyToValue(qnt)), nil
}

func jpfHashMod(arguments []ExecValue) (ExecValue, error) {
	p := decimalFromAny(arguments[1].value)
	s := string(arguments[0].value.GetStringBytes())
	hash, _ := decimal.FromUInt64(xxhash.ChecksumString64(s))
	if !p.IsFinite() {
		return nilValue, nil
	}
	if !hash.IsFinite() {
		return nilValue, nil
	}

	res := hash.Sub(hash.Div(p).Floor().Mul(p))
	return Value(anyToValue(res)), nil
}

func jpfDiv(arguments []ExecValue) (ExecValue, error) {
	r := decimalFromAny(arguments[1].value)
	if !r.IsFinite() {
		return nilValue, nil
	}

	l := decimalFromAny(arguments[0].value)
	if !l.IsFinite() {
		return Value(anyToValue(0)), nil
	}

	d := l.Div(r)
	if !d.IsFinite() {
		return nilValue, nil
	}

	return Value(anyToValue(d)), nil
}

func jpfSub(arguments []ExecValue) (ExecValue, error) {
	l := decimalFromAny(arguments[0].value)
	if !l.IsFinite() {
		l = decimal.Decimal128{}
	}
	r := decimalFromAny(arguments[1].value)
	if !r.IsFinite() {
		r = decimal.Decimal128{}
	}

	return Value(anyToValue(l.Sub(r))), nil
}

func jpfIf(arguments []ExecValue) (ExecValue, error) {
	ifCond, ifTrue, ifFalse := arguments[0], arguments[1], arguments[2]
	if IsFalse(ifCond) {
		return ifFalse, nil
	}
	return ifTrue, nil
}
