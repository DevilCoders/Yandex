package instances

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestDeleteAction(t *testing.T) {
	suite.Run(t, new(DeleteActionTestSuite))
}

type DeleteActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *DeleteActionTestSuite) TestDeleteOk() {
	li := &license.Instance{
		ID:    "id",
		State: license.ActiveInstanceState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li, nil)
	suite.Require().NoError(err)

	err = NewDeleteAction(suite.Env).Do(context.Background(), DeleteParams{ID: li.ID})
	suite.Require().NoError(err)

	li, err = suite.Env.Adapters().DB().GetLicenseInstanceByID(context.Background(), li.ID, nil)
	suite.Require().Error(err)
	suite.Require().Nil(li)
}

func (suite *DeleteActionTestSuite) TestDeleteError() {
	err := NewDeleteAction(suite.Env).Do(context.Background(), DeleteParams{ID: "no_id"})
	suite.Require().Error(err)
}
