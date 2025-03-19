package locks

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
	utils_pkg "a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
)

func TestLockAction(t *testing.T) {
	suite.Run(t, new(CreateActionTestSuite))
}

type CreateActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *CreateActionTestSuite) TestLockOk() {
	li := &license.Instance{
		ID:    "id",
		State: license.ActiveInstanceState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li, nil)
	suite.Require().NoError(err)

	resourceLockID := "resource_lock_id"

	lockResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{InstanceID: li.ID, ResourceLockID: resourceLockID})

	suite.Require().NoError(err)
	suite.Require().NotNil(lockResult.Lock)

	lock, err := suite.Env.Adapters().DB().GetLicenseLockByInstanceID(context.Background(), li.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(lock)
	suite.Require().Equal(resourceLockID, lock.ResourceLockID)
}

func (suite *CreateActionTestSuite) TestLockAlreadyLockedError() {
	timeNow := utils_pkg.GetTimeNow()
	li := &license.Instance{
		ID:        "id",
		State:     license.ActiveInstanceState,
		StartTime: timeNow,
		EndTime:   timeNow.Add(time.Hour),
	}
	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li, nil)
	suite.Require().NoError(err)

	ll := &license.Lock{
		ID:             "custom_id",
		InstanceID:     li.ID,
		ResourceLockID: "custom_resource_id",
		State:          license.LockedLockState,
		StartTime:      timeNow,
		EndTime:        timeNow.Add(time.Hour),
	}
	err = suite.Env.Adapters().DB().UpsertLicenseLock(context.Background(), ll, nil)
	suite.Require().NoError(err)

	resourceLockID := "resource_lock_id"

	lockResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{InstanceID: li.ID, ResourceLockID: resourceLockID})

	suite.Require().Error(err)
	suite.Require().Nil(lockResult)
}

func (suite *CreateActionTestSuite) TestLockAfterUnlockOk() {
	li := &license.Instance{
		ID:    "id",
		State: license.ActiveInstanceState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li, nil)
	suite.Require().NoError(err)

	timeNow := utils_pkg.GetTimeNow()

	ll := &license.Lock{
		ID:             "custom_id",
		InstanceID:     li.ID,
		ResourceLockID: "custom_resource_id",
		State:          license.UnlockedLockState,
		StartTime:      timeNow,
		EndTime:        timeNow,
	}
	err = suite.Env.Adapters().DB().UpsertLicenseLock(context.Background(), ll, nil)
	suite.Require().NoError(err)

	resourceLockID := "resource_lock_id"

	lockResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{InstanceID: li.ID, ResourceLockID: resourceLockID})

	suite.Require().NoError(err)
	suite.Require().NotNil(lockResult.Lock)
	suite.Require().Equal(resourceLockID, lockResult.Lock.ResourceLockID)
}

func (suite *CreateActionTestSuite) TestLockError() {
	lockResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{InstanceID: "no_id", ResourceLockID: "resource_lock_id"})
	suite.Require().Error(err)
	suite.Require().Nil(lockResult)
}
