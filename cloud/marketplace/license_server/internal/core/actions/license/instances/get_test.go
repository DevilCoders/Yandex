package instances

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
	li := &license.Instance{
		ID:    "id",
		State: license.ActiveInstanceState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li, nil)
	suite.Require().NoError(err)

	res, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{ID: li.ID})
	suite.Require().NoError(err)
	suite.Require().NotNil(res)
	suite.Require().Equal(li.ID, res.Instance.ID)
}

func (suite *GetActionTestSuite) TestGetError() {
	getInstance, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{ID: "no_id"})
	suite.Require().Error(err)
	suite.Require().Nil(getInstance)
}
