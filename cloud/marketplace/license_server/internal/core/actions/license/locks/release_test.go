package locks

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestReleaseAction(t *testing.T) {
	suite.Run(t, new(ReleaseActionTestSuite))
}

type ReleaseActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *ReleaseActionTestSuite) TestUnlockOk() {
	instanceID := "instance_id"
	resourceLockID := "resource_lock_id"
	ll := &license.Lock{
		ID:             "custom_id",
		InstanceID:     instanceID,
		ResourceLockID: resourceLockID,
		State:          license.LockedLockState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseLock(context.Background(), ll, nil)
	suite.Require().NoError(err)

	lockResult, err := NewReleaseAction(suite.Env).Do(context.Background(), ReleaseParams{InstanceID: instanceID})

	suite.Require().NoError(err)
	suite.Require().NotNil(lockResult.Lock)

	lock, err := suite.Env.Adapters().DB().GetLicenseLockByInstanceID(context.Background(), instanceID, nil)
	suite.Require().Error(err)
	suite.Require().Nil(lock)
}

func (suite *ReleaseActionTestSuite) TestUnLockError() {
	lockResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{InstanceID: "no_id", ResourceLockID: "resource_lock_id"})
	suite.Require().Error(err)
	suite.Require().Nil(lockResult)
}
