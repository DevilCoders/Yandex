package console

import (
	"context"
	"encoding/json"
	"strconv"
)

type GetBillingAccountRequest struct {
	AuthData
	BillingAccountID string
}

type BillingAccountResponse struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	CreatedAt   int64  `json:"createdAt"`
	CountryCode string `json:"countryCode"`
	Currency    string `json:"currency"`
	Active      bool   `json:"active"`
	Balance     string `json:"balance"`
}

type ListBillingAccountsRequest struct {
	AuthData
	PageSize  *int64
	PageToken *string
}

type ListBillingAccountsResponse struct {
	BillingAccounts []BillingAccountResponse `json:"billingAccounts"`
	NextPageToken   *string                  `json:"nextPageToken"`
}

func (c *consoleClient) GetBillingAccount(ctx context.Context, req *GetBillingAccountRequest) (*BillingAccountResponse, error) {

	url := "/billingAccounts/{id}"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("id", req.BillingAccountID)

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &BillingAccountResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}

func (c *consoleClient) ListBillingAccounts(ctx context.Context, req *ListBillingAccountsRequest) (*ListBillingAccountsResponse, error) {

	url := "/billingAccounts"
	request := c.createRequest(ctx, &req.AuthData)

	if req.PageSize != nil {
		request = request.SetQueryParam("pageSize", strconv.FormatInt(*req.PageSize, 10))
	}

	if req.PageToken != nil {
		request = request.SetQueryParam("pageToken", *req.PageToken)
	}

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &ListBillingAccountsResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}
