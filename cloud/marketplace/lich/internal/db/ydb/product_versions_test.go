package ydb

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

func TestGetProductVersions(t *testing.T) {
	suite.Run(t, new(GetProductVersionsTestSuite))
}

type GetProductVersionsTestSuite struct {
	BaseYDBTestSuite

	id1 ProductVersion
	id2 ProductVersion
	id3 ProductVersion
	id4 ProductVersion
	id5 ProductVersion
}

func (suite *GetProductVersionsTestSuite) SetupSuite() {
	suite.BaseYDBTestSuite.SetupSuite()

	suite.id1 = ProductVersion{
		ID:           "id1",
		Payload:      ydb.NewAnyJSON(`{"foo1": "bar1"}`),
		LicenseRules: ydb.NewAnyJSON("[1,2,3]"),
	}

	suite.id2 = ProductVersion{
		ID:           "id2",
		Payload:      ydb.NewAnyJSON(`{"foo2": "bar2"}`),
		LicenseRules: ydb.NewAnyJSON("[4,5,6]"),
	}

	suite.id3 = ProductVersion{
		ID:           "id3",
		Payload:      ydb.NewAnyJSON(`{"zoo": "foo"}`),
		LicenseRules: ydb.NewAnyJSON(`{"any": "object"}`),
	}

	suite.id4 = ProductVersion{
		ID:           "id4",
		Payload:      nil,
		LicenseRules: ydb.NewAnyJSON(`{"any": "object"}`),
	}

	suite.id5 = ProductVersion{
		ID:           "id5",
		Payload:      ydb.NewAnyJSON(`{"zoo": "foo"}`),
		LicenseRules: nil,
	}
}

func (suite *GetProductVersionsTestSuite) TestGetOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	requestIDs := []string{
		"id1",
		"id3",
	}

	result, err := suite.provider.Get(ctx, GetProductVersionsParams{
		IDs: requestIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotEmpty(result)

	suite.Require().Len(result, len(requestIDs), "unexpected results count %d", len(result))

	suite.Require().Contains(result, suite.id1)
	suite.Require().Contains(result, suite.id3)
	suite.Require().NotContains(result, suite.id2)
	suite.Require().NotContains(result, suite.id4)
	suite.Require().NotContains(result, suite.id5)
}

func (suite *GetProductVersionsTestSuite) TestSingleResultOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	requestIDs := []string{
		"id2",
	}

	result, err := suite.provider.Get(ctx, GetProductVersionsParams{
		IDs: requestIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotEmpty(result)

	suite.Require().Len(result, len(requestIDs), "unexpected results count %d", len(result))

	suite.Require().Contains(result, suite.id2)
	suite.Require().NotContains(result, suite.id1)
	suite.Require().NotContains(result, suite.id3)
	suite.Require().NotContains(result, suite.id4)
	suite.Require().NotContains(result, suite.id5)
}

func (suite *GetProductVersionsTestSuite) TestSingleResultWithNilPayloadOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	requestIDs := []string{
		"id4",
	}

	result, err := suite.provider.Get(ctx, GetProductVersionsParams{
		IDs: requestIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotEmpty(result)

	suite.Require().Len(result, len(requestIDs), "unexpected results count %d", len(result))

	suite.Require().Contains(result, suite.id4)

	suite.Require().NotContains(result, suite.id1)
	suite.Require().NotContains(result, suite.id2)
	suite.Require().NotContains(result, suite.id3)
	suite.Require().NotContains(result, suite.id5)
}

func (suite *GetProductVersionsTestSuite) TestSingleResultWithNilLicenseRulesOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	requestIDs := []string{
		"id5",
	}

	result, err := suite.provider.Get(ctx, GetProductVersionsParams{
		IDs: requestIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotEmpty(result)

	suite.Require().Len(result, len(requestIDs), "unexpected results count %d", len(result))

	suite.Require().Contains(result, suite.id5)

	suite.Require().NotContains(result, suite.id1)
	suite.Require().NotContains(result, suite.id2)
	suite.Require().NotContains(result, suite.id3)
	suite.Require().NotContains(result, suite.id4)
}

func (suite *GetProductVersionsTestSuite) TestNotFound() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	requestIDs := []string{
		"idNotExist",
	}

	result, err := suite.provider.Get(ctx, GetProductVersionsParams{
		IDs: requestIDs,
	})

	suite.Require().NoError(err)
	suite.Require().Empty(result)
}

func (suite *GetProductVersionsTestSuite) TestEmptyRequestNotFound() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	result, err := suite.provider.Get(ctx, GetProductVersionsParams{})

	suite.Require().NoError(err)
	suite.Require().Empty(result)
}
