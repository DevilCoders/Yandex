package model

// TODO: check preconditions.

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters/structs"
)

func TestLicenseRules(t *testing.T) {
	suite.Run(t, new(LicenseRulesTestSuite))
}

func TestBillingAccountLicenseRules(t *testing.T) {
	suite.Run(t, new(BillingAccountLicenseRulesTestSuite))
}

func TestPermissionStagesRules(t *testing.T) {
	suite.Run(t, new(PermissionStagesRulesTestSuite))
}

func TestHasRule(t *testing.T) {
	suite.Run(t, new(HasRuleTestSuite))
}

func TestWhitelistRule(t *testing.T) {
	suite.Run(t, new(WhitelistRuleTestSuite))
}

func TestBlacklistRule(t *testing.T) {
	suite.Run(t, new(BlacklistRuleTestSuite))
}

func TestRegexpRule(t *testing.T) {
	suite.Run(t, new(RegexpRuleTestsuite))
}

func TestRangeRule(t *testing.T) {
	suite.Run(t, new(RangeRuleTestSuite))
}

func TestRegexpPatternNormalization(t *testing.T) {
	suite.Run(t, new(RegexpPatternNormalizationTestSuite))
}

type LicenseRulesTestSuite struct {
	suite.Suite
}

func (suite *LicenseRulesTestSuite) TestNillInitStructures() {
	result, err := MakeAllLicenseRulesExpression(nil, nil)
	suite.Require().NoError(err)
	suite.Require().Empty(result)

	suite.Require().True(result.Evaluate())
}

func (suite *LicenseRulesTestSuite) TestEmptyRuleSet() {
	billingAccount := BillingAccount{}

	m, err := structs.NewMapping(&billingAccount)
	suite.Require().NoError(err)
	suite.Require().Empty(m)

	permSet := NewPermissionStages()

	result, err := MakeAllLicenseRulesExpression(m, &permSet)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

type BillingAccountLicenseRulesTestSuite struct {
	suite.Suite
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingNotExpectedUsageStatus() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "kaboom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path:     "usage_status",
				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom",
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingAccountNilExpectedSpec() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "kaboom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: nil,
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingAccountEmptyExpectedSpec() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "kaboom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingExpectedUsageStatus() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "boom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom",
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

type testExtraInnerStruct struct {
	DeepStatus string `json:"deep_status"`
}

type testInnerStruct struct {
	Other struct {
		ExtraEmbedded *testExtraInnerStruct `json:"ExtraEmbedded"`
	} `json:"Other"`
}

type testStruct struct {
	UsageStatus  string           `json:"usage_status"`
	SomeEmbedded *testInnerStruct `json:"SomeEmbedded"`
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingWhiteListWithDeepSubstructSuccess() {
	var source testStruct
	source.SomeEmbedded = &testInnerStruct{}
	source.SomeEmbedded.Other.ExtraEmbedded = &testExtraInnerStruct{}
	source.SomeEmbedded.Other.ExtraEmbedded.DeepStatus = "deep"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "some_embedded.other.extra_embedded.deep_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom", "deep",
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingWhiteListWithDeepSubstructFailure() {
	var source testStruct
	source.SomeEmbedded = &testInnerStruct{}

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "some_embedded.other.extra_embedded.deep_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom", "deep",
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingHasWithNotSoDeepSubstructSuccess() {
	var source testStruct
	source.SomeEmbedded = &testInnerStruct{}
	source.SomeEmbedded.Other.ExtraEmbedded = &testExtraInnerStruct{}

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "some_embedded.other.extra_embedded",

				Category: HasRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingHasWithDeepSubstructFailure() {
	var source testStruct
	source.SomeEmbedded = &testInnerStruct{}

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "some_embedded.other.extra_embedded",

				Category: HasRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingHasWithShallowStructSuccess() {
	var source testStruct
	source.SomeEmbedded = &testInnerStruct{}

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "some_embedded",

				Category: HasRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingExpectedUsageStatusHasPreconditionFailure() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "boom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom",
				},
			},
			Precondition: &BaseRule{
				Path: "main_usage_status",

				Category: HasRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingExpectedUsageStatusHasPreconditionSuccess() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "boom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom",
				},
			},
			Precondition: &BaseRule{
				Path: "usage_status",

				Category: HasRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,
			},

			Info: struct{ Message string }{
				Message: "boom",
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestBillingExpectedUsageStatusWithMessages() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "boom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	expectedMessage := "failure"

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom",
				},
			},
		},
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: WhiteListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom",
				},
			},

			Info: struct{ Message string }{Message: expectedMessage},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	ok, messages := result.Evaluate()
	suite.Require().False(ok)
	suite.Require().Len(messages, 1)
	suite.Require().Contains(messages, expectedMessage)
}

