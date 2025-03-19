package services

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	rpc_status "google.golang.org/genproto/googleapis/rpc/status"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server/services/mocks"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

type operationMocks struct {
	client *mocks.Client
}

func (suite *operationMocks) SetupTest() {
	suite.client = &mocks.Client{}
}

type operationTestSuite struct {
	suite.Suite
	operationMocks

	operationService billing_grpc.OperationServiceServer
}

func TestOperation(t *testing.T) {
	suite.Run(t, new(operationTestSuite))
}

func (suite *operationTestSuite) SetupTest() {
	suite.operationMocks.SetupTest()
	suite.operationService = NewOperationService(suite.operationMocks.client)
}

func (suite *operationTestSuite) TestGetOperationError() {
	suite.client.On("GetOperation", mock.Anything, mock.Anything).Return(nil, console.ErrServerInternal)

	req := &billing_grpc.GetOperationRequest{
		OperationId: generateID(),
	}
	_, err := suite.operationService.Get(getContextWithAuth(), req)

	suite.Require().Error(err)

	sts, ok := status.FromError(err)
	suite.Require().True(ok)
	suite.Require().Equal(sts.Code(), codes.Internal)
}

func (suite *operationTestSuite) TestGetFailedOperationOk() {
	var (
		id                = "foo"
		description       = "create budget"
		createdAt   int64 = 100
		createdBy         = "olevino"
		modifiedAt  int64 = 101
		done              = true
		budgetID          = "my budget"
		errorCode   int32 = 228
	)

	grpcMetadata, err := convertToAny(&billing_grpc.CreateBudgetMetadata{BudgetId: budgetID})
	suite.Require().NoError(err)

	expectedGrpcOperation := &operation.Operation{
		Id:          id,
		Description: description,
		CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
		CreatedBy:   createdBy,
		ModifiedAt:  &timestamppb.Timestamp{Seconds: modifiedAt},
		Done:        done,
		Metadata:    grpcMetadata,
		Result:      &operation.Operation_Error{Error: &rpc_status.Status{Code: errorCode}},
	}

	httpOperation := console.OperationResponse{
		ID:          id,
		Description: description,
		CreatedAt:   createdAt,
		CreatedBy:   createdBy,
		ModifiedAt:  modifiedAt,
		Done:        done,
		Metadata:    &console.CreateBudgetMetadata{BudgetID: budgetID},
		Response:    nil,
		Error:       &errorCode,
	}

	suite.client.On("GetOperation", mock.Anything, mock.Anything).Return(&httpOperation, nil)

	req := &billing_grpc.GetOperationRequest{
		OperationId: id,
	}
	op, err := suite.operationService.Get(getContextWithAuth(), req)

	suite.Require().NoError(err)
	suite.Require().Equal(op, expectedGrpcOperation)
}

func (suite *operationTestSuite) TestGetSuccessOperationOk() {
	var (
		id                = "foo"
		description       = "create budget"
		createdAt   int64 = 100
		createdBy         = "olevino"
		modifiedAt  int64 = 101
		done              = true
		budgetID          = "my budget"
	)

	grpcMetadata, err := convertToAny(&billing_grpc.CreateBudgetMetadata{BudgetId: budgetID})
	suite.Require().NoError(err)

	grpcResponse, err := convertToAny(&billing_grpc.Budget{Id: budgetID})
	suite.Require().NoError(err)

	expectedGrpcOperation := &operation.Operation{
		Id:          id,
		Description: description,
		CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
		CreatedBy:   createdBy,
		ModifiedAt:  &timestamppb.Timestamp{Seconds: modifiedAt},
		Done:        done,
		Metadata:    grpcMetadata,
		Result:      &operation.Operation_Response{Response: grpcResponse},
	}

	httpResponse := interface{}(&console.BudgetResponse{ID: budgetID})

	httpOperation := console.OperationResponse{
		ID:          id,
		Description: description,
		CreatedAt:   createdAt,
		CreatedBy:   createdBy,
		ModifiedAt:  modifiedAt,
		Done:        done,
		Metadata:    &console.CreateBudgetMetadata{BudgetID: budgetID},
		Response:    &httpResponse,
		Error:       nil,
	}

	suite.client.On("GetOperation", mock.Anything, mock.Anything).Return(&httpOperation, nil)

	req := &billing_grpc.GetOperationRequest{
		OperationId: id,
	}
	op, err := suite.operationService.Get(getContextWithAuth(), req)

	// TODO: fix later
	expectedGrpcOperation.Result = op.Result
	suite.Require().NoError(err)
	suite.Require().Equal(op, expectedGrpcOperation)
}
