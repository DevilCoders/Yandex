package ydb

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestLicensesTemplateVersions(t *testing.T) {
	suite.Run(t, new(LicenseTemplateVersionsTestSuite))
}

type LicenseTemplateVersionsTestSuite struct {
	BaseYDBTestSuite
}

func (suite *LicenseTemplateVersionsTestSuite) SetupSuite() {
	suite.BaseYDBTestSuite.SetupSuite()
}

func (suite *LicenseTemplateVersionsTestSuite) TestGetByIDOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	err := suite.provider.UpsertLicenseTemplateVersion(ctx, &LicenseTemplateVersion{ID: "id1"}, nil)
	suite.Require().NoError(err)

	ltv, err := suite.provider.GetLicenseTemplateVersionByID(ctx, "id1", nil)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(ltv)
	suite.Require().Equal("id1", ltv.ID)
}

func (suite *LicenseTemplateVersionsTestSuite) TestNotFound() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	requestID := "idNotExist"

	result, err := suite.provider.GetLicenseTemplateVersionByID(ctx, requestID, nil)

	suite.Require().Error(err)
	suite.Require().Nil(result)
}

func (suite *LicenseTemplateVersionsTestSuite) TestUpsertOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	err := suite.provider.UpsertLicenseTemplateVersion(ctx, &LicenseTemplateVersion{ID: "id1"}, nil)
	suite.Require().NoError(err)

	ltv, err := suite.provider.GetLicenseTemplateVersionByID(ctx, "id1", nil)
	suite.Require().NoError(err)

	ltv.Period = "30_d"

	err = suite.provider.UpsertLicenseTemplateVersion(ctx, ltv, nil)
	suite.Require().NoError(err)

	ltvUpserted, err := suite.provider.GetLicenseTemplateVersionByID(ctx, ltv.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(ltvUpserted)
	suite.Require().Equal(ltv.Period, ltvUpserted.Period)
}

func (suite *LicenseTemplateVersionsTestSuite) TestGetPendingByLicenseTemplateIDOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	ltv := &LicenseTemplateVersion{ID: "id1", TemplateID: "license_template_id", State: "pending"}

	err := suite.provider.UpsertLicenseTemplateVersion(ctx, ltv, nil)
	suite.Require().NoError(err)

	pendingLtv, err := suite.provider.GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx, ltv.TemplateID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(pendingLtv)
	suite.Require().Equal("pending", pendingLtv.State)
}