func (suite *BillingAccountLicenseRulesTestSuite) TestExistantUsageStatusBlackListed() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "boom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: BlackListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom", "boom",
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)
	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

func (suite *BillingAccountLicenseRulesTestSuite) TestNonExistantUsageStatusBlackListed() {
	var source struct {
		UsageStatus string `json:"usage_status"`
	}

	source.UsageStatus = "boom"

	m, err := structs.NewMapping(&source)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(m)

	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "usage_status",

				Category: BlackListRulesSpecCategory,
				Entity:   BillingAccountRuleEntity,

				Expected: []string{
					"zoom", "doom",
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(m, nil, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

type PermissionStagesRulesTestSuite struct {
	suite.Suite
}

func (suite *PermissionStagesRulesTestSuite) TestEmptyPermissionSet() {
	permSet := NewPermissionStages()
	result, err := MakeAllLicenseRulesExpression(nil, &permSet)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

func (suite *PermissionStagesRulesTestSuite) TestExistantPermissionSet() {
	permSet := NewPermissionStages("a", "b", "c")
	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "a",

				Category: WhiteListRulesSpecCategory,
				Entity:   CloudPermissionStagesRuleEntity,

				Expected: []string{
					existenceRuleMarker,
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(nil, &permSet, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().True(result.Evaluate())
}

func (suite *PermissionStagesRulesTestSuite) TestEmptyExpectedValue() {
	permSet := NewPermissionStages("a", "b", "c")
	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "b",

				Category: WhiteListRulesSpecCategory,
				Entity:   CloudPermissionStagesRuleEntity,

				Expected: []string{},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(nil, &permSet, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

func (suite *PermissionStagesRulesTestSuite) TestUnexpectedPermission() {
	expectedMessage := "failed"
	permSet := NewPermissionStages("a", "b", "c")
	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "z",

				Category: WhiteListRulesSpecCategory,
				Entity:   CloudPermissionStagesRuleEntity,

				Expected: []string{
					"exists",
				},
			},

			Info: struct {
				Message string
			}{
				Message: expectedMessage,
			},
		},
	}

	rule, err := MakeAllLicenseRulesExpression(nil, &permSet, rulesSpecs...)
	suite.Require().NoError(err)
	result, messages := rule.Evaluate()
	suite.Require().False(result)
	suite.Require().Len(messages, 1)
	suite.Require().Contains(messages, expectedMessage)
}

func (suite *PermissionStagesRulesTestSuite) TestUnexpectedPermissionBlackListed() {
	permSet := NewPermissionStages("a", "b", "c")
	rulesSpecs := []RuleSpec{
		{
			BaseRule: BaseRule{
				Path: "z",

				Category: BlackListRulesSpecCategory,
				Entity:   CloudPermissionStagesRuleEntity,

				Expected: []string{
					existenceRuleMarker,
				},
			},
		},
	}

	result, err := MakeAllLicenseRulesExpression(nil, &permSet, rulesSpecs...)

	suite.Require().NoError(err)
	suite.Require().False(result.Evaluate())
}

type baseRuleTestSuite struct {
	suite.Suite

	testCase struct {
		StringField string `json:"a"`
		InnerStruct struct {
			OtherField string `json:"b"`

			RangeIntValue    int     `json:"iRange"`
			RangeFloatValue  float64 `json:"fRange"`
			RangeStringValue string  `json:"s_range"`
		} `json:"inner"`
	}

	mapping structs.Mapping
}

func (suite *baseRuleTestSuite) SetupTest() {
	suite.testCase.StringField = "string"
	suite.testCase.InnerStruct.OtherField = "inner.string"

	suite.testCase.InnerStruct.RangeIntValue = 1
	suite.testCase.InnerStruct.RangeFloatValue = 2.
	suite.testCase.InnerStruct.RangeStringValue = "3.14"

	var err error
	suite.mapping, err = structs.NewMapping(suite.testCase)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(suite.mapping)
}

type HasRuleTestSuite struct {
	baseRuleTestSuite
}

func (suite *HasRuleTestSuite) TestHasFieldOk() {
	rule := newHasRule(suite.mapping, "a", "has a field", nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate())
}

func (suite *HasRuleTestSuite) TestHasInnerFieldOk() {
	rule := newHasRule(suite.mapping, "inner.b", "has a rule", nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate())
}

func (suite *HasRuleTestSuite) TestHasFieldNotFound() {
	expectedMessage := "doesn't has rule"
	rule := newHasRule(suite.mapping, "inner.c", expectedMessage, nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate())
	suite.Require().Equal(expectedMessage, rule.Message())
}

type WhitelistRuleTestSuite struct {
	baseRuleTestSuite
}

func (suite *WhitelistRuleTestSuite) TestEqualFieldExists() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a",
			Expected: []string{
				"some",
				"other",
				"string",
			},
		},
	}

	rule := newWhitelistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to compare field value")
}

func (suite *WhitelistRuleTestSuite) TestEqualFieldNotExpected() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a",
			Expected: []string{
				"not expected",
			},
		},
	}

	rule := newWhitelistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "false positive")
}

