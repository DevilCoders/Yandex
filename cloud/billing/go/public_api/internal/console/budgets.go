package console

import (
	"context"
	"encoding/json"
)

type GetBudgetRequest struct {
	AuthData `json:"-"`
	ID       string `json:"id"`
}

type CloudFoldersConsumptionFilterResponse struct {
	CloudID   string   `json:"cloudId"`
	FolderIds []string `json:"folderIds"`
}

type ConsumptionFilterResponse struct {
	ServiceIds          []string                                `json:"serviceIds"`
	CloudFoldersFilters []CloudFoldersConsumptionFilterResponse `json:"cloudFoldersFilters"`
}

type ThresholdRuleResponse struct {
	ThresholdType              string   `json:"thresholdType"`
	Amount                     string   `json:"amount"`
	NotificationUserAccountIds []string `json:"notificationUserAccountIds"`
}

type CostBudgetSpecResponse struct {
	Amount            string                    `json:"amount"`
	NotificationUsers []string                  `json:"notificationUsers"`
	ThresholdRules    []ThresholdRuleResponse   `json:"thresholdRules"`
	Filter            ConsumptionFilterResponse `json:"filter"`
	ResetPeriod       *string                   `json:"resetPeriod"`
	StartDate         *string                   `json:"startDate"`
	EndDate           string                    `json:"endDate"`
}

type ExpenseBudgetSpecResponse struct {
	Amount            string                    `json:"amount"`
	NotificationUsers []string                  `json:"notificationUsers"`
	ThresholdRules    []ThresholdRuleResponse   `json:"thresholdRules"`
	Filter            ConsumptionFilterResponse `json:"filter"`
	ResetPeriod       *string                   `json:"resetPeriod"`
	StartDate         *string                   `json:"startDate"`
	EndDate           string                    `json:"endDate"`
}

type BalanceBudgetSpecResponse struct {
	Amount            string                  `json:"amount"`
	NotificationUsers []string                `json:"notificationUsers"`
	ThresholdRules    []ThresholdRuleResponse `json:"thresholdRules"`
	StartDate         *string                 `json:"startDate"`
	EndDate           string                  `json:"endDate"`
}

type BudgetResponse struct {
	ID               string `json:"id"`
	Name             string `json:"name"`
	CreatedAt        int64  `json:"createdAt"`
	BillingAccountID string `json:"billingAccountId"`
	Status           string `json:"status"`

	CostBudgetSpec    *CostBudgetSpecResponse    `json:"costBudgetSpec"`
	ExpenseBudgetSpec *ExpenseBudgetSpecResponse `json:"expenseBudgetSpec"`
	BalanceBudgetSpec *BalanceBudgetSpecResponse `json:"balanceBudgetSpec"`
}

type ListBudgetRequest struct {
	AuthData         `json:"-"`
	BillingAccountID string  `json:"billingAccountId"`
	PageSize         *int64  `json:"pageSize"`
	PageToken        *string `json:"pageToken"`
}

type ListBudgetResponse struct {
	Budgets       []BudgetResponse `json:"budgets"`
	NextPageToken *string          `json:"nextPageToken"`
}

type CloudFoldersConsumptionFilterRequest struct {
	CloudID   string   `json:"cloudId"`
	FolderIds []string `json:"folderIds"`
}

type ConsumptionFilterRequest struct {
	ServiceIds         []string                               `json:"serviceIds"`
	CloudFolderFilters []CloudFoldersConsumptionFilterRequest `json:"cloudFoldersFilters"`
}

type ThresholdRuleRequest struct {
	ThresholdType              string   `json:"thresholdType"`
	Amount                     string   `json:"amount"`
	NotificationUserAccountIds []string `json:"notificationUserAccountIds"`
}

type CostBudgetSpecRequest struct {
	Amount                     string                   `json:"amount"`
	NotificationUserAccountIds []string                 `json:"notificationUserAccountIds"`
	ThresholdRules             []ThresholdRuleRequest   `json:"thresholdRules"`
	Filter                     ConsumptionFilterRequest `json:"filter"`
	ResetPeriod                string                   `json:"resetPeriod"`
	StartDate                  string                   `json:"startDate"`
	EndDate                    string                   `json:"endDate"`
}

type ExpenseBudgetSpecRequest struct {
	Amount                     string                   `json:"amount"`
	NotificationUserAccountIds []string                 `json:"notificationUserAccountIds"`
	ThresholdRules             []ThresholdRuleRequest   `json:"thresholdRules"`
	Filter                     ConsumptionFilterRequest `json:"filter"`
	ResetPeriod                string                   `json:"resetPeriod"`
	StartDate                  string                   `json:"startDate"`
	EndDate                    string                   `json:"endDate"`
}

type BalanceBudgetSpecRequest struct {
	Amount                     string                 `json:"amount"`
	NotificationUserAccountIds []string               `json:"notificationUserAccountIds"`
	ThresholdRules             []ThresholdRuleRequest `json:"thresholdRules"`
	StartDate                  string                 `json:"startDate"`
	EndDate                    string                 `json:"endDate"`
}

type CreateBudgetRequest struct {
	AuthData         `json:"-"`
	BillingAccountID string `json:"billingAccountId"`
	Name             string `json:"name"`

	CostBudgetSpec    *CostBudgetSpecRequest    `json:"costBudgetSpec"`
	ExpenseBudgetSpec *ExpenseBudgetSpecRequest `json:"expenseBudgetSpec"`
	BalanceBudgetSpec *BalanceBudgetSpecRequest `json:"balanceBudgetSpec"`
}

type CreateBudgetMetadata struct {
	BudgetID string `json:"budgetId"`
}

func (c *consoleClient) GetBudget(ctx context.Context, req *GetBudgetRequest) (*BudgetResponse, error) {

	url := "/budgets/{id}"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("id", req.ID)

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &BudgetResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}
func (c *consoleClient) ListBudgets(ctx context.Context, req *ListBudgetRequest) (*ListBudgetResponse, error) {

	url := "/billingAccounts/{billingAccountID}/budgets"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("billingAccountID", req.BillingAccountID)

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &ListBudgetResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}

func (c *consoleClient) CreateBudget(ctx context.Context, req *CreateBudgetRequest) (*OperationResponse, error) {
	url := "/budgets"
	request := c.createRequest(ctx, &req.AuthData)

	reqBody, err := json.Marshal(req)
	if err != nil {
		return nil, ErrRequest
	}

	request = request.SetHeader("Content-Type", "application/json").SetBody(reqBody)

	response, err := request.Post(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	return UnmarshalOperation(response)

}
