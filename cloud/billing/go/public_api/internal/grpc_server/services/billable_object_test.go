package services

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	"google.golang.org/protobuf/types/known/anypb"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server/services/mocks"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

type billableObjectsMocks struct {
	client *mocks.Client
}

func (suite *billableObjectsMocks) SetupTest() {
	suite.client = &mocks.Client{}
}

type billableObjectsTestSuite struct {
	suite.Suite
	billableObjectsMocks

	billingAccountService billing_grpc.BillingAccountServiceServer
}

func TestBillableObjects(t *testing.T) {
	suite.Run(t, new(billableObjectsTestSuite))
}

func (suite *billableObjectsTestSuite) SetupTest() {
	suite.billableObjectsMocks.SetupTest()
	suite.billingAccountService = NewBillingAccountService(suite.billableObjectsMocks.client)
}

func (suite *billableObjectsTestSuite) TestListBindingsFirstPage() {
	billingAccountID := generateID()
	nextPageToken := "next_page"
	cloudType := "cloud"
	billableObjectID1 := "binding1"
	billableObjectID2 := "binding2"

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	consoleBinding1 := console.BillableObjectBindingResponse{
		BillingAccountID:   billingAccountID,
		BillableObjectType: cloudType,
		BillableObjectID:   billableObjectID1,
		EffectiveTime:      100,
	}

	consoleBinding2 := console.BillableObjectBindingResponse{
		BillingAccountID:   billingAccountID,
		BillableObjectType: cloudType,
		BillableObjectID:   billableObjectID2,
		EffectiveTime:      200,
	}

	suite.client.
		On("ListBillableObjectBindings",
			mock.Anything, // ctx
			&console.ListBillableObjectBindingsRequest{ // request
				BillingAccountID: billingAccountID,
				AuthData:         console.AuthData{Token: expectedToken},
			},
		).
		Return(
			&console.BillableObjectBindingsListResponse{
				BillableObjectBindings: []console.BillableObjectBindingResponse{consoleBinding1, consoleBinding2},
				NextPageToken:          &nextPageToken,
			}, nil)

	expectedGrpc := &billing_grpc.ListBillableObjectBindingsResponse{
		NextPageToken: nextPageToken,
		BillableObjectBindings: []*billing_grpc.BillableObjectBinding{
			{
				EffectiveTime: &timestamppb.Timestamp{Seconds: 100},
				BillableObject: &billing_grpc.BillableObject{
					Id:   billableObjectID1,
					Type: cloudType,
				},
			}, {
				EffectiveTime: &timestamppb.Timestamp{Seconds: 200},
				BillableObject: &billing_grpc.BillableObject{
					Id:   billableObjectID2,
					Type: cloudType,
				},
			},
		},
	}

	req := &billing_grpc.ListBillableObjectBindingsRequest{
		BillingAccountId: billingAccountID,
	}
	response, err := suite.billingAccountService.ListBillableObjectBindings(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response, expectedGrpc)

}
func (suite *billableObjectsTestSuite) TestListBindingsLastPage() {
	billingAccountID := generateID()
	pageToken := "current_page"
	nextPageToken := ""
	cloudType := "cloud"
	billableObjectID1 := "binding1"
	billableObjectID2 := "binding2"

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	consoleBinding1 := console.BillableObjectBindingResponse{
		BillingAccountID:   billingAccountID,
		BillableObjectType: cloudType,
		BillableObjectID:   billableObjectID1,
		EffectiveTime:      100,
	}

	consoleBinding2 := console.BillableObjectBindingResponse{
		BillingAccountID:   billingAccountID,
		BillableObjectType: cloudType,
		BillableObjectID:   billableObjectID2,
		EffectiveTime:      200,
	}

	suite.client.
		On("ListBillableObjectBindings",
			mock.Anything, // ctx
			&console.ListBillableObjectBindingsRequest{ // request
				BillingAccountID: billingAccountID,
				PageToken:        &pageToken,
				AuthData:         console.AuthData{Token: expectedToken},
			},
		).
		Return(
			&console.BillableObjectBindingsListResponse{
				BillableObjectBindings: []console.BillableObjectBindingResponse{consoleBinding1, consoleBinding2},
			}, nil)

	expectedGrpc := &billing_grpc.ListBillableObjectBindingsResponse{
		NextPageToken: nextPageToken,
		BillableObjectBindings: []*billing_grpc.BillableObjectBinding{
			{
				EffectiveTime: &timestamppb.Timestamp{Seconds: 100},
				BillableObject: &billing_grpc.BillableObject{
					Id:   billableObjectID1,
					Type: cloudType,
				},
			}, {
				EffectiveTime: &timestamppb.Timestamp{Seconds: 200},
				BillableObject: &billing_grpc.BillableObject{
					Id:   billableObjectID2,
					Type: cloudType,
				},
			},
		},
	}

	req := &billing_grpc.ListBillableObjectBindingsRequest{
		BillingAccountId: billingAccountID,
		PageToken:        pageToken,
	}
	response, err := suite.billingAccountService.ListBillableObjectBindings(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response, expectedGrpc)

}

