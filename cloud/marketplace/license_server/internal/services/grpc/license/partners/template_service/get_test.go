package templateservice

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/partners"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	license_model "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestGetGrpc(t *testing.T) {
	suite.Run(t, new(GetGrpcTestSuite))
}

type GetGrpcTestSuite struct {
	tests.BaseTestSuite

	service *templateService
}

func (suite *GetGrpcTestSuite) SetupTest() {
	suite.service = NewTemplateService(suite.Env)
}

func (suite *GetGrpcTestSuite) TestGetOk() {
	lt := &license_model.Template{
		ID:     "ID",
		State:  license_model.ActiveTemplateState,
		Period: model.DefaultPeriod(),
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.Require().NoError(err)

	ltRes, err := suite.service.Get(context.Background(), &partners.GetTemplateRequest{Id: lt.ID})

	suite.Require().NoError(err)
	suite.Require().NotNil(ltRes)
	suite.Require().Equal(lt.ID, ltRes.Id)
}

func (suite *GetGrpcTestSuite) TestGetError() {
	ltRes, err := suite.service.Get(context.Background(), &partners.GetTemplateRequest{Id: "NO_ID"})

	suite.Require().Error(err)
	suite.Require().Nil(ltRes)
	errGrpc := status.Convert(err)
	suite.Require().Equal(int32(codes.NotFound), errGrpc.Proto().Code)
}