func (suite *WhitelistRuleTestSuite) TestEqualInnerFieldOk() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "inner.b",
			Expected: []string{
				"some",
				"other",
				"inner.string",
			},
		},
	}

	rule := newWhitelistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)
	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *WhitelistRuleTestSuite) TestEqualInnerFieldNotExpected() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a.inner.b",
			Expected: []string{
				"some",
				"other",
				"string",
			},
		},
	}

	rule := newWhitelistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)
	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *WhitelistRuleTestSuite) TestPathNotExists() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a.inner.inner.b",
			Expected: []string{
				"some",
				"other",
				"string",
			},
		},
	}

	rule := newWhitelistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)
	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

type BlacklistRuleTestSuite struct {
	baseRuleTestSuite
}

func (suite *BlacklistRuleTestSuite) SetupTest() {
	suite.testCase.StringField = "string"
	suite.testCase.InnerStruct.OtherField = "inner.string"

	var err error
	suite.mapping, err = structs.NewMapping(suite.testCase)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(suite.mapping)
}

func (suite *BlacklistRuleTestSuite) TestEqualFieldExists() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a",
			Expected: []string{
				"some",
				"other",
				"string",
			},
		},
	}

	rule := newBlacklistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "value blacklisted")
}

func (suite *BlacklistRuleTestSuite) TestEqualFieldNotExpected() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a",
			Expected: []string{
				"not expected",
			},
		},
	}

	rule := newBlacklistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "value blacklisted")
}

