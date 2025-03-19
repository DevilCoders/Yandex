package entities

import "a.yandex-team.ru/cloud/billing/go/pkg/decimal"

type Sku struct {
	SkuID string
	SkuInfo
	SkuUsageInfo
}

type SkuInfo struct {
	SkuName      string
	PricingUnit  string
	SkuUsageType UsageType
}

type SkuUsageInfo struct {
	Formula   JMESPath
	UsageUnit string
}

type SkuResolveRules struct {
	Key   string
	Rules []SkuResolve
}

type SkuResolve struct {
	SkuID  string
	Rules  []MatchRulesByPath
	Policy JMESPath
}

type MatchRulesByPath map[string]MatchRule

type MatchRule struct {
	Type   MatchRuleType
	Exists bool
	Value  ValueMatch
}

type ValueMatch struct {
	Null        bool
	EmptyString bool
	String      string
	Number      decimal.Decimal128
	True        bool
	False       bool
}

type JMESPath string
