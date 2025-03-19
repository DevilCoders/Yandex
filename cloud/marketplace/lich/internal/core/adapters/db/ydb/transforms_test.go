package ydb

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

func TestPayloadTranformation(t *testing.T) {
	suite.Run(t, new(PayloadTransformationTestSuite))
}

func TestLicenseRulesTranformation(t *testing.T) {
	suite.Run(t, new(LicenseRulesTransformationTestSuite))
}

type PayloadTransformationTestSuite struct {
	suite.Suite
}

func (suite *PayloadTransformationTestSuite) TestEmptyString() {
	const anyJSON = ``

	payload, err := payloadFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().Error(err)
	suite.Require().Empty(payload)
}

func (suite *PayloadTransformationTestSuite) TestEmptyJSONOBject() {
	const anyJSON = `{}`

	payload, err := payloadFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().NoError(err)
	suite.Require().Empty(payload)
}

func (suite *PayloadTransformationTestSuite) TestInvalidJSON() {
	const anyJSON = `{ "some_value";`

	payload, err := payloadFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().Error(err)
	suite.Require().Empty(payload)
}

func (suite *PayloadTransformationTestSuite) TestBasicTransformation() {
	const anyJSON = `{
		"compute_image": {
			"resource_spec": {
				"licensed_instance_pool": "windows_dc"
			}
		}
	}`

	payload, err := payloadFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().NoError(err)
	suite.Require().NotNil(payload.ComputeImage)
	suite.Require().Equal("windows_dc", payload.ComputeImage.ResourceSpec.LicenseInstancePool)
}

type LicenseRulesTransformationTestSuite struct {
	suite.Suite
}

func (suite *LicenseRulesTransformationTestSuite) TestEmptyString() {
	const anyJSON = ``

	licenseRules, err := licenseRulesFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().Error(err)
	suite.Require().Empty(licenseRules)
}

func (suite *LicenseRulesTransformationTestSuite) TestNonExpectedJSONOBject() {
	const anyJSON = `{}`

	licenseRules, err := licenseRulesFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().Error(err)
	suite.Require().Empty(licenseRules)
}

func (suite *LicenseRulesTransformationTestSuite) TestEmptyJSONArray() {
	const anyJSON = `[]`

	licenseRules, err := licenseRulesFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().NoError(err)
	suite.Require().Empty(licenseRules)
}

func (suite *LicenseRulesTransformationTestSuite) TestInvalidJSON() {
	const anyJSON = `{ "some_broken_value";`

	licenseRules, err := licenseRulesFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().Error(err)
	suite.Require().Empty(licenseRules)
}

func (suite *LicenseRulesTransformationTestSuite) TestCategories() {
	const anyJSON = `[
		{
			"path": "a",
			"category": "blacklist",
			"entity": "cloud_permission_stages",
			"expected": [
				"one", "two"
			]
		},
		{
			"path": "b",
			"category": "whitelist",
			"entity": "billing_account",
			"expected": [
				"three", "four"
			]
		}
	]`

	licenseRules, err := licenseRulesFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().NoError(err)
	suite.Require().Len(licenseRules, 2)

	first, second := licenseRules[0], licenseRules[1]

	suite.Require().Equal("a", first.Path)
	suite.Require().Equal(model.BlackListRulesSpecCategory, first.Category)
	suite.Require().Equal(model.CloudPermissionStagesRuleEntity, first.Entity)
	suite.Require().EqualValues(first.Expected, []string{"one", "two"})

	suite.Require().Equal("b", second.Path)
	suite.Require().Equal(model.WhiteListRulesSpecCategory, second.Category)
	suite.Require().Equal(model.BillingAccountRuleEntity, second.Entity)
	suite.Require().EqualValues(second.Expected, []string{"three", "four"})
}

func (suite *LicenseRulesTransformationTestSuite) TestUnexpectedCategoriesAndEntities() {
	const anyJSON = `[
		{
			"path": "boo",
			"category": "foo-bar",
			"entity": "entity",
			"expected": [
				"one",
				"two"
			]
		}
	]`

	licenseRules, err := licenseRulesFromAnyJSON(ydb.NewAnyJSON(anyJSON))
	suite.Require().Error(err)
	suite.Require().Empty(licenseRules)
}
