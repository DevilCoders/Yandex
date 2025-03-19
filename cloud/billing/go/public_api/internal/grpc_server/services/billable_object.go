package services

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

var _ billing_grpc.BillingAccountServiceServer = &BillingAccountService{}

func (b *BillingAccountService) ListBillableObjectBindings(ctx context.Context, request *billing_grpc.ListBillableObjectBindingsRequest) (*billing_grpc.ListBillableObjectBindingsResponse, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ListBillableObjectBindingsRequest{
		AuthData:         console.AuthData{Token: token},
		BillingAccountID: request.BillingAccountId,
	}

	if request.PageToken != "" {
		consoleReq.PageToken = &request.PageToken
	}

	if request.PageSize != 0 {
		consoleReq.PageSize = &request.PageSize
	}

	bindings, err := b.client.ListBillableObjectBindings(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	var grpcBindings []*billing_grpc.BillableObjectBinding
	for _, binding := range bindings.BillableObjectBindings {
		billableObject := billing_grpc.BillableObject{
			Id:   binding.BillableObjectID,
			Type: binding.BillableObjectType,
		}
		grpcBindings = append(grpcBindings, &billing_grpc.BillableObjectBinding{
			BillableObject: &billableObject,
			EffectiveTime:  &timestamppb.Timestamp{Seconds: binding.EffectiveTime},
		})
	}

	response := &billing_grpc.ListBillableObjectBindingsResponse{
		BillableObjectBindings: grpcBindings,
	}

	if bindings.NextPageToken != nil {
		response.NextPageToken = *bindings.NextPageToken
	} else {
		response.NextPageToken = ""
	}

	return response, nil
}

func (b *BillingAccountService) BindBillableObject(ctx context.Context, request *billing_grpc.BindBillableObjectRequest) (*operation.Operation, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.BindBillableObjectRequest{
		AuthData:         console.AuthData{Token: token},
		BillingAccountID: request.BillingAccountId,
	}
	if request.BillableObject != nil {
		consoleReq.BillableObjectID = request.BillableObject.Id
		consoleReq.BillableObjectType = request.BillableObject.Type
	}

	consoleOperation, err := b.client.BindBillableObject(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	return FormatOperation(consoleOperation)
}
