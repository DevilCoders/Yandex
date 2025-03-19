package locks

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestGetAction(t *testing.T) {
	suite.Run(t, new(GetActionTestSuite))
}

type GetActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *GetActionTestSuite) TestGetOk() {
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

	lockResult, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{InstanceID: instanceID})

	suite.Require().NoError(err)
	suite.Require().NotNil(lockResult.Lock)
	suite.Require().Equal(instanceID, lockResult.Lock.InstanceID)
	suite.Require().Equal(resourceLockID, lockResult.Lock.ResourceLockID)
}

func (suite *GetActionTestSuite) TestGetError() {
	lockResult, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{InstanceID: "no_id"})
	suite.Require().Error(err)
	suite.Require().Nil(lockResult)
}
