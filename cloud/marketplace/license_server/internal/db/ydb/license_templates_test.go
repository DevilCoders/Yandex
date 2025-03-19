package ydb

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestLicensesTemplates(t *testing.T) {
	suite.Run(t, new(LicenseTemplatesTestSuite))
}

type LicenseTemplatesTestSuite struct {
	BaseYDBTestSuite
}

func (suite *LicenseTemplatesTestSuite) SetupSuite() {
	suite.BaseYDBTestSuite.SetupSuite()
}

func (suite *LicenseTemplatesTestSuite) TestGetByIDOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	err := suite.provider.UpsertLicenseTemplate(ctx, &LicenseTemplate{ID: "id1", TemplateVersionID: "version_id"}, nil)
	suite.Require().NoError(err)

	lt, err := suite.provider.GetLicenseTemplateByID(ctx, "id1", nil)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(lt)
	suite.Require().Equal("id1", lt.ID)
	suite.Require().Equal("version_id", string(lt.TemplateVersionID))
}

func (suite *LicenseTemplatesTestSuite) TestListByTariffIDOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	err := suite.provider.UpsertLicenseTemplate(ctx, &LicenseTemplate{
		ID:                "id1",
		TemplateVersionID: "version_id",
		PublisherID:       "publisher_id",
		ProductID:         "product_id",
		TariffID:          "tariff_id",
	}, nil)
	suite.Require().NoError(err)
	err = suite.provider.UpsertLicenseTemplate(ctx, &LicenseTemplate{
		ID:                "id2",
		TemplateVersionID: "version_id2",
		PublisherID:       "publisher_id",
		ProductID:         "product_id",
		TariffID:          "tariff_id",
	}, nil)
	suite.Require().NoError(err)

	lts, err := suite.provider.ListLicenseTemplatesByTariffID(ctx, "publisher_id", "product_id", "tariff_id", nil)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(lts)

	ids := make([]string, 0, len(lts))
	for i := range lts {
		ids = append(ids, lts[i].ID)
	}
	suite.Require().Contains(ids, "id1")
	suite.Require().Contains(ids, "id2")
}

func (suite *LicenseTemplatesTestSuite) TestUpsertOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	err := suite.provider.UpsertLicenseTemplate(ctx, &LicenseTemplate{ID: "id1"}, nil)
	suite.Require().NoError(err)

	lt, err := suite.provider.GetLicenseTemplateByID(ctx, "id1", nil)
	suite.Require().NoError(err)

	lt.ProductID = "upserted_product_id"

	err = suite.provider.UpsertLicenseTemplate(ctx, lt, nil)
	suite.Require().NoError(err)

	ltUpserted, err := suite.provider.GetLicenseTemplateByID(ctx, lt.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(ltUpserted)
	suite.Require().Equal(lt.ProductID, ltUpserted.ProductID)
}
