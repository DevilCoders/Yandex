package console

import (
	"context"
)

type Client interface {
	GetService(ctx context.Context, request *GetServiceRequest) (*ServiceResponse, error)

	ListServices(ctx context.Context, request *ListServiceRequest) (*ListServiceResponse, error)

	GetOperation(ctx context.Context, request *GetOperationRequest) (*OperationResponse, error)

	ListBillableObjectBindings(ctx context.Context, request *ListBillableObjectBindingsRequest) (*BillableObjectBindingsListResponse, error)

	BindBillableObject(ctx context.Context, request *BindBillableObjectRequest) (*OperationResponse, error)

	ListCustomers(ctx context.Context, request *ListCustomersRequest) (*ListCustomersResponse, error)

	InviteCustomer(ctx context.Context, request *InviteCustomerRequest) (*OperationResponse, error)

	ActivateCustomer(ctx context.Context, request *ActivateCustomerRequest) (*OperationResponse, error)

	SuspendCustomer(ctx context.Context, request *SuspendCustomerRequest) (*OperationResponse, error)

	GetSku(ctx context.Context, req *GetSkuRequest) (*Sku, error)

	ListSkus(ctx context.Context, req *ListSkuRequest) (*ListSkuResponse, error)

	ListAccessBindings(ctx context.Context, request *ListAccessBindingsRequest, authData *AuthData) (*ListAccessBindingsResponse, error)

	UpdateAccessBindings(ctx context.Context, request *UpdateAccessBindingsRequest, authData *AuthData) (*OperationResponse, error)

	GetBillingAccount(ctx context.Context, request *GetBillingAccountRequest) (*BillingAccountResponse, error)

	ListBillingAccounts(ctx context.Context, request *ListBillingAccountsRequest) (*ListBillingAccountsResponse, error)

	GetBudget(ctx context.Context, request *GetBudgetRequest) (*BudgetResponse, error)

	ListBudgets(ctx context.Context, request *ListBudgetRequest) (*ListBudgetResponse, error)

	CreateBudget(ctx context.Context, request *CreateBudgetRequest) (*OperationResponse, error)
}
