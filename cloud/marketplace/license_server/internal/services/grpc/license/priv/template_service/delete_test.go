package templateservice

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc/codes"

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

	service *templateService
}

func (suite *DeleteGrpcTestSuite) SetupTest() {
	suite.service = NewTemplateService(suite.Env)
}

func (suite *DeleteGrpcTestSuite) TestDeleteOk() {
	lt := &license_model.Template{
		ID:     "ID",
		State:  license_model.ActiveTemplateState,
		Period: model.DefaultPeriod(),
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.Require().NoError(err)

	resOp, err := suite.service.Delete(context.Background(), &priv.DeleteTemplateRequest{Id: lt.ID})

	suite.Require().NoError(err)
	suite.Require().NotNil(resOp)
	suite.Require().Equal(true, resOp.Done)

	lt, err = suite.Env.Adapters().DB().GetLicenseTemplateByID(context.Background(), lt.ID, nil)

	suite.Require().Error(err)
	suite.Require().Nil(lt)

	op, err := suite.Env.Adapters().DB().GetOperation(context.Background(), resOp.Id, nil)

	suite.Require().NoError(err)

	opProto, err := op.Proto()
	suite.Require().NoError(err)
	suite.Require().Equal(resOp.Id, opProto.Id)
}

func (suite *DeleteGrpcTestSuite) TestDeleteError() {
	resOp, err := suite.service.Delete(context.Background(), &priv.DeleteTemplateRequest{Id: "NO_ID"})

	suite.Require().NoError(err)
	suite.Require().NotNil(resOp)
	suite.Require().Equal(true, resOp.Done)

	opErr, ok := resOp.Result.(*operation.Operation_Error)
	suite.Require().Equal(true, ok)
	suite.Require().Equal(int32(codes.NotFound), opErr.Error.Code)
}
