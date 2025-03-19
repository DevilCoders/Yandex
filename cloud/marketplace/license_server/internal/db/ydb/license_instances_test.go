package ydb

import (
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

func TestLicenseInstances(t *testing.T) {
	suite.Run(t, new(LicenseInstancesTestSuite))
}

type LicenseInstancesTestSuite struct {
	BaseYDBTestSuite
}

func (suite *LicenseInstancesTestSuite) SetupSuite() {
	suite.BaseYDBTestSuite.SetupSuite()
}

func (suite *LicenseInstancesTestSuite) TestGetByIDOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	timeNow := utils.GetTimeNow()

	liTest := &LicenseInstance{
		ID:                "id",
		TemplateID:        "template_id",
		TemplateVersionID: "template_version_id",
		CloudID:           "cloud_id",
		Name:              "name",
		StartTime:         ydb.UInt64Ts(timeNow),
		EndTime:           ydb.UInt64Ts(timeNow),
		State:             "active",
	}

	err := suite.provider.UpsertLicenseInstance(ctx, liTest, nil)
	suite.Require().NoError(err)

	li, err := suite.provider.GetLicenseInstanceByID(ctx, liTest.ID, nil)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(li)
	suite.Require().Equal(liTest.ID, li.ID)
}

func (suite *LicenseInstancesTestSuite) TestNotFound() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	requestID := "idNotExist"

	result, err := suite.provider.GetLicenseInstanceByID(ctx, requestID, nil)

	suite.Require().Error(err)
	suite.Require().Nil(result)
}

func (suite *LicenseInstancesTestSuite) TestListOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	timeNow := utils.GetTimeNow()

	li1Test := &LicenseInstance{
		ID:                "id1",
		TemplateID:        "template_id",
		TemplateVersionID: "template_version_id",
		CloudID:           "cloud_id",
		Name:              "name",
		StartTime:         ydb.UInt64Ts(timeNow),
		EndTime:           ydb.UInt64Ts(timeNow),
		State:             "active",
	}

	err := suite.provider.UpsertLicenseInstance(ctx, li1Test, nil)
	suite.Require().NoError(err)

	li2Test := &LicenseInstance{
		ID:                "id2",
		TemplateID:        "template_id",
		TemplateVersionID: "template_version_id",
		CloudID:           "cloud_id",
		Name:              "name",
		State:             "active",
	}

	err = suite.provider.UpsertLicenseInstance(ctx, li2Test, nil)
	suite.Require().NoError(err)

	lis, err := suite.provider.GetLicenseInstancesByCloudID(ctx, li1Test.CloudID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(lis)

	lisValue := make([]LicenseInstance, 0, len(lis))
	for _, li := range lis {
		lisValue = append(lisValue, *li)
	}

	suite.Require().Contains(lisValue, *li1Test)
	suite.Require().Contains(lisValue, *li2Test)
}

func (suite *LicenseInstancesTestSuite) TestGetPendingInstancesOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	timeNow := utils.GetTimeNow()
	timeNowHourAgo := timeNow.Add(-time.Hour)

	li1Test := &LicenseInstance{
		ID:                "id1",
		TemplateID:        "template_id",
		TemplateVersionID: "template_version_id",
		CloudID:           "cloud_id",
		Name:              "name",
		StartTime:         ydb.UInt64Ts(timeNowHourAgo),
		EndTime:           ydb.UInt64Ts(timeNowHourAgo),
		State:             "pending",
	}

	err := suite.provider.UpsertLicenseInstance(ctx, li1Test, nil)
	suite.Require().NoError(err)

	li2Test := &LicenseInstance{
		ID:                "id2",
		TemplateID:        "template_id",
		TemplateVersionID: "template_version_id",
		CloudID:           "cloud_id",
		Name:              "name",
		StartTime:         ydb.UInt64Ts(timeNowHourAgo),
		EndTime:           ydb.UInt64Ts(timeNowHourAgo),
		State:             "pending",
	}

	err = suite.provider.UpsertLicenseInstance(ctx, li2Test, nil)
	suite.Require().NoError(err)

	lis, err := suite.provider.GetPendingInstances(ctx, ydb.UInt64Ts(timeNow), nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(lis)

	lisValue := make([]LicenseInstance, 0, len(lis))
	for _, li := range lis {
		lisValue = append(lisValue, *li)
	}

	suite.Require().Contains(lisValue, *li1Test)
	suite.Require().Contains(lisValue, *li2Test)
}

func (suite *LicenseInstancesTestSuite) TestGetRecreativeInstancesOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	timeNow := utils.GetTimeNow()
	timeNowHourAgo := timeNow.Add(-time.Hour)

	li1Test := &LicenseInstance{
		ID:                "id1",
		TemplateID:        "template_id",
		TemplateVersionID: "template_version_id",
		CloudID:           "cloud_id",
		Name:              "name",
		StartTime:         ydb.UInt64Ts(timeNowHourAgo),
		EndTime:           ydb.UInt64Ts(timeNowHourAgo),
		State:             "active",
	}

	err := suite.provider.UpsertLicenseInstance(ctx, li1Test, nil)
	suite.Require().NoError(err)

	li2Test := &LicenseInstance{
		ID:                "id2",
		TemplateID:        "template_id",
		TemplateVersionID: "template_version_id",
		CloudID:           "cloud_id",
		Name:              "name",
		StartTime:         ydb.UInt64Ts(timeNowHourAgo),
		EndTime:           ydb.UInt64Ts(timeNowHourAgo),
		State:             "active",
	}

	err = suite.provider.UpsertLicenseInstance(ctx, li2Test, nil)
	suite.Require().NoError(err)

	lis, err := suite.provider.GetRecreativeInstances(ctx, ydb.UInt64Ts(timeNow), nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(lis)

	lisValue := make([]LicenseInstance, 0, len(lis))
	for _, li := range lis {
		lisValue = append(lisValue, *li)
	}

	suite.Require().Contains(lisValue, *li1Test)
	suite.Require().Contains(lisValue, *li2Test)
}
