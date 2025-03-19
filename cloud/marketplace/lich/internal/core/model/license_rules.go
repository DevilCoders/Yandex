package model

import (
	"fmt"
	"strconv"
	"strings"

	"a.yandex-team.ru/mds/go/xstrings"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters/structs"
)

const (
	existenceRuleMarker = "exists"
)

type PermissionStages struct {
	xstrings.StringsSet
}

func NewPermissionStages(p ...string) PermissionStages {
	return PermissionStages{
		StringsSet: xstrings.NewSet(p...),
	}
}

type RuleSpecEntity string

func (r RuleSpecEntity) String() string {
	return string(r)
}

const (
	BillingAccountRuleEntity        RuleSpecEntity = "billing_account"
	CloudPermissionStagesRuleEntity RuleSpecEntity = "cloud_permission_stages"
)

type RuleSpecCategory string

const (
	BlackListRulesSpecCategory RuleSpecCategory = "blacklist"
	WhiteListRulesSpecCategory RuleSpecCategory = "whitelist"
	RegexRulesSpecCategory     RuleSpecCategory = "regex"
	HasRulesSpecCategory       RuleSpecCategory = "has"
	RangeRulesSpecCategory     RuleSpecCategory = "range"
)

func (c RuleSpecCategory) String() string {
	return string(c)
}

func (c RuleSpecCategory) Validate() error {
	switch c {
	case BlackListRulesSpecCategory,
		WhiteListRulesSpecCategory,
		RegexRulesSpecCategory,
		HasRulesSpecCategory,
		RangeRulesSpecCategory:
		return nil
	default:
		return fmt.Errorf("unsupported rule spec category type %s", c.String())
	}
}

type BaseRule struct {
	Path string

	Category RuleSpecCategory
	Entity   RuleSpecEntity

	Expected []string
}

type RuleSpec struct {
	BaseRule
	Precondition *BaseRule

	Info struct {
		Message string
	}
}

type RuleExpression interface {
	Evaluate() bool
	CheckPrecondition() bool

	Message() string
}

type AllRuleExpression []RuleExpression

func (r AllRuleExpression) Evaluate() (bool, []string) {
	var (
		result   = true
		messages []string
	)

	for _, rule := range r {
		if !rule.CheckPrecondition() {
			continue
		}

		ok := rule.Evaluate()
		if !ok && rule.Message() != "" {
			messages = append(messages, rule.Message())
		}

		result = result && ok
	}

	return result, messages
}

func MakeAllLicenseRulesExpression(
	mapping structs.Mapping, permissionStages *PermissionStages, specs ...RuleSpec,
) (AllRuleExpression, error) {
	var out AllRuleExpression

	for _, spec := range specs {
		var m structs.Mapping

		switch spec.Entity {
		case BillingAccountRuleEntity:
			m = mapping
		case CloudPermissionStagesRuleEntity:
			m = newPermissionsMapping(permissionStages, spec.Path)
		default:
			return nil, fmt.Errorf("unsupported entity spec %s", spec.Entity)
		}

		out = append(out, newRule(m, spec))
	}

	return out, nil
}

type baseRule struct {
	mapping structs.Mapping

	path         string
	checkMessage string

	precondition RuleExpression
}

func (b *baseRule) Message() string {
	return b.checkMessage
}

func (b *baseRule) CheckPrecondition() bool {
	if b.precondition != nil {
		return b.precondition.Evaluate()
	}

	return true
}

type blacklistRule struct {
	baseRule

	expected xstrings.StringsSet
}

func newBlacklistRule(
	mapping structs.Mapping, spec RuleSpec, precondition RuleExpression,
) *blacklistRule {
	return &blacklistRule{
		baseRule: baseRule{
			mapping: mapping,

			path:         spec.Path,
			checkMessage: spec.Info.Message,

			precondition: precondition,
		},

		expected: xstrings.NewSet(spec.Expected...),
	}
}

func (b *blacklistRule) Evaluate() bool {
	// TODO: support more base types.
	if v, ok := b.mapping.Value(b.path).(string); ok {
		return !b.expected.Member(v)
	}

	return false
}

type whitelistRule struct {
	baseRule
	expected xstrings.StringsSet
}

func (b *whitelistRule) Evaluate() bool {
	// TODO: support more base types.
	if v, ok := b.mapping.Value(b.path).(string); ok {
		return b.expected.Member(v)
	}

	return false
}

