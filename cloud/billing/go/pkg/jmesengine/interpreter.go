package jmesengine

import (
	"errors"
	"strconv"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/jmesparse"
)

/* This is a tree based interpreter.  It walks the AST and directly
   interprets the AST to search through a JSON document.
*/

type TreeInterpreter struct {
	fCall *functionCaller
}

func NewInterpreter() *TreeInterpreter {
	interpreter := TreeInterpreter{}
	interpreter.fCall = newFunctionCaller()
	return &interpreter
}

type ExecValue struct {
	value *fastjson.Value
	ref   jmesparse.ASTNode
	intr  *TreeInterpreter
}

func (ev ExecValue) Value() *fastjson.Value {
	if ev.value == nil {
		v := nullValue
		return &v
	}
	return ev.value
}

func (ev ExecValue) String() string {
	if ev.ref.NodeType != jmesparse.ASTEmpty {
		return ev.ref.String()
	}
	if ev.intr != nil {
		return "<interpreter>"
	}
	return ev.value.String()
}

// Execute takes an ASTNode and input data and interprets the AST directly.
// It will produce the result of applying the JMESPath expression associated
// with the ASTNode to the input data "value".
func (intr *TreeInterpreter) Execute(node jmesparse.ASTNode, value ExecValue) (ExecValue, error) {
	switch node.NodeType {
	case jmesparse.ASTComparator:
		left, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, err
		}
		right, err := intr.Execute(node.Children[1], value)
		if err != nil {
			return nilValue, err
		}
		switch node.Value {
		case jmesparse.TEQ:
			eq := objsEqual(left.value, right.value)
			return boolToValue(eq), nil
		case jmesparse.TNE:
			ne := !objsEqual(left.value, right.value)
			return boolToValue(ne), nil
		}
		leftNum := decimalFromAny(left.value)
		if !leftNum.IsFinite() {
			return nilValue, nil
		}
		rightNum := decimalFromAny(right.value)
		if !rightNum.IsFinite() {
			return nilValue, nil
		}
		switch node.Value {
		case jmesparse.TGT:
			return boolToValue(leftNum.Cmp(rightNum) > 0), nil
		case jmesparse.TGTE:
			return boolToValue(leftNum.Cmp(rightNum) >= 0), nil
		case jmesparse.TLT:
			return boolToValue(leftNum.Cmp(rightNum) < 0), nil
		case jmesparse.TLTE:
			return boolToValue(leftNum.Cmp(rightNum) <= 0), nil
		}
	case jmesparse.ASTExpRef:
		return expRef(node.Children[0]), nil
	case jmesparse.ASTFunctionExpression:
		resolvedArgs := make([]ExecValue, 0, len(node.Children))
		for _, arg := range node.Children {
			current, err := intr.Execute(arg, value)
			if err != nil {
				return nilValue, err
			}
			resolvedArgs = append(resolvedArgs, current)
		}
		return intr.fCall.CallFunction(node.Value.(string), resolvedArgs, intr)
	case jmesparse.ASTField:
		v := value.value.Get(node.Value.(string))
		return Value(v), nil
	case jmesparse.ASTFilterProjection:
		left, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, nil
		}
		if left.valueType() != fastjson.TypeArray {
			return nilValue, nil
		}
		arr := left.value.GetArray()
		compareNode := node.Children[2]
		collected := make([]*fastjson.Value, 0, len(arr))

		for _, element := range arr {
			result, err := intr.Execute(compareNode, Value(element))
			if err != nil {
				return nilValue, err
			}
			if !IsFalse(result) {
				current, err := intr.Execute(node.Children[1], Value(element))
				if err != nil {
					return nilValue, err
				}
				if current.valueType() != fastjson.TypeNull {
					collected = append(collected, current.value)
				}
			}
		}
		return arrayToValue(collected), nil
	case jmesparse.ASTFlatten:
		left, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, nil
		}
		if left.valueType() != fastjson.TypeArray {
			return nilValue, nil
		}
		flattened := []*fastjson.Value{}
		for _, element := range left.value.GetArray() {
			switch element.Type() {
			case fastjson.TypeArray:
				flattened = append(flattened, element.GetArray()...)
			default:
				flattened = append(flattened, element)
			}
		}
		return arrayToValue(flattened), nil
	case jmesparse.ASTIdentity, jmesparse.ASTCurrentNode:
		return value, nil
	case jmesparse.ASTIndex:
		index := node.Value.(int)
		if index >= 0 {
			return Value(value.value.Get(strconv.Itoa(index))), nil
		}
		index = -index
		arr := value.value.GetArray()
		if index > len(arr) {
			return nilValue, nil
		}
		return Value(arr[len(arr)-index]), nil
	case jmesparse.ASTKeyValPair:
		return intr.Execute(node.Children[0], value)
	case jmesparse.ASTLiteral:
		return Value(anyToValue(node.Value)), nil
	case jmesparse.ASTMultiSelectHash:
		if value.valueType() == fastjson.TypeNull {
			return nilValue, nil
		}
		collected := objectValue()
		for _, child := range node.Children {
			current, err := intr.Execute(child, value)
			if err != nil {
				return nilValue, err
			}
			key := child.Value.(string)
			collected.Set(key, current.value)
		}
		return Value(collected), nil
	case jmesparse.ASTMultiSelectList:
		if value.valueType() == fastjson.TypeNull {
			return nilValue, nil
		}
		collected := []*fastjson.Value{}
		for _, child := range node.Children {
			current, err := intr.Execute(child, value)
			if err != nil {
				return nilValue, err
			}
			collected = append(collected, current.value)
		}
		return arrayToValue(collected), nil
	case jmesparse.ASTOrExpression:
		matched, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, err
		}
		if IsFalse(matched) {
			matched, err = intr.Execute(node.Children[1], value)
			if err != nil {
				return nilValue, err
			}
		}
		return matched, nil
	case jmesparse.ASTAndExpression:
		matched, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, err
		}
		if IsFalse(matched) {
			return matched, nil
		}
		return intr.Execute(node.Children[1], value)
	case jmesparse.ASTNotExpression:
		matched, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, err
		}
		if IsFalse(matched) {
			return boolToValue(true), nil
		}
		return boolToValue(false), nil
	case jmesparse.ASTPipe:
		result := value
		var err error
		for _, child := range node.Children {
			result, err = intr.Execute(child, result)
			if err != nil {
				return nilValue, err
			}
		}
		return result, nil
	case jmesparse.ASTProjection:
		left, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, err
		}
		if left.valueType() != fastjson.TypeArray {
			return nilValue, nil
		}
		collected := []*fastjson.Value{}
		for _, element := range left.value.GetArray() {
			current, err := intr.Execute(node.Children[1], Value(element))
			if err != nil {
				return nilValue, err
			}
			if current.valueType() != fastjson.TypeNull {
				collected = append(collected, current.value)
			}
		}
		return arrayToValue(collected), nil
	case jmesparse.ASTSubexpression, jmesparse.ASTIndexExpression:
		left, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, err
		}
		return intr.Execute(node.Children[1], left)
	case jmesparse.ASTSlice:
		if value.valueType() != fastjson.TypeArray {
			return nilValue, nil
		}
		parts := node.Value.([]*int)
		sliceParams := make([]sliceParam, 3)
		for i, part := range parts {
			if part != nil {
				sliceParams[i].Specified = true
				sliceParams[i].N = *part
			}
		}
		sliced, err := slice(value.value.GetArray(), sliceParams)
		if err != nil {
			return nilValue, err
		}
		return arrayToValue(sliced), nil
	case jmesparse.ASTValueProjection:
		left, err := intr.Execute(node.Children[0], value)
		if err != nil {
			return nilValue, nil
		}
		if left.valueType() != fastjson.TypeObject {
			return nilValue, nil
		}
		obj := left.value.GetObject()
		values := make([]*fastjson.Value, 0, obj.Len())

		obj.Visit(func(_ []byte, v *fastjson.Value) {
			values = append(values, copyValue(v))
		})

		collected := []*fastjson.Value{}
		for _, element := range values {
			current, err := intr.Execute(node.Children[1], Value(element))
			if err != nil {
				return nilValue, err
			}
			if current.valueType() != fastjson.TypeNull {
				collected = append(collected, current.value)
			}
		}
		return arrayToValue(collected), nil
	}
	return nilValue, errors.New("Unknown AST node: " + node.NodeType.String())
}

func (ev ExecValue) valueType() fastjson.Type {
	if ev.value == nil {
		return fastjson.TypeNull
	}
	return ev.value.Type()
}

func Value(v *fastjson.Value) ExecValue {
	if v == nil {
		return nilValue
	}
	return ExecValue{value: v}
}

func expRef(node jmesparse.ASTNode) ExecValue {
	return ExecValue{ref: node}
}

func intrParam(i *TreeInterpreter) ExecValue {
	return ExecValue{intr: i}
}
