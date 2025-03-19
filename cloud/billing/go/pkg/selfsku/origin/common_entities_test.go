package origin

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type unitsTestSuite struct {
	suite.Suite
	value UnitConversion
}

func TestUnits(t *testing.T) {
	suite.Run(t, new(unitsTestSuite))
}

func (suite *unitsTestSuite) SetupTest() {
	suite.value = UnitConversion{
		SrcUnit: "source unit",
		DstUnit: "dest_unit",
		Factor:  decimal.Must(decimal.FromInt64(42)),
	}
}

func (suite *unitsTestSuite) TestValid() {
	err := suite.value.Valid()
	suite.NoError(err)
}

func (suite *unitsTestSuite) TestEmpty() {
	{
		checkVal := suite.value
		checkVal.SrcUnit = ""
		err := checkVal.Valid()
		suite.Error(err)
	}
	{
		checkVal := suite.value
		checkVal.DstUnit = ""
		err := checkVal.Valid()
		suite.Error(err)
	}
	{
		checkVal := suite.value
		checkVal.Factor = decimal.Decimal128{}
		err := checkVal.Valid()
		suite.Error(err)
	}
}

func (suite *unitsTestSuite) TestSameUnit() {
	checkVal := suite.value
	checkVal.SrcUnit = checkVal.DstUnit
	err := checkVal.Valid()
	suite.Error(err)
}

func (suite *unitsTestSuite) TestNegative() {
	checkVal := suite.value
	checkVal.Factor = checkVal.Factor.Neg()
	err := checkVal.Valid()
	suite.Error(err)
}

type servicesTestSuite struct {
	suite.Suite
	value Service
}

func TestServices(t *testing.T) {
	suite.Run(t, new(servicesTestSuite))
}

func (suite *servicesTestSuite) SetupTest() {
	suite.value = Service{
		ID:          "00000000000000000",
		Name:        "valid.service-name_42",
		Description: "Some Test Service",
		Group:       "Test",
	}
}

func (suite *servicesTestSuite) TestValid() {
	err := suite.value.Valid()
	suite.NoError(err)
}

func (suite *servicesTestSuite) TestInvalidID() {
	cases := []string{
		"", "000", "A0000000000000000", "z0000000000000000",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			checkVal := suite.value
			checkVal.ID = c
			err := checkVal.Valid()
			suite.Error(err)
		})
	}
}

func (suite *servicesTestSuite) TestInvalidName() {
	cases := []string{
		"", "/", "A",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			checkVal := suite.value
			checkVal.Name = c
			err := checkVal.Valid()
			suite.Error(err)
		})
	}
}

func (suite *servicesTestSuite) TestEmptyGroup() {
	{
		checkVal := suite.value
		checkVal.Group = ""
		err := checkVal.Valid()
		suite.Error(err)
	}
}

type tagsTestSuite struct {
	suite.Suite
	value TagChecks
}

func TestTags(t *testing.T) {
	suite.Run(t, new(tagsTestSuite))
}

func (suite *tagsTestSuite) SetupTest() {
	suite.initData()
}

func (suite *tagsTestSuite) initData() {
	suite.value = TagChecks{
		"valid.schema": {
			Required: []string{"tag1", "tag2"},
			Optional: []string{"optional_tag"},
		},
	}
}

func (suite *tagsTestSuite) TestValid() {
	err := suite.value.Valid()
	suite.NoError(err)
}

func (suite *tagsTestSuite) TestInvalidName() {
	cases := []string{
		"", "Invalid",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			suite.initData()
			suite.value[c] = suite.value["valid.schema"]
			err := suite.value.Valid()
			suite.Error(err)
		})
	}
}

func (suite *tagsTestSuite) TestTagsDuplicates() {
	{
		checkVal := suite.value
		tc := checkVal["valid.schema"]
		tc.Required = append(tc.Required, tc.Required[0])
		checkVal["for-check"] = tc
		err := checkVal.Valid()
		suite.Error(err)
	}
	{
		checkVal := suite.value
		tc := checkVal["valid.schema"]
		tc.Optional = append(tc.Optional, tc.Optional[0])
		checkVal["for-check"] = tc
		err := checkVal.Valid()
		suite.Error(err)
	}
	{
		checkVal := suite.value
		tc := checkVal["valid.schema"]
		tc.Required = append(tc.Required, tc.Optional[0])
		checkVal["for-check"] = tc
		err := checkVal.Valid()
		suite.Error(err)
	}
}

func (suite *tagsTestSuite) TestTagsInvalidValues() {
	{
		checkVal := suite.value
		tc := checkVal["valid.schema"]
		tc.Required = append(tc.Required, "")
		checkVal["for-check"] = tc
		err := checkVal.Valid()
		suite.Error(err)
	}
	{
		checkVal := suite.value
		tc := checkVal["valid.schema"]
		tc.Optional = append(tc.Optional, "")
		checkVal["for-check"] = tc
		err := checkVal.Valid()
		suite.Error(err)
	}
}
