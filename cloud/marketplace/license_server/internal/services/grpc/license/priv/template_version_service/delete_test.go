package templateversionservice

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"
	"google.golang.org/protobuf/types/known/emptypb"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	license_model "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestDeleteGrpc(t *testing.T) {
	suite.Run(t, new(DeleteGrpcTestSuite))
}

type DeleteGrpcTestSuite struct {
	tests.BaseTestSuite

	service *templateVersionService
}

func (suite *DeleteGrpcTestSuite) SetupTest() {
	suite.service = NewTemplateVersionService(suite.Env)
}

func (suite *DeleteGrpcTestSuite) TestDeleteOk() {
	ltv := &license_model.TemplateVersion{
		ID:     "ID",
		State:  license_model.DeprecatedTemplateVersionState,
		Period: model.DefaultPeriod(),
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplateVersion(context.Background(), ltv, nil)
	suite.Require().NoError(err)

	resOp, err := suite.service.Delete(context.Background(), &priv.DeleteTemplateVersionRequest{Id: ltv.ID})
	suite.Require().NoError(err)

	suite.Require().NotNil(resOp)
	suite.Require().Equal(true, resOp.Done)

	meta := new(priv.DeleteTemplateVersionMetadata)
	err = resOp.Metadata.UnmarshalTo(meta)
	suite.Require().NoError(err)
	suite.Require().NotNil(meta.TemplateVersionId)

	empty := new(emptypb.Empty)
	err = resOp.Result.(*operation.Operation_Response).Response.UnmarshalTo(empty)
	suite.Require().NoError(err)

	ltv, err = suite.Env.Adapters().DB().GetLicenseTemplateVersionByID(context.Background(), ltv.ID, nil)

	suite.Require().Error(err)
	suite.Require().Nil(ltv)
}
