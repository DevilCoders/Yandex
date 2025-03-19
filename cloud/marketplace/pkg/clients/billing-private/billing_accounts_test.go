package billing

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestParseExtendedBillingAccount(t *testing.T) {
	suite.Run(t, new(ParseExtendedBillingAccountTestSuite))
}

func TestSchemelessInt(t *testing.T) {
	suite.Run(t, new(SchemelessIntTestSuite))
}

type ParseExtendedBillingAccountTestSuite struct {
	suite.Suite
}

func (suite *ParseExtendedBillingAccountTestSuite) TestParseNotPersonCompany() {
	const sourceJSON = `{"person": null}`

	var view ExtendedBillingAccountCamelCaseView
	err := json.Unmarshal([]byte(sourceJSON), &view)
	suite.Require().NoError(err)

	suite.Require().Nil(view.PersonData.Company)
	suite.Require().Nil(view.PersonData.Individual)
}

func (suite *ParseExtendedBillingAccountTestSuite) TestParsePersonHasCompanySnakeCase() {
	const sourceJSON = `{
		"person": {
			"company": {
				"email": "thorin.oakshield@yandex.ru",
				"inn": 100500,
				"legal_address": "my sweet home",
				"longname": "Oakshield",
				"name": "Thorint",
				"phone": "+don't call me",
				"post_address": "Lonely Mountain",
				"post_code": 10101
			}
		}
	}`

	var view ExtendedBillingAccountCamelCaseView
	err := json.Unmarshal([]byte(sourceJSON), &view)

	suite.Require().NoError(err)
	suite.Require().NotNil(view.PersonData)
	suite.Require().NotNil(view.PersonData.Company)
	suite.Require().Equal("thorin.oakshield@yandex.ru", view.PersonData.Company.Email)
	suite.Require().Equal("100500", view.PersonData.Company.INN.String())

	suite.Require().Empty(view.PersonData.Company.PostAddress)
	suite.Require().Equal("Lonely Mountain", view.PersonData.Company.PostAddressSnake)

	suite.Require().Empty(view.PersonData.Company.LegalAddress)
	suite.Require().Equal("my sweet home", view.PersonData.Company.LegalAddressSnake)
	suite.Require().Equal("10101", view.PersonData.Company.PostCodeSnake.String())
}

func (suite *ParseExtendedBillingAccountTestSuite) TestParsePersonHasCompany() {
	const sourceJSON = `{
		"person": {
			"company": {
				"email": "thorin.oakshield@yandex.ru",
				"inn": 100500,
				"legalAddress": "my sweet home",
				"longname": "Oakshield",
				"name": "Thorint",
				"phone": "+don't call me",
				"postAddress": "Lonely Mountain",
				"postCode": 10101
			}
		}
	}`

	var view ExtendedBillingAccountCamelCaseView
	err := json.Unmarshal([]byte(sourceJSON), &view)

	suite.Require().NoError(err)
	suite.Require().NotNil(view.PersonData)
	suite.Require().NotNil(view.PersonData.Company)
	suite.Require().Equal("thorin.oakshield@yandex.ru", view.PersonData.Company.Email)
	suite.Require().Equal("100500", view.PersonData.Company.INN.String())

	suite.Require().Empty(view.PersonData.Company.PostAddressSnake)
	suite.Require().Equal("Lonely Mountain", view.PersonData.Company.PostAddress)

	suite.Require().Empty(view.PersonData.Company.LegalAddressSnake)
	suite.Require().Equal("my sweet home", view.PersonData.Company.LegalAddress)

	suite.Require().Empty(view.PersonData.Company.PostCodeSnake.String())
	suite.Require().Equal("10101", view.PersonData.Company.PostCode.String())
}

func (suite *ParseExtendedBillingAccountTestSuite) TestParsePersonWithCompanyEmptyINN() {
	const sourceJSON = `{
		"person": {
			"company": {
				"inn": null,
				"some other value": 99
			}
		}
	}`

	var view ExtendedBillingAccountCamelCaseView
	err := json.Unmarshal([]byte(sourceJSON), &view)

	suite.Require().NoError(err)
	suite.Require().NotNil(view.PersonData)
	suite.Require().NotNil(view.PersonData.Company)
	suite.Require().Empty(view.PersonData.Company.INN.String())
}

