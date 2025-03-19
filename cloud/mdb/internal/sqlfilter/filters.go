package sqlfilter

import (
	"fmt"
	"time"

	"github.com/alecthomas/participle/lexer"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter/grammar"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func errorPrefix(pos lexer.Position) string {
	if pos.Line > 1 {
		return fmt.Sprintf("filter syntax error at or near line %d column %d: ", pos.Line, pos.Column)
	}
	return fmt.Sprintf("filter syntax error at or near column %d: ", pos.Column)
}

func newSyntaxError(pos lexer.Position, message string) error {
	return semerr.InvalidInput(errorPrefix(pos) + message)
}

func newSyntaxErrorf(pos lexer.Position, format string, a ...interface{}) error {
	return semerr.InvalidInputf(errorPrefix(pos)+format, a...)
}

type OperatorType int

const (
	Equals OperatorType = iota
	NotEquals
	Less
	LessOrEquals
	Greater
	GreaterOrEquals
	In
	NotIn
)

func (o OperatorType) String() string {
	switch o {
	case Equals:
		return "="
	case NotEquals:
		return "!="
	case Greater:
		return ">"
	case GreaterOrEquals:
		return ">="
	case Less:
		return "<"
	case LessOrEquals:
		return "<="
	case In:
		return "IN"
	case NotIn:
		return "NOT IN"
	}
	return "Unknown"
}

// Term ...
type Term struct {
	Attribute string
	Operator  OperatorType
	Value     Value
}

type Value struct {
	parsed grammar.Value
}

func (v Value) IsBool() bool {
	return v.parsed.Bool != nil
}

func (v Value) AsBool() bool {
	return bool(*v.parsed.Bool)
}

func (v Value) IsString() bool {
	return v.parsed.String != nil
}

func (v Value) AsString() string {
	return *v.parsed.String
}

func (v Value) IsInt() bool {
	return v.parsed.Int != nil
}

func (v Value) AsInt() int64 {
	return v.parsed.Int.I
}

func (v Value) IsTime() bool {
	return v.parsed.DateTime != nil
}

func (v Value) AsTime() time.Time {
	return v.parsed.DateTime.T
}

func (v Value) isNotEmptyList() bool {
	return len(v.parsed.List) > 0
}

func (v Value) IsBoolList() bool {
	if v.isNotEmptyList() {
		return v.parsed.List[0].Bool != nil
	}
	return false
}

func (v Value) AsBoolList() []bool {
	ret := make([]bool, len(v.parsed.List))
	for i, vi := range v.parsed.List {
		ret[i] = bool(*vi.Bool)
	}
	return ret
}

func (v Value) IsStringList() bool {
	if v.isNotEmptyList() {
		return v.parsed.List[0].String != nil
	}
	return false
}

func (v Value) AsStringList() []string {
	ret := make([]string, len(v.parsed.List))
	for i, vi := range v.parsed.List {
		ret[i] = *vi.String
	}
	return ret
}

func (v Value) IsIntList() bool {
	if v.isNotEmptyList() {
		return v.parsed.List[0].Int != nil
	}
	return false
}

func (v Value) AsIntList() []int64 {
	ret := make([]int64, len(v.parsed.List))
	for i, vi := range v.parsed.List {
		ret[i] = vi.Int.I
	}
	return ret
}

func (v Value) IsTimeList() bool {
	if v.isNotEmptyList() {
		return v.parsed.List[0].DateTime != nil
	}
	return false
}

func (v Value) AsTimeList() []time.Time {
	ret := make([]time.Time, len(v.parsed.List))
	for i, vi := range v.parsed.List {
		ret[i] = vi.DateTime.T
	}
	return ret
}

func opFromG(op grammar.Operator) (OperatorType, error) {
	r := Equals
	switch op {
	case "=":
		r = Equals
	case "!=":
		r = NotEquals
	case ">":
		r = Greater
	case ">=":
		r = GreaterOrEquals
	case "<":
		r = Less
	case "<=":
		r = LessOrEquals
	case "IN":
		r = In
	case "NOT IN":
		r = NotIn
	default:
		return r, xerrors.Errorf("unexpected operator: %q", op)
	}
	return r, nil
}

func validateTerm(f grammar.Term, op OperatorType) error {
	if len(f.Value.List) > 0 {
		head := f.Value.List[0]
		for i, lv := range f.Value.List {
			if lv.TypeOf() != head.TypeOf() {
				return newSyntaxErrorf(
					f.Value.Pos,
					"list items should have same type. Item %d is %s. Previous items are %ss",
					i, lv.TypeOf(), head.TypeOf())
			}
		}
		if len(head.List) > 0 {
			return newSyntaxError(
				f.Value.Pos,
				"nested list are not supported")
		}
		if op != In && op != NotIn {
			return newSyntaxError(f.Pos, "list values require [ NOT ] IN operator")
		}
	} else {
		if op == In || op == NotIn {
			return newSyntaxErrorf(f.Pos, "%s operator expect list value, got %s", f.Operator, f.Value.TypeOf())
		}
	}
	return nil
}

func fromParsed(f grammar.Term) (Term, error) {
	op, err := opFromG(f.Operator)
	if err != nil {
		return Term{}, err
	}
	err = validateTerm(f, op)
	if err != nil {
		return Term{}, err
	}
	return Term{
		Attribute: f.Attribute,
		Operator:  op,
		Value:     Value{parsed: *f.Value},
	}, nil
}

// Parse filter string and returns list of Term
func Parse(filtersString string) ([]Term, error) {
	parsed, err := grammar.Parse(filtersString)
	if err != nil {
		var syntaxErr *grammar.SyntaxError
		if xerrors.As(err, &syntaxErr) {
			return nil, newSyntaxError(syntaxErr.Pos, syntaxErr.Error())
		}
		return nil, xerrors.Errorf("unexpected parse error: %w", err)
	}
	var ret []Term
	for _, term := range parsed {
		f, err := fromParsed(term)
		if err != nil {
			return nil, err
		}
		ret = append(ret, f)
	}
	return ret, nil
}
