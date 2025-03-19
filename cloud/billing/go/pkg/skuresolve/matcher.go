package skuresolve

import (
	"fmt"
	"strings"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/jmesengine"
)

// Matcher implemens logic for selecting resolvers by key and apply them to define sku
type Matcher map[string][]Resolver

func NewMatcher() Matcher {
	return make(Matcher)
}

// AddResolvers to matcher by key
func (sm Matcher) AddResolvers(key string, resolvers ...Resolver) {
	sm[key] = append(sm[key], resolvers...)
}

// Match metric to resolvers selected by key
func (sm Matcher) Match(key string, mg MetricValueGetter) (skus []string, err error) {
	for _, sr := range sm[key] {
		match, err := sr.Resolve(mg)
		if err != nil {
			return nil, err
		}
		if match {
			skus = append(skus, sr.Sku())
		}
	}
	return
}

// Resolver is general interface that can say if metric resolves to its sku
type Resolver interface {
	Resolve(MetricValueGetter) (bool, error)
	Sku() string
}

// NewPolicyResolver constructs resolver by jmespath policy
func NewPolicyResolver(sku string, policy JMESPath) (Resolver, error) {
	if policy == "" { // special case
		return notMatchResolver(sku), nil
	}

	ast, err := parseJMES(policy)
	if err != nil {
		return nil, err
	}
	return &policyResolver{sku: sku, policy: ast}, nil
}

// NewRulesResolver constructs resolver by set of resolving rules
func NewRulesResolver(sku string, rules ...PathsMatcher) (Resolver, error) {
	result := rulesResolver{
		sku:   sku,
		rules: rules,
	}
	return &result, nil
}

type rulesResolver struct {
	sku   string
	rules orRulesMatcher
}

func (r *rulesResolver) Sku() string { return r.sku }

func (r *rulesResolver) Resolve(mg MetricValueGetter) (bool, error) {
	return r.rules.ValueMatches(mg)
}

type orRulesMatcher []PathsMatcher

func (m orRulesMatcher) ValueMatches(mg MetricValueGetter) (bool, error) {
	for _, matcher := range m {
		matched, err := matcher.ValueMatches(mg)
		if matched || err != nil {
			return matched, err
		}
	}
	return false, nil
}

type policyResolver struct {
	sku    string
	policy *jmesAST
}

func (r *policyResolver) Sku() string { return r.sku }

func (r *policyResolver) Resolve(mg MetricValueGetter) (bool, error) {
	if r.policy.hasStatic {
		return !jmesengine.IsFalse(r.policy.staticResult), nil
	}
	metricValue := mg.GetFullValue()
	rv, err := jmesEngine.Execute(r.policy.node, jmesengine.Value(metricValue))
	if err != nil {
		return false, fmt.Errorf("policy execution error: %w", err)
	}
	return !jmesengine.IsFalse(rv), nil
}

type notMatchResolver string

func (r notMatchResolver) Sku() string { return string(r) }

func (r notMatchResolver) Resolve(mg MetricValueGetter) (bool, error) {
	return false, nil
}

func parseFastJSONPath(p string) []string {
	p = strings.ReplaceAll(p, "]", "")
	return strings.FieldsFunc(p, func(r rune) bool { return r == '.' || r == '[' })
}

var nanDecimal = decimal.Must(decimal.FromString("NaN"))

func fastjsonDecimal(v *fastjson.Value) (d decimal.Decimal128) {
	var buf [64]byte
	val := v.MarshalTo(buf[:0])
	if err := d.UnmarshalJSON(val); err != nil {
		d = nanDecimal
	}
	return
}
