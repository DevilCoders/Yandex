package skuresolve

import (
	"time"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

// PathsMatcher is collection of value matchers with paths
type PathsMatcher map[string]MetricValueMatcher

func (m PathsMatcher) ValueMatches(mg MetricValueGetter) (bool, error) {
	for path, matcher := range m {
		matched, err := mg.MatchByPath(path, matcher)
		if !matched || err != nil {
			return false, err
		}
	}
	return true, nil
}

// NewNullMatcher matches if value is null
func NewNullMatcher() MetricValueMatcher {
	return valueMatcher{
		null:       true,
		number:     nanDecimal,
		valueTypes: []fastjson.Type{fastjson.TypeNull},
	}
}

// NewStringMatcher matches if value is string and equal to value
func NewStringMatcher(value string) MetricValueMatcher {
	return valueMatcher{
		emptyString: value == "",
		string:      value,
		number:      nanDecimal,
		valueTypes:  []fastjson.Type{fastjson.TypeString},
	}
}

// NewBoolMatcher matches if value is bool and equal to value
func NewBoolMatcher(value bool) MetricValueMatcher {
	if value {
		return valueMatcher{
			number:     nanDecimal,
			valueTypes: []fastjson.Type{fastjson.TypeTrue},
		}
	}
	return valueMatcher{
		number:     nanDecimal,
		valueTypes: []fastjson.Type{fastjson.TypeFalse},
	}
}

// NewNumberMatcher matches if value is number and equal to value
func NewNumberMatcher(value decimal.Decimal128) MetricValueMatcher {
	return valueMatcher{
		number:     value,
		valueTypes: []fastjson.Type{fastjson.TypeNumber, fastjson.TypeString},
	}
}

type existsMatcher struct {
	not bool
}

// NewExistsMatcher check value existence
func NewExistsMatcher(exists bool) MetricValueMatcher {
	return existsMatcher{not: !exists}
}

type valueMatcher struct {
	valueTypes []fastjson.Type

	null        bool
	emptyString bool
	string      string
	number      decimal.Decimal128
}

func (m valueMatcher) MatchJSON(v *fastjson.Value) bool {
	if v == nil && m.null { // NOTE: special case for some ancient rules compatibility
		return true
	}

	if v == nil || !m.matchType(v) {
		return false
	}

	vn := fastjsonDecimal(v)
	switch {
	case m.null && v.Type() == fastjson.TypeNull:
		return true
	case m.emptyString && len(v.GetStringBytes()) == 0:
		return true
	case m.string != "" && string(v.GetStringBytes()) == m.string:
		return true
	case m.number.IsFinite() && vn.IsFinite() && vn.Cmp(m.number) == 0:
		return true
	case v.Type() == fastjson.TypeTrue:
		return true
	case v.Type() == fastjson.TypeFalse:
		return true
	default:
		return false
	}
}

func (m valueMatcher) MatchValue(v interface{}) bool {
	switch vv := v.(type) {
	case decimal.Decimal128: // usage.quantity only for now
		return m.number.IsFinite() && vv.IsFinite() && m.number.Cmp(vv) == 0
	case time.Time:
		if vv.IsZero() {
			return false
		}
		ts, _ := decimal.FromInt64(vv.Unix())
		return m.number.IsFinite() && !vv.IsZero() && m.number.Cmp(ts) == 0
	case string:
		return m.string != "" && m.string == vv || m.emptyString && vv == ""
	default:
		return false
	}
}

func (m valueMatcher) matchType(v *fastjson.Value) bool {
	vt := v.Type()
	for _, t := range m.valueTypes {
		if t == vt {
			return true
		}
	}
	return false
}

func (m existsMatcher) MatchJSON(v *fastjson.Value) bool {
	if v != nil {
		return !m.not
	}
	return m.not
}

func (m existsMatcher) MatchValue(v interface{}) bool {
	switch vv := v.(type) {
	case decimal.Decimal128: // usage.quantity only for now
		if vv.IsZero() {
			return m.not
		}
		return !m.not
	case time.Time: // usage.finish only for now
		if vv.IsZero() || vv.Unix() == 0 {
			return m.not
		}
		return !m.not
	default:
		return false
	}
}
