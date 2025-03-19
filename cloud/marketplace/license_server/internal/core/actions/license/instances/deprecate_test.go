package instances

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestDeprecateAction(t *testing.T) {
	suite.Run(t, new(DeprecateActionTestSuite))
}

type DeprecateActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *DeprecateActionTestSuite) TestDeprecateOk() {
	li := &license.Instance{
		ID:    "id",
		State: license.ActiveInstanceState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseInstance(context.Background(), li, nil)
	suite.Require().NoError(err)

	res, err := NewDeprecateAction(suite.Env).Do(context.Background(), DeprecateParams{ID: li.ID})
	suite.Require().NoError(err)
	suite.Require().NotNil(res)

	li, err = suite.Env.Adapters().DB().GetLicenseInstanceByID(context.Background(), res.Instance.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(li)
	suite.Require().Equal(license.DeprecatedInstanceState, li.State)
}

func (suite *DeprecateActionTestSuite) TestDeprecateError() {
	deprecateInstance, err := NewDeprecateAction(suite.Env).Do(context.Background(), DeprecateParams{ID: "no_id"})
	suite.Require().Error(err)
	suite.Require().Nil(deprecateInstance)
}
