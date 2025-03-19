package ydb

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

func TestLicensesLocks(t *testing.T) {
	suite.Run(t, new(LicenseLocksTestSuite))
}

type LicenseLocksTestSuite struct {
	BaseYDBTestSuite
}

func (suite *LicenseLocksTestSuite) SetupSuite() {
	suite.BaseYDBTestSuite.SetupSuite()
}

func (suite *LicenseLocksTestSuite) TestGetByInstanceIDOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	llTest := &LicenseLock{
		ID:             "id",
		InstanceID:     "instance_id",
		ResourceLockID: "resource_lock_id",
		State:          "locked",
	}

	err := suite.provider.UpsertLicenseLock(ctx, llTest, nil)
	suite.Require().NoError(err)

	ll, err := suite.provider.GetLicenseLockByInstanceID(ctx, llTest.InstanceID, nil)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(ll)
	suite.Require().Equal(llTest.ID, ll.ID)
}

func (suite *LicenseLocksTestSuite) TestUnlockByLicenseInstanceIDsOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	instanceID := "license_instance_id"

	llTest1 := &LicenseLock{
		ID:             "id1",
		InstanceID:     instanceID,
		ResourceLockID: "resource_lock_id1",
		State:          "locked",
	}

	llTest2 := &LicenseLock{
		ID:             "id2",
		InstanceID:     instanceID,
		ResourceLockID: "resource_lock_id2",
		State:          "unlocked",
	}

	err := suite.provider.UpsertLicenseLock(ctx, llTest1, nil)
	suite.Require().NoError(err)

	err = suite.provider.UpsertLicenseLock(ctx, llTest2, nil)
	suite.Require().NoError(err)

	timeNow := ydb.UInt64Ts(utils.GetTimeNow())

	err = suite.provider.UnlockLicenseLocksByLicenseInstanceID(ctx, instanceID, timeNow, nil)

	suite.Require().NoError(err)
}

func (suite *LicenseLocksTestSuite) TestGetByInstanceIDNill() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	ll, err := suite.provider.GetLicenseLockByInstanceID(ctx, "nill", nil)

	suite.Require().Error(err)
	suite.Require().Empty(ll)
}

func (suite *LicenseLocksTestSuite) TestUpsertOk() {
	ctx, cancel := suite.newDBContext()
	defer cancel()

	llTest := &LicenseLock{
		InstanceID:     "instance_id",
		ResourceLockID: "resource_lock_id",
		State:          "locked",
	}

	err := suite.provider.UpsertLicenseLock(ctx, llTest, nil)
	suite.Require().NoError(err)

	ll, err := suite.provider.GetLicenseLockByInstanceID(ctx, llTest.InstanceID, nil)

	suite.Require().NoError(err)
	suite.Require().NotEmpty(ll)
}
