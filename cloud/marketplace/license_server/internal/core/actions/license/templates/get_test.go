package templates

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestGetAction(t *testing.T) {
	suite.Run(t, new(GetActionTestSuite))
}

type GetActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *GetActionTestSuite) TestGettingOk() {
	lt := &license.Template{
		ID:     "id",
		Period: model.DefaultPeriod(),
		State:  license.PendingTemplateState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.NoError(err)

	getResult, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{ID: lt.ID})

	suite.Require().NoError(err)
	suite.Require().NotNil(getResult)
	suite.Require().Equal(*lt, *getResult.Template)
}

func (suite *GetActionTestSuite) TestGettingError() {
	getResult, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{ID: "no_id"})

	suite.Require().Error(err)
	suite.Require().Nil(getResult)
}