func (suite *BlacklistRuleTestSuite) TestEqualInnerFieldOk() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a.inner.b",
			Expected: []string{
				"some",
				"other",
				"inner.string",
			},
		},
	}

	rule := newBlacklistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)
	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *BlacklistRuleTestSuite) TestEqualInnerFieldNotExpected() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "inner.b",
			Expected: []string{
				"some",
				"other",
				"string",
			},
		},
	}

	rule := newBlacklistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)
	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *BlacklistRuleTestSuite) TestEqualInnerPathNotExists() {
	ruleSpec := RuleSpec{
		BaseRule: BaseRule{
			Path: "a.inner.inner.b",
			Expected: []string{
				"some",
				"other",
				"inner.string",
			},
		},
	}

	rule := newBlacklistRule(suite.mapping, ruleSpec, nil)
	suite.Require().NotNil(rule)
	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

type RegexpRuleTestsuite struct {
	baseRuleTestSuite
}

func (suite *RegexpRuleTestsuite) TestMatch() {
	rule := newRegexpRule(suite.mapping, "a", "^str", "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RegexpRuleTestsuite) TestMatchWithPreconditionSuccess() {
	hasPrecondition := newHasRule(suite.mapping, "a", "", nil)
	suite.Require().NotNil(hasPrecondition)

	rule := newRegexpRule(suite.mapping, "a", "^str", "message1", hasPrecondition)

	suite.Require().NotNil(rule)
	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RegexpRuleTestsuite) TestNonExistantPath() {
	rule := newRegexpRule(suite.mapping, "inner.z", "^$", "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RegexpRuleTestsuite) TestMatchFailure() {
	rule := newRegexpRule(suite.mapping, "a", "^ztr", "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RegexpRuleTestsuite) TestInnerMatch() {
	rule := newRegexpRule(suite.mapping, "inner.b", "ing$", "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RegexpRuleTestsuite) TestInnerMatchWithPreconditionSuccess() {
	hasPrecondition := newHasRule(suite.mapping, "inner.b", "", nil)
	suite.Require().NotNil(hasPrecondition)

	rule := newRegexpRule(suite.mapping, "inner.b", "ing$", "message1", hasPrecondition)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RegexpRuleTestsuite) TestInnerMatchFailure() {
	rule := newRegexpRule(suite.mapping, "inner.b", "zing$", "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RegexpRuleTestsuite) TestInnerMatchWithPreconditionFailure() {
	hasPrecondition := newHasRule(suite.mapping, "inner.z", "", nil)
	suite.Require().NotNil(hasPrecondition)

	rule := newRegexpRule(suite.mapping, "inner.b", "ing$", "message1", hasPrecondition)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

type RegexpPatternNormalizationTestSuite struct {
	suite.Suite
}

func (suite *RegexpPatternNormalizationTestSuite) TestSuccessfulNormalization() {
	pattern := "/^abc$/"
	expected := "^abc$"

	result := trimSlashes(pattern)

	suite.Require().Equal(expected, result)
}

func (suite *RegexpPatternNormalizationTestSuite) TestEmptyString() {
	pattern := ""
	expected := ""

	result := trimSlashes(pattern)

	suite.Require().Equal(expected, result)
}

func (suite *RegexpPatternNormalizationTestSuite) TestSingleSymbol() {
	pattern := "/"
	expected := "/"

	result := trimSlashes(pattern)

	suite.Require().Equal(expected, result)
}

func (suite *RegexpPatternNormalizationTestSuite) TestSymmetricsSlashes() {
	pattern := "//"
	expected := ""

	result := trimSlashes(pattern)

	suite.Require().Equal(expected, result)
}

func (suite *RegexpPatternNormalizationTestSuite) TestSymmetricsMultipleSlashes() {
	pattern := "/////"
	expected := "///"

	result := trimSlashes(pattern)

	suite.Require().Equal(expected, result)
}

func (suite *RegexpPatternNormalizationTestSuite) TestAsymmetricsMultipleSlashes() {
	pattern := "////$"
	expected := pattern

	result := trimSlashes(pattern)

	suite.Require().Equal(expected, result)
}

func (suite *RegexpPatternNormalizationTestSuite) TestNormalizedPattern() {
	pattern := "^foo-bar$"
	expected := pattern

	result := trimSlashes(pattern)

	suite.Require().Equal(expected, result)
}

type RangeRuleTestSuite struct {
	baseRuleTestSuite
}

func (suite *RangeRuleTestSuite) TestIntOk() {
	rule := newRangeRule(suite.mapping, "inner.i_range", 0., 2., "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RangeRuleTestSuite) TestIntOutOfRange() {
	rule := newRangeRule(suite.mapping, "inner.i_range", 0., 1., "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RangeRuleTestSuite) TestFloatOk() {
	rule := newRangeRule(suite.mapping, "inner.f_range", 0., 4., "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RangeRuleTestSuite) TestFloatOutOfRange() {
	rule := newRangeRule(suite.mapping, "inner.f_range", 0., 2., "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RangeRuleTestSuite) TestStringOk() {
	rule := newRangeRule(suite.mapping, "inner.s_range", 0., 4., "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().True(rule.Evaluate(), "failed to evaluate expression")
}

func (suite *RangeRuleTestSuite) TestStringOutOfRange() {
	rule := newRangeRule(suite.mapping, "inner.s_range", 0., 3., "message1", nil)
	suite.Require().NotNil(rule)

	suite.Require().False(rule.Evaluate(), "failed to evaluate expression")
}