func (suite *ParseExtendedBillingAccountTestSuite) TestParsePersonWithCompanyStringINN() {
	const sourceJSON = `{
		"person": {
			"company": {
				"inn": "123456"
			}
		}
	}`

	var view ExtendedBillingAccountCamelCaseView
	err := json.Unmarshal([]byte(sourceJSON), &view)

	suite.Require().NoError(err)
	suite.Require().NotNil(view.PersonData)
	suite.Require().NotNil(view.PersonData.Company)
	suite.Require().Equal("123456", view.PersonData.Company.INN.String())
}

func (suite *ParseExtendedBillingAccountTestSuite) TestParsePersonWithCompanyBrokenStringINN() {
	const sourceJSON = `{
		"person": {
			"company": {
				"inn": "a-z"
			}
		}
	}`

	var view ExtendedBillingAccountCamelCaseView
	err := json.Unmarshal([]byte(sourceJSON), &view)

	suite.Require().NoError(err)
	suite.Require().Equal("a-z", view.PersonData.Company.INN.String())
}

type SchemelessIntTestSuite struct {
	suite.Suite
}

func (suite *SchemelessIntTestSuite) TestIntStringValue() {
	const sourceJSON = `{
		"string": "42"
	}`

	var testCase struct {
		Value SchemelessInt `json:"string"`
	}

	err := json.Unmarshal([]byte(sourceJSON), &testCase)
	suite.Require().NoError(err)
	suite.Require().Equal("42", testCase.Value.String())
}

func (suite *SchemelessIntTestSuite) TestZeroPaddedIntStringValue() {
	const sourceJSON = `{
		"string": "00042"
	}`

	var testCase struct {
		Value SchemelessInt `json:"string"`
	}

	err := json.Unmarshal([]byte(sourceJSON), &testCase)
	suite.Require().NoError(err)
	suite.Require().Equal("00042", testCase.Value.String())
}

func (suite *SchemelessIntTestSuite) TestAnyStringValue() {
	const sourceJSON = `{
		"string": "district 15"
	}`

	var testCase struct {
		Value SchemelessInt `json:"string"`
	}

	err := json.Unmarshal([]byte(sourceJSON), &testCase)
	suite.Require().NoError(err)
	suite.Require().Equal("district 15", testCase.Value.String())
}

func (suite *SchemelessIntTestSuite) TestAnyEscapedStringValue() {
	const sourceJSON = `{
		"string": "district\" 15"
	}`

	var testCase struct {
		Value SchemelessInt `json:"string"`
	}

	err := json.Unmarshal([]byte(sourceJSON), &testCase)
	suite.Require().NoError(err)
	suite.Require().Equal("district\" 15", testCase.Value.String())
}

func (suite *SchemelessIntTestSuite) TestIntValue() {
	const sourceJSON = `{
		"string": 100500
	}`

	var testCase struct {
		Value SchemelessInt `json:"string"`
	}

	err := json.Unmarshal([]byte(sourceJSON), &testCase)
	suite.Require().NoError(err)
	suite.Require().Equal("100500", testCase.Value.String())
}

func (suite *SchemelessIntTestSuite) TestUnsupportedtype() {
	const sourceJSON = `{
		"string": [100500]
	}`

	var testCase struct {
		Value SchemelessInt `json:"string"`
	}

	err := json.Unmarshal([]byte(sourceJSON), &testCase)
	suite.Require().Error(err)
	suite.Require().Empty(testCase)
}

func (suite *SchemelessIntTestSuite) TestNull() {
	const sourceJSON = `{
		"string": null
	}`

	var testCase struct {
		Value SchemelessInt `json:"string"`
	}

	err := json.Unmarshal([]byte(sourceJSON), &testCase)
	suite.Require().NoError(err)
	suite.Require().Empty(testCase.Value.String())
}
