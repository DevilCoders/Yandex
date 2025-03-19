package services

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	"google.golang.org/protobuf/types/known/anypb"
	"google.golang.org/protobuf/types/known/emptypb"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server/services/mocks"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/access"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

type accessMocks struct {
	client *mocks.Client
}

func (suite *accessMocks) SetupTest() {
	suite.client = &mocks.Client{}
}

type accessTestSuite struct {
	suite.Suite
	accessMocks

	billingAccountService billing_grpc.BillingAccountServiceServer
}

func TestAccess(t *testing.T) {
	suite.Run(t, new(billableObjectsTestSuite))
}

func (suite *accessTestSuite) SetupTest() {
	suite.accessMocks.SetupTest()
	suite.billingAccountService = NewBillingAccountService(suite.accessMocks.client)
}

func (suite *billableObjectsTestSuite) TestListAccessBindings() {
	billingAccountID := generateID()

	context := getContextWithAuth()
	expectedToken, _ := getToken(context)

	consoleAccessBinding1 := console.AccessBinding{
		RoleID: "viewer",
		Subject: console.Subject{
			ID:   generateID(),
			Type: "userAccount",
		},
	}
	consoleAccessBinding2 := console.AccessBinding{
		RoleID: "editor",
		Subject: console.Subject{
			ID:   generateID(),
			Type: "serviceAccount",
		},
	}

	suite.client.
		On("ListAccessBindings",
			mock.Anything, // ctx
			&console.ListAccessBindingsRequest{
				BillingAccountID: billingAccountID,
			},
			&console.AuthData{Token: expectedToken},
		).
		Return(
			&console.ListAccessBindingsResponse{
				AccessBindings: []console.AccessBinding{consoleAccessBinding1, consoleAccessBinding2},
				NextPageToken:  nil,
			}, nil)

	expectedGrpc := &access.ListAccessBindingsResponse{
		NextPageToken: "",
		AccessBindings: []*access.AccessBinding{
			{
				RoleId: consoleAccessBinding1.RoleID,
				Subject: &access.Subject{
					Id:   consoleAccessBinding1.Subject.ID,
					Type: consoleAccessBinding1.Subject.Type,
				},
			},
			{
				RoleId: consoleAccessBinding2.RoleID,
				Subject: &access.Subject{
					Id:   consoleAccessBinding2.Subject.ID,
					Type: consoleAccessBinding2.Subject.Type,
				},
			},
		},
	}

	req := &access.ListAccessBindingsRequest{
		ResourceId: billingAccountID,
	}
	response, err := suite.billingAccountService.ListAccessBindings(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response, expectedGrpc)
}

func (suite *billableObjectsTestSuite) TestListAccessBindingsNextPageToken() {
	billingAccountID := generateID()
	expectedNextPageToken := generateID()

	context := getContextWithAuth()
	expectedToken, _ := getToken(context)

	suite.client.
		On("ListAccessBindings",
			mock.Anything, // ctx
			&console.ListAccessBindingsRequest{
				BillingAccountID: billingAccountID,
			},
			&console.AuthData{Token: expectedToken},
		).
		Return(
			&console.ListAccessBindingsResponse{
				AccessBindings: []console.AccessBinding{},
				NextPageToken:  &expectedNextPageToken,
			}, nil)

	req := &access.ListAccessBindingsRequest{
		ResourceId: billingAccountID,
	}
	response, err := suite.billingAccountService.ListAccessBindings(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response.NextPageToken, expectedNextPageToken)
}

