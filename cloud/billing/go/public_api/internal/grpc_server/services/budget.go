package services

import (
	"context"
	"strings"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

var _ billing_grpc.BudgetServiceServer = &BudgetService{}

type BudgetService struct {
	client console.Client
}

func NewBudgetService(client console.Client) *BudgetService {
	return &BudgetService{client: client}
}

func (b BudgetService) Get(ctx context.Context, request *billing_grpc.GetBudgetRequest) (*billing_grpc.Budget, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.GetBudgetRequest{
		AuthData: console.AuthData{Token: token},
		ID:       request.Id,
	}
	consoleBudget, err := b.client.GetBudget(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	grpcBudget := mapConsoleBudgetToGrpcBudget(consoleBudget)
	return grpcBudget, nil
}

func (b BudgetService) List(ctx context.Context, request *billing_grpc.ListBudgetsRequest) (*billing_grpc.ListBudgetsResponse, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ListBudgetRequest{
		AuthData:         console.AuthData{Token: token},
		BillingAccountID: request.BillingAccountId,
	}

	consoleResponse, err := b.client.ListBudgets(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	var budgets []*billing_grpc.Budget
	for _, consoleBudget := range consoleResponse.Budgets {
		consoleBudget := consoleBudget
		budgets = append(budgets, mapConsoleBudgetToGrpcBudget(&consoleBudget))
	}

	return &billing_grpc.ListBudgetsResponse{Budgets: budgets}, nil
}

func (b BudgetService) Create(ctx context.Context, request *billing_grpc.CreateBudgetRequest) (*operation.Operation, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.CreateBudgetRequest{
		AuthData:         console.AuthData{Token: token},
		BillingAccountID: request.BillingAccountId,
		Name:             request.Name,
	}

	switch request.BudgetSpec.(type) {
	case *billing_grpc.CreateBudgetRequest_CostBudgetSpec:
		grpcCostBudgetSpec, _ := request.BudgetSpec.(*billing_grpc.CreateBudgetRequest_CostBudgetSpec)
		consoleReq.CostBudgetSpec = mapGrpcCostBudgetSpec(grpcCostBudgetSpec)
	case *billing_grpc.CreateBudgetRequest_ExpenseBudgetSpec:
		grpcExpenseBudgetSpec, _ := request.BudgetSpec.(*billing_grpc.CreateBudgetRequest_ExpenseBudgetSpec)
		consoleReq.ExpenseBudgetSpec = mapGrpcExpenseBudgetSpec(grpcExpenseBudgetSpec)
	case *billing_grpc.CreateBudgetRequest_BalanceBudgetSpec:
		grpcBalanceBudgetSpec, _ := request.BudgetSpec.(*billing_grpc.CreateBudgetRequest_BalanceBudgetSpec)
		consoleReq.BalanceBudgetSpec = mapGrpcBalanceBudgetSpec(grpcBalanceBudgetSpec)
	}

	consoleOperation, err := b.client.CreateBudget(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	return FormatOperation(consoleOperation)
}

func mapGrpcCostBudgetSpec(grpcCostBudgetSpec *billing_grpc.CreateBudgetRequest_CostBudgetSpec) *console.CostBudgetSpecRequest {
	if grpcCostBudgetSpec == nil || grpcCostBudgetSpec.CostBudgetSpec == nil {
		return &console.CostBudgetSpecRequest{}
	}

	return &console.CostBudgetSpecRequest{
		Amount:                     grpcCostBudgetSpec.CostBudgetSpec.Amount,
		NotificationUserAccountIds: grpcCostBudgetSpec.CostBudgetSpec.NotificationUserAccountIds,
		ThresholdRules:             mapGrpcThresholdRulesToConsole(grpcCostBudgetSpec.CostBudgetSpec.ThresholdRules),
		Filter:                     mapGrpcConsumptionFilterToConsole(grpcCostBudgetSpec.CostBudgetSpec.Filter),
		ResetPeriod:                strings.ToLower(grpcCostBudgetSpec.CostBudgetSpec.GetResetPeriod().String()),
		StartDate:                  grpcCostBudgetSpec.CostBudgetSpec.GetStartDate(),
		EndDate:                    grpcCostBudgetSpec.CostBudgetSpec.GetEndDate(),
	}
}

func mapGrpcConsumptionFilterToConsole(filter *billing_grpc.ConsumptionFilter) console.ConsumptionFilterRequest {
	result := console.ConsumptionFilterRequest{
		ServiceIds:         []string{},
		CloudFolderFilters: []console.CloudFoldersConsumptionFilterRequest{},
	}

	if filter == nil {
		return result
	}

	result.ServiceIds = filter.ServiceIds
	result.CloudFolderFilters = mapGrpcCloudFolderFilterToConsole(filter.CloudFoldersFilters)

	return result
}

func mapGrpcExpenseBudgetSpec(grpcExpenseBudgetSpec *billing_grpc.CreateBudgetRequest_ExpenseBudgetSpec) *console.ExpenseBudgetSpecRequest {
	if grpcExpenseBudgetSpec == nil || grpcExpenseBudgetSpec.ExpenseBudgetSpec == nil {
		return &console.ExpenseBudgetSpecRequest{}
	}

	return &console.ExpenseBudgetSpecRequest{
		Amount:                     grpcExpenseBudgetSpec.ExpenseBudgetSpec.Amount,
		NotificationUserAccountIds: grpcExpenseBudgetSpec.ExpenseBudgetSpec.NotificationUserAccountIds,
		ThresholdRules:             mapGrpcThresholdRulesToConsole(grpcExpenseBudgetSpec.ExpenseBudgetSpec.ThresholdRules),
		Filter:                     mapGrpcConsumptionFilterToConsole(grpcExpenseBudgetSpec.ExpenseBudgetSpec.Filter),
		ResetPeriod:                strings.ToLower(grpcExpenseBudgetSpec.ExpenseBudgetSpec.GetResetPeriod().String()),
		StartDate:                  grpcExpenseBudgetSpec.ExpenseBudgetSpec.GetStartDate(),
		EndDate:                    grpcExpenseBudgetSpec.ExpenseBudgetSpec.GetEndDate(),
	}
}

func mapGrpcBalanceBudgetSpec(grpcBalanceBudgetSpec *billing_grpc.CreateBudgetRequest_BalanceBudgetSpec) *console.BalanceBudgetSpecRequest {
	if grpcBalanceBudgetSpec == nil || grpcBalanceBudgetSpec.BalanceBudgetSpec == nil {
		return &console.BalanceBudgetSpecRequest{}
	}

	return &console.BalanceBudgetSpecRequest{
		Amount:                     grpcBalanceBudgetSpec.BalanceBudgetSpec.Amount,
		NotificationUserAccountIds: grpcBalanceBudgetSpec.BalanceBudgetSpec.NotificationUserAccountIds,
		ThresholdRules:             mapGrpcThresholdRulesToConsole(grpcBalanceBudgetSpec.BalanceBudgetSpec.ThresholdRules),
		StartDate:                  strings.ToLower(grpcBalanceBudgetSpec.BalanceBudgetSpec.GetStartDate()),
		EndDate:                    grpcBalanceBudgetSpec.BalanceBudgetSpec.GetEndDate(),
	}
}

func mapConsoleBudgetToGrpcBudget(consoleBudget *console.BudgetResponse) *billing_grpc.Budget {
	grpcBudget := &billing_grpc.Budget{
		Id:               consoleBudget.ID,
		Name:             consoleBudget.Name,
		CreatedAt:        &timestamppb.Timestamp{Seconds: consoleBudget.CreatedAt},
		BillingAccountId: consoleBudget.BillingAccountID,
		Status:           billing_grpc.BudgetStatus(billing_grpc.BudgetStatus_value[strings.ToUpper(consoleBudget.Status)]),
	}

	if consoleBudget.CostBudgetSpec != nil {
		grpcBudget.BudgetSpec = mapCostBudgetSpec(consoleBudget)
	} else if consoleBudget.ExpenseBudgetSpec != nil {
		grpcBudget.BudgetSpec = mapExpenseBudgetSpec(consoleBudget)
	} else if consoleBudget.BalanceBudgetSpec != nil {
		grpcBudget.BudgetSpec = mapBalanceBudgetSpec(consoleBudget)
	}
	return grpcBudget
}

func mapConsoleThresholdRulesToGrpc(consoleRules []console.ThresholdRuleResponse) []*billing_grpc.ThresholdRule {
	var result []*billing_grpc.ThresholdRule
	for _, consoleRule := range consoleRules {
		result = append(result, &billing_grpc.ThresholdRule{
			Type:                       billing_grpc.ThresholdType(billing_grpc.ThresholdType_value[strings.ToUpper(consoleRule.ThresholdType)]),
			Amount:                     consoleRule.Amount,
			NotificationUserAccountIds: consoleRule.NotificationUserAccountIds,
		})
	}
	return result
}

func mapGrpcThresholdRulesToConsole(grpcRules []*billing_grpc.ThresholdRule) []console.ThresholdRuleRequest {
	var result []console.ThresholdRuleRequest
	for _, grpcRule := range grpcRules {
		result = append(result, console.ThresholdRuleRequest{
			ThresholdType:              strings.ToLower(grpcRule.Type.String()),
			Amount:                     grpcRule.Amount,
			NotificationUserAccountIds: grpcRule.NotificationUserAccountIds,
		})
	}
	return result
}

func mapConsoleCloudFolderFilterToGrpc(consoleFilters []console.CloudFoldersConsumptionFilterResponse) []*billing_grpc.CloudFoldersConsumptionFilter {
	var result []*billing_grpc.CloudFoldersConsumptionFilter
	for _, consoleFilter := range consoleFilters {
		result = append(result, &billing_grpc.CloudFoldersConsumptionFilter{
			CloudId:   consoleFilter.CloudID,
			FolderIds: consoleFilter.FolderIds,
		})
	}
	return result
}

func mapGrpcCloudFolderFilterToConsole(grpcFilters []*billing_grpc.CloudFoldersConsumptionFilter) []console.CloudFoldersConsumptionFilterRequest {
	var result []console.CloudFoldersConsumptionFilterRequest
	for _, grpcFilter := range grpcFilters {
		result = append(result, console.CloudFoldersConsumptionFilterRequest{
			CloudID:   grpcFilter.CloudId,
			FolderIds: grpcFilter.FolderIds,
		})
	}
	return result
}

func mapCostBudgetSpec(consoleBudget *console.BudgetResponse) *billing_grpc.Budget_CostBudget {
	costBudget := &billing_grpc.Budget_CostBudget{
		CostBudget: &billing_grpc.CostBudgetSpec{
			Amount:                     consoleBudget.CostBudgetSpec.Amount,
			NotificationUserAccountIds: consoleBudget.CostBudgetSpec.NotificationUsers,
			ThresholdRules:             mapConsoleThresholdRulesToGrpc(consoleBudget.CostBudgetSpec.ThresholdRules),
			Filter: &billing_grpc.ConsumptionFilter{
				ServiceIds:          consoleBudget.CostBudgetSpec.Filter.ServiceIds,
				CloudFoldersFilters: mapConsoleCloudFolderFilterToGrpc(consoleBudget.CostBudgetSpec.Filter.CloudFoldersFilters),
			},
			EndDate: consoleBudget.CostBudgetSpec.EndDate,
		},
	}
	if consoleBudget.CostBudgetSpec.ResetPeriod != nil {
		costBudget.CostBudget.StartType = &billing_grpc.CostBudgetSpec_ResetPeriod{
			ResetPeriod: billing_grpc.ResetPeriodType(billing_grpc.ResetPeriodType_value[strings.ToUpper(*consoleBudget.CostBudgetSpec.ResetPeriod)]),
		}
	} else if consoleBudget.CostBudgetSpec.StartDate != nil {
		costBudget.CostBudget.StartType = &billing_grpc.CostBudgetSpec_StartDate{
			StartDate: *consoleBudget.CostBudgetSpec.ResetPeriod,
		}
	}
	return costBudget
}

func mapExpenseBudgetSpec(consoleBudget *console.BudgetResponse) *billing_grpc.Budget_ExpenseBudget {
	expenseBudget := &billing_grpc.Budget_ExpenseBudget{
		ExpenseBudget: &billing_grpc.ExpenseBudgetSpec{
			Amount:                     consoleBudget.ExpenseBudgetSpec.Amount,
			NotificationUserAccountIds: consoleBudget.ExpenseBudgetSpec.NotificationUsers,
			ThresholdRules:             mapConsoleThresholdRulesToGrpc(consoleBudget.ExpenseBudgetSpec.ThresholdRules),
			Filter: &billing_grpc.ConsumptionFilter{
				ServiceIds:          consoleBudget.ExpenseBudgetSpec.Filter.ServiceIds,
				CloudFoldersFilters: mapConsoleCloudFolderFilterToGrpc(consoleBudget.ExpenseBudgetSpec.Filter.CloudFoldersFilters),
			},
			EndDate: consoleBudget.ExpenseBudgetSpec.EndDate,
		},
	}
	if consoleBudget.ExpenseBudgetSpec.ResetPeriod != nil {
		expenseBudget.ExpenseBudget.StartType = &billing_grpc.ExpenseBudgetSpec_ResetPeriod{
			ResetPeriod: billing_grpc.ResetPeriodType(billing_grpc.ResetPeriodType_value[strings.ToUpper(*consoleBudget.ExpenseBudgetSpec.ResetPeriod)]),
		}
	} else if consoleBudget.ExpenseBudgetSpec.StartDate != nil {
		expenseBudget.ExpenseBudget.StartType = &billing_grpc.ExpenseBudgetSpec_StartDate{
			StartDate: *consoleBudget.ExpenseBudgetSpec.ResetPeriod,
		}
	}
	return expenseBudget
}

func mapBalanceBudgetSpec(consoleBudget *console.BudgetResponse) *billing_grpc.Budget_BalanceBudget {
	balanceBudget := &billing_grpc.Budget_BalanceBudget{
		BalanceBudget: &billing_grpc.BalanceBudgetSpec{
			Amount:                     consoleBudget.BalanceBudgetSpec.Amount,
			NotificationUserAccountIds: consoleBudget.BalanceBudgetSpec.NotificationUsers,
			ThresholdRules:             mapConsoleThresholdRulesToGrpc(consoleBudget.BalanceBudgetSpec.ThresholdRules),
			EndDate:                    consoleBudget.BalanceBudgetSpec.EndDate,
		},
	}
	return balanceBudget
}
