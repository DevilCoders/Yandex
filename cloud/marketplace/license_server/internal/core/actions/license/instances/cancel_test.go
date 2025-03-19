package instances

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestCancelAction(t *testing.T) {
	suite.Run(t, new(CancelActionTestSuite))
}

type CancelActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *CancelActionTestSuite) TestCancelOk() {
	li := &license.Instance{
		ID:    "id",
		State: license.ActiveInstanceState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li, nil)
	suite.Require().NoError(err)

	cancelInstance, err := NewCancelAction(suite.Env).Do(context.Background(), CancelParams{ID: li.ID})
	suite.Require().NoError(err)
	suite.Require().NotNil(cancelInstance.Instance)
	suite.Require().Equal(license.CancelledInstanceState, cancelInstance.Instance.State)
}

func (suite *CancelActionTestSuite) TestCancelError() {
	cancelInstance, err := NewCancelAction(suite.Env).Do(context.Background(), CancelParams{ID: "no_id"})
	suite.Require().Error(err)
	suite.Require().Nil(cancelInstance)
}