func (suite *billableObjectsTestSuite) TestListAccessBindingsPageTokenAndPageSize() {
	billingAccountID := generateID()
	expectedPageToken := generateID()
	expectedPageSize := int64(100)

	context := getContextWithAuth()
	expectedToken, _ := getToken(context)

	suite.client.
		On("ListAccessBindings",
			mock.Anything, // ctx
			&console.ListAccessBindingsRequest{
				BillingAccountID: billingAccountID,
				PageSize:         &expectedPageSize,
				PageToken:        &expectedPageToken,
			},
			&console.AuthData{Token: expectedToken},
		).
		Return(
			&console.ListAccessBindingsResponse{
				AccessBindings: []console.AccessBinding{},
				NextPageToken:  nil,
			}, nil)

	req := &access.ListAccessBindingsRequest{
		ResourceId: billingAccountID,
		PageToken:  expectedPageToken,
		PageSize:   expectedPageSize,
	}
	_, err := suite.billingAccountService.ListAccessBindings(context, req)

	suite.Require().NoError(err)
}

func (suite *billableObjectsTestSuite) TestUpdateAccessBindingsOk() {
	var (
		billingAccountID = generateID()
		subjectID1       = generateID()
		subjectID2       = generateID()
	)

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	expectedConsoleRequest := &console.UpdateAccessBindingsRequest{
		BillingAccountID: billingAccountID,
		AccessBindingsDeltas: []console.AccessBindingDelta{
			{
				Action: "add",
				AccessBinding: console.AccessBinding{
					RoleID: "viewer",
					Subject: console.Subject{
						ID:   subjectID1,
						Type: "userAccount",
					},
				},
			},
			{
				Action: "remove",
				AccessBinding: console.AccessBinding{
					RoleID: "editor",
					Subject: console.Subject{
						ID:   subjectID2,
						Type: "serviceAccount",
					},
				},
			},
		},
	}

	consoleResponse := interface{}(&console.UpdateAccessBindingsResponse{})
	consoleOperation := console.OperationResponse{
		ID:          generateID(),
		Description: "foo bar",
		CreatedAt:   int64(100),
		CreatedBy:   "foo user",
		ModifiedAt:  int64(100),
		Done:        true,
		Metadata:    &console.UpdateAccessBindingsMetadata{BillingAccountID: billingAccountID},
		Response:    &consoleResponse,
		Error:       nil,
	}

	suite.client.On("UpdateAccessBindings", mock.Anything, expectedConsoleRequest, &console.AuthData{Token: expectedToken}).Return(&consoleOperation, nil)

	grpcMetadata, err := anypb.New(&access.UpdateAccessBindingsMetadata{ResourceId: billingAccountID})
	suite.Require().NoError(err)
	grpcResponse, err := anypb.New(&emptypb.Empty{})
	suite.Require().NoError(err)

	expectedGrpcOperation := &operation.Operation{
		Id:          consoleOperation.ID,
		Description: consoleOperation.Description,
		CreatedAt:   &timestamppb.Timestamp{Seconds: consoleOperation.CreatedAt},
		CreatedBy:   consoleOperation.CreatedBy,
		ModifiedAt:  &timestamppb.Timestamp{Seconds: consoleOperation.ModifiedAt},
		Done:        consoleOperation.Done,
		Metadata:    grpcMetadata,
		Result:      &operation.Operation_Response{Response: grpcResponse},
	}

	grpcRequest := &access.UpdateAccessBindingsRequest{
		ResourceId: billingAccountID,
		AccessBindingDeltas: []*access.AccessBindingDelta{
			{
				Action: access.AccessBindingAction_ADD,
				AccessBinding: &access.AccessBinding{
					RoleId: "viewer",
					Subject: &access.Subject{
						Id:   subjectID1,
						Type: "userAccount",
					},
				},
			},
			{
				Action: access.AccessBindingAction_REMOVE,
				AccessBinding: &access.AccessBinding{
					RoleId: "editor",
					Subject: &access.Subject{
						Id:   subjectID2,
						Type: "serviceAccount",
					},
				},
			},
		},
	}
	op, err := suite.billingAccountService.UpdateAccessBindings(context, grpcRequest)

	suite.Require().NoError(err)
	suite.Require().Equal(op, expectedGrpcOperation)
}
