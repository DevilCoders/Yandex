package services

import (
	"context"

	rpc_status "google.golang.org/genproto/googleapis/rpc/status"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/anypb"
	"google.golang.org/protobuf/types/known/emptypb"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/access"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

var _ billing_grpc.OperationServiceServer = &OperationService{}

type OperationService struct {
	client console.Client
}

func NewOperationService(client console.Client) *OperationService {
	return &OperationService{client: client}
}

func (s *OperationService) Get(ctx context.Context, request *billing_grpc.GetOperationRequest) (*operation.Operation, error) {

	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.GetOperationRequest{
		AuthData: console.AuthData{Token: token},
		ID:       request.OperationId,
	}
	consoleOperation, err := s.client.GetOperation(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	return FormatOperation(consoleOperation)
}

func FormatOperation(consoleOperation *console.OperationResponse) (*operation.Operation, error) {

	// convert metadata to pb.Any
	metadata, err := formatMetadata(consoleOperation.Metadata)
	if err != nil {
		return nil, err
	}

	// build model
	grpcOperation := &operation.Operation{
		Id:          consoleOperation.ID,
		Description: consoleOperation.Description,
		CreatedAt:   &timestamppb.Timestamp{Seconds: consoleOperation.CreatedAt},
		CreatedBy:   consoleOperation.CreatedBy,
		ModifiedAt:  &timestamppb.Timestamp{Seconds: consoleOperation.ModifiedAt},
		Done:        consoleOperation.Done,
		Metadata:    metadata,
	}

	err = formatResult(consoleOperation, grpcOperation)
	if err != nil {
		return nil, err
	}

	return grpcOperation, nil
}

func formatResult(consoleOperation *console.OperationResponse, grpcOperation *operation.Operation) error {
	if consoleOperation.Error != nil {
		grpcOperation.Result = &operation.Operation_Error{Error: &rpc_status.Status{Code: *consoleOperation.Error}}
	} else {
		response, err := formatResponse(*consoleOperation.Response)
		if err != nil {
			return err
		}
		grpcOperation.Result = &operation.Operation_Response{Response: response}
	}
	return nil
}

func formatMetadata(metadata interface{}) (*anypb.Any, error) {
	var grpcMetadata interface{}
	switch m := metadata.(type) {
	case *console.CreateBudgetMetadata:
		grpcMetadata = &billing_grpc.CreateBudgetMetadata{BudgetId: m.BudgetID}
	case *console.BindBillableObjectOperationMetadata:
		grpcMetadata = &billing_grpc.BindBillableObjectMetadata{BillableObjectId: m.BillableObjectID}
	case *console.CustomerMetadata:
		grpcMetadata = &billing_grpc.CustomerMetadata{
			ResellerId: m.ResellerID,
			CustomerId: m.CustomerID,
		}
	case *console.UpdateAccessBindingsMetadata:
		grpcMetadata = &access.UpdateAccessBindingsMetadata{ResourceId: m.BillingAccountID}
	default:
		return nil, errInternal
	}
	return convertToAny(grpcMetadata)
}

func formatResponse(response interface{}) (*anypb.Any, error) {
	var grpcResponse interface{}
	switch r := response.(type) {
	case *console.BudgetResponse:
		grpcResponse = mapConsoleBudgetToGrpcBudget(r)
	case *console.BillableObjectBindingResponse:
		grpcResponse = &billing_grpc.BillableObjectBinding{
			EffectiveTime: &timestamppb.Timestamp{Seconds: r.EffectiveTime},
			BillableObject: &billing_grpc.BillableObject{
				Id:   r.BillableObjectID,
				Type: r.BillableObjectType,
			},
		}
	case *console.Customer:
		grpcResponse = &billing_grpc.Customer{
			Id:               r.ID,
			BillingAccountId: r.BillingAccountID,
		}

	case *console.UpdateAccessBindingsResponse:
		grpcResponse = &emptypb.Empty{}
	default:
		return nil, errInternal
	}
	return convertToAny(grpcResponse)
}