func (suite *billableObjectsTestSuite) TestListBindingsIntermediatePageWithPageSize() {
	billingAccountID := generateID()
	pageToken := "current_page"
	nextPageToken := "next_page"
	var pageSize int64 = 100
	cloudType := "cloud"
	billableObjectID1 := "binding1"
	billableObjectID2 := "binding2"

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	consoleBinding1 := console.BillableObjectBindingResponse{
		BillingAccountID:   billingAccountID,
		BillableObjectType: cloudType,
		BillableObjectID:   billableObjectID1,
		EffectiveTime:      100,
	}

	consoleBinding2 := console.BillableObjectBindingResponse{
		BillingAccountID:   billingAccountID,
		BillableObjectType: cloudType,
		BillableObjectID:   billableObjectID2,
		EffectiveTime:      200,
	}

	suite.client.
		On("ListBillableObjectBindings",
			mock.Anything, // ctx
			&console.ListBillableObjectBindingsRequest{ // request
				BillingAccountID: billingAccountID,
				PageToken:        &pageToken,
				PageSize:         &pageSize,
				AuthData:         console.AuthData{Token: expectedToken},
			},
		).
		Return(
			&console.BillableObjectBindingsListResponse{
				BillableObjectBindings: []console.BillableObjectBindingResponse{consoleBinding1, consoleBinding2},
				NextPageToken:          &nextPageToken,
			}, nil)

	expectedGrpc := &billing_grpc.ListBillableObjectBindingsResponse{
		NextPageToken: nextPageToken,
		BillableObjectBindings: []*billing_grpc.BillableObjectBinding{
			{
				EffectiveTime: &timestamppb.Timestamp{Seconds: 100},
				BillableObject: &billing_grpc.BillableObject{
					Id:   billableObjectID1,
					Type: cloudType,
				},
			}, {
				EffectiveTime: &timestamppb.Timestamp{Seconds: 200},
				BillableObject: &billing_grpc.BillableObject{
					Id:   billableObjectID2,
					Type: cloudType,
				},
			},
		},
	}

	req := &billing_grpc.ListBillableObjectBindingsRequest{
		BillingAccountId: billingAccountID,
		PageToken:        pageToken,
		PageSize:         pageSize,
	}
	response, err := suite.billingAccountService.ListBillableObjectBindings(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response, expectedGrpc)

}

func (suite *billableObjectsTestSuite) TestBindOk() {
	var (
		billingAccountID         = generateID()
		billableObjectID         = generateID()
		billableObjectType       = "cloud"
		effectiveTime      int64 = 100

		operationID                = generateID()
		operationDescription       = "billable_object_bind_to_billing_account"
		createdAt            int64 = 100
		createdBy                  = "olevino"
		modifiedAt           int64 = 101
		done                       = true
	)
	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	expectedHTTPRequest := &console.BindBillableObjectRequest{
		BillingAccountID:   billingAccountID,
		BillableObjectType: billableObjectType,
		BillableObjectID:   billableObjectID,
		AuthData:           console.AuthData{Token: expectedToken},
	}

	httpResponse := interface{}(&console.BillableObjectBindingResponse{
		EffectiveTime:      effectiveTime,
		BillableObjectID:   billableObjectID,
		BillableObjectType: billableObjectType,
		BillingAccountID:   billingAccountID,
	})

	httpOperation := console.OperationResponse{
		ID:          operationID,
		Description: operationDescription,
		CreatedAt:   createdAt,
		CreatedBy:   createdBy,
		ModifiedAt:  modifiedAt,
		Done:        done,
		Metadata:    &console.BindBillableObjectOperationMetadata{BillableObjectID: billableObjectID},
		Response:    &httpResponse,
		Error:       nil,
	}

	suite.client.On("BindBillableObject", mock.Anything, expectedHTTPRequest).Return(&httpOperation, nil)

	grpcMetadata, err := anypb.New(&billing_grpc.BindBillableObjectMetadata{BillableObjectId: billableObjectID})
	suite.Require().NoError(err)

	grpcResponse, err := anypb.New(&billing_grpc.BillableObjectBinding{
		EffectiveTime:  &timestamppb.Timestamp{Seconds: effectiveTime},
		BillableObject: &billing_grpc.BillableObject{Type: billableObjectType, Id: billableObjectID},
	})
	suite.Require().NoError(err)

	expectedGrpcOperation := &operation.Operation{
		Id:          operationID,
		Description: operationDescription,
		CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
		CreatedBy:   createdBy,
		ModifiedAt:  &timestamppb.Timestamp{Seconds: modifiedAt},
		Done:        done,
		Metadata:    grpcMetadata,
		Result:      &operation.Operation_Response{Response: grpcResponse},
	}

	grpcRequest := &billing_grpc.BindBillableObjectRequest{
		BillingAccountId: billingAccountID,
		BillableObject: &billing_grpc.BillableObject{
			Id:   billableObjectID,
			Type: billableObjectType,
		},
	}
	op, err := suite.billingAccountService.BindBillableObject(context, grpcRequest)

	suite.Require().NoError(err)
	suite.Require().Equal(op, expectedGrpcOperation)
}
