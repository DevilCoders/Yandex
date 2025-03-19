package templateversionservice

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/partners"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	license_model "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestCreateGrpc(t *testing.T) {
	suite.Run(t, new(CreateGrpcTestSuite))
}

type CreateGrpcTestSuite struct {
	tests.BaseTestSuite

	service *templateVersionService
}

func (suite *CreateGrpcTestSuite) SetupTest() {
	suite.service = NewTemplateVersionService(suite.Env)
}

func (suite *CreateGrpcTestSuite) TestCreatingOk() {
	lt := &license_model.Template{
		ID:     "ID",
		State:  license_model.ActiveTemplateState,
		Period: model.DefaultPeriod(),
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.Require().NoError(err)

	resOp, err := suite.service.Create(context.Background(), &partners.CreateTemplateVersionRequest{
		TemplateId: lt.ID,
		Name:       "test-name",
		Prices:     &license.Price{CurrencyToValue: map[string]string{"RUB": "1000000"}},
		Period:     "10000_d",
	})

	suite.Require().NoError(err)

	suite.Require().NotNil(resOp)
	suite.Require().Equal(true, resOp.Done)

	meta := new(partners.CreateTemplateVersionMetadata)
	err = resOp.Metadata.UnmarshalTo(meta)
	suite.Require().NoError(err)
	suite.Require().NotNil(meta.TemplateVersionId)

	ltvGrpc := new(license.TemplateVersion)
	err = resOp.Result.(*operation.Operation_Response).Response.UnmarshalTo(ltvGrpc)

	suite.Require().NoError(err)

	ltv, err := suite.Env.Adapters().DB().GetLicenseTemplateVersionByID(context.Background(), ltvGrpc.Id, nil)

	suite.Require().NoError(err)
	suite.Require().NotNil(ltv)
}