func newWhitelistRule(
	mapping structs.Mapping, spec RuleSpec, precondition RuleExpression,
) *whitelistRule {
	return &whitelistRule{
		baseRule: baseRule{
			mapping: mapping,

			path:         spec.Path,
			checkMessage: spec.Info.Message,

			precondition: precondition,
		},

		expected: xstrings.NewSet(spec.Expected...),
	}
}

type hasRule struct {
	baseRule
}

func newHasRule(mapping structs.Mapping, path, checkMessage string, precondition RuleExpression) *hasRule {
	return &hasRule{
		baseRule: baseRule{
			mapping: mapping,

			path:         path,
			checkMessage: checkMessage,

			precondition: precondition,
		},
	}
}

func (p *hasRule) Evaluate() bool {
	return p.mapping.ContainsPath(p.path)
}

type rangeRule struct {
	baseRule

	lowBound float64
	hiBound  float64
}

func newRangeRule(
	mapping structs.Mapping, path string, lowBound, hiBound float64, checkMessage string, precondition RuleExpression,
) *rangeRule {
	return &rangeRule{
		baseRule: baseRule{
			mapping: mapping,

			path:         path,
			checkMessage: checkMessage,

			precondition: precondition,
		},

		lowBound: lowBound,
		hiBound:  hiBound,
	}
}

func (p *rangeRule) Evaluate() bool {
	return p.mapping.InRange(p.path, p.lowBound, p.hiBound)
}

type regexpRule struct {
	baseRule

	pattern string
}

func newRegexpRule(mapping structs.Mapping, path, pattern, checkMessage string, precondition RuleExpression) *regexpRule {
	return &regexpRule{
		baseRule: baseRule{
			mapping: mapping,

			path:         path,
			checkMessage: checkMessage,

			precondition: precondition,
		},

		pattern: trimSlashes(pattern),
	}
}

// trimSlashes removes Perl/JS style slashes around the pattern.
func trimSlashes(pattern string) string {
	if strings.HasPrefix(pattern, "/") && strings.HasSuffix(pattern, "/") && len(pattern) > 1 {
		return pattern[1 : len(pattern)-1]
	}

	return pattern
}

func (p *regexpRule) Evaluate() bool {
	return p.mapping.Match(p.path, p.pattern)
}

func newRule(mapping structs.Mapping, spec RuleSpec) RuleExpression {
	var precondition RuleExpression
	if spec.Precondition != nil {
		precondition = newRule(mapping, RuleSpec{
			BaseRule: *spec.Precondition,
		})
	}

	switch spec.Category {
	case BlackListRulesSpecCategory:
		return newBlacklistRule(mapping, spec, precondition)
	case WhiteListRulesSpecCategory:
		return newWhitelistRule(mapping, spec, precondition)
	case HasRulesSpecCategory:
		return newHasRule(mapping, spec.Path, spec.Info.Message, precondition)
	case RegexRulesSpecCategory:
		if len(spec.Expected) != 0 {
			pattern := spec.Expected[0]
			return newRegexpRule(mapping, spec.Path, pattern, spec.Info.Message, precondition)
		}
	case RangeRulesSpecCategory:
		low, hi, err := parseRuleRange(spec.Expected)
		if err != nil {
			return newRangeRule(mapping, spec.Path, low, hi, spec.Info.Message, precondition)
		}
	}

	return newFalseRule(spec.Info.Message)
}

func parseRuleRange(expected []string) (low, hi float64, err error) {
	if len(expected) < 2 {
		return 0., 0., fmt.Errorf("unexpected number of fields")
	}

	if low, err = strconv.ParseFloat(expected[0], 64); err != nil {
		return 0., 0., err
	}

	if hi, err = strconv.ParseFloat(expected[1], 64); err != nil {
		return 0., 0., err
	}

	return low, hi, err
}

func newPermissionsMapping(permissionStages *PermissionStages, path string) structs.Mapping {
	value := existenceRuleMarker
	if !permissionStages.Member(path) {
		value = "None"
	}

	return structs.NewStringsMapping(value, permissionStages.Elements()...)
}

type falseRule struct {
	message string
}

func newFalseRule(message string) *falseRule {
	return &falseRule{
		message: message,
	}
}

func (*falseRule) Evaluate() bool {
	return false
}

func (*falseRule) CheckPrecondition() bool {
	return true
}

func (f *falseRule) Message() string {
	return f.message
}
