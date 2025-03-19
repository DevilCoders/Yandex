package services

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	operation "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

var _ billing_grpc.CustomerServiceServer = &CustomerService{}

type CustomerService struct {
	client console.Client
}

func NewCustomerService(client console.Client) *CustomerService {
	return &CustomerService{client: client}
}

func (c *CustomerService) List(ctx context.Context, request *billing_grpc.ListCustomersRequest) (*billing_grpc.ListCustomersResponse, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ListCustomersRequest{
		AuthData:   console.AuthData{Token: token},
		ResellerID: request.ResellerId,
	}

	if request.PageToken != "" {
		consoleReq.PageToken = &request.PageToken
	}

	if request.PageSize != 0 {
		consoleReq.PageSize = &request.PageSize
	}

	consoleResp, err := c.client.ListCustomers(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	var customers []*billing_grpc.Customer
	for _, customer := range consoleResp.Customers {
		customers = append(customers, &billing_grpc.Customer{
			Id:               customer.ID,
			BillingAccountId: customer.BillingAccountID,
		})
	}

	response := &billing_grpc.ListCustomersResponse{
		Customers: customers,
	}
	if consoleResp.NextPageToken != nil {
		response.NextPageToken = *consoleResp.NextPageToken
	}

	return response, nil
}

func (c CustomerService) Invite(ctx context.Context, request *billing_grpc.InviteCustomerRequest) (*operation.Operation, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.InviteCustomerRequest{
		AuthData:        console.AuthData{Token: token},
		ResellerID:      request.ResellerId,
		InvitationEmail: request.InvitationEmail,
		Name:            request.Name,
	}
	if request.Person != nil {
		consoleReq.Person = console.CustomerPerson{
			Name:         request.Person.Name,
			Longname:     request.Person.Longname,
			Phone:        request.Person.Phone,
			Email:        request.Person.Email,
			PostCode:     request.Person.PostCode,
			PostAddress:  request.Person.PostAddress,
			LegalAddress: request.Person.LegalAddress,
			Tin:          request.Person.Tin,
		}
	}

	consoleOperation, err := c.client.InviteCustomer(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	return FormatOperation(consoleOperation)
}

func (c CustomerService) Activate(ctx context.Context, request *billing_grpc.ActivateCustomerRequest) (*operation.Operation, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ActivateCustomerRequest{
		AuthData:   console.AuthData{Token: token},
		CustomerID: request.CustomerId,
	}

	consoleOperation, err := c.client.ActivateCustomer(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	return FormatOperation(consoleOperation)
}

func (c CustomerService) Suspend(ctx context.Context, request *billing_grpc.SuspendCustomerRequest) (*operation.Operation, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.SuspendCustomerRequest{
		AuthData:   console.AuthData{Token: token},
		CustomerID: request.CustomerId,
	}

	consoleOperation, err := c.client.SuspendCustomer(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	return FormatOperation(consoleOperation)
}
