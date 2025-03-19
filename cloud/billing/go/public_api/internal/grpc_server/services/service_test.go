package services

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server/services/mocks"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
)

type serviceMocks struct {
	client *mocks.Client
}

func (suite *serviceMocks) SetupTest() {
	suite.client = &mocks.Client{}
}

type serviceTestSuite struct {
	suite.Suite
	serviceMocks

	serviceService billing_grpc.ServiceServiceServer
}

func TestService(t *testing.T) {
	suite.Run(t, new(serviceTestSuite))
}

func (suite *serviceTestSuite) SetupTest() {
	suite.serviceMocks.SetupTest()
	suite.serviceService = NewServiceService(suite.serviceMocks.client)
}

func mapConsoleServiceResponse(service *billing_grpc.Service) *console.ServiceResponse {
	return &console.ServiceResponse{
		ID:          service.Id,
		Name:        "some name",
		Description: service.Name,
	}
}

func (suite *serviceTestSuite) TestGetServiceError() {
	suite.client.On("GetService", mock.Anything, mock.Anything).Return(nil, console.ErrServerInternal)

	req := &billing_grpc.GetServiceRequest{
		Id: generateID(),
	}
	_, err := suite.serviceService.Get(getContextWithAuth(), req)

	suite.Require().Error(err)

	sts, ok := status.FromError(err)
	suite.Require().True(ok)
	suite.Require().Equal(sts.Code(), codes.Internal)
}

func (suite *serviceTestSuite) TestGetServiceOK() {
	expectedService := &billing_grpc.Service{
		Id:          "foo",
		Name:        "bar",
		Description: "",
	}

	suite.client.On("GetService", mock.Anything, mock.Anything).Return(mapConsoleServiceResponse(expectedService), nil)

	req := &billing_grpc.GetServiceRequest{
		Id: generateID(),
	}
	service, err := suite.serviceService.Get(getContextWithAuth(), req)

	suite.Require().NoError(err)
	suite.Require().Equal(service, expectedService)
}
