package console

import (
	"context"
	"encoding/json"
	"strconv"
)

type ListCustomersRequest struct {
	AuthData
	ResellerID string
	PageSize   *int64
	PageToken  *string
}

type InviteCustomerRequest struct {
	AuthData        `json:"-"`
	ResellerID      string         `json:"resellerId"`
	Name            string         `json:"name"`
	InvitationEmail string         `json:"invitationEmail"`
	Person          CustomerPerson `json:"person"`
}

type ActivateCustomerRequest struct {
	AuthData
	CustomerID string
}

type SuspendCustomerRequest struct {
	AuthData
	CustomerID string
}

type CustomerPerson struct {
	Name         string `json:"name"`
	Longname     string `json:"longname"`
	Phone        string `json:"phone"`
	Email        string `json:"email"`
	PostCode     string `json:"postCode"`
	PostAddress  string `json:"postAddress"`
	LegalAddress string `json:"legalAddress"`
	Tin          string `json:"tin"`
}

type ListCustomersResponse struct {
	Customers     []Customer `json:"customers"`
	NextPageToken *string    `json:"nextPageToken"`
}

type Customer struct {
	ID               string `json:"id"`
	BillingAccountID string `json:"billingAccountId"`
}

type CustomerMetadata struct {
	ResellerID string `json:"resellerId"`
	CustomerID string `json:"customerId"`
}

func (c *consoleClient) ListCustomers(ctx context.Context, req *ListCustomersRequest) (*ListCustomersResponse, error) {
	url := "/customers"
	request := c.createRequest(ctx, &req.AuthData)
	request = request.SetQueryParam("resellerId", req.ResellerID)

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

	res := &ListCustomersResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}

func (c *consoleClient) InviteCustomer(ctx context.Context, req *InviteCustomerRequest) (*OperationResponse, error) {
	url := "/customers:invite"
	request := c.createRequest(ctx, &req.AuthData)

	body, err := json.Marshal(req)
	if err != nil {
		return nil, ErrMarshalResponse.Wrap(err)
	}
	request = request.SetHeader("Content-Type", "application/json").SetBody(body)

	response, err := request.Post(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	return UnmarshalOperation(response)
}

func (c *consoleClient) ActivateCustomer(ctx context.Context, req *ActivateCustomerRequest) (*OperationResponse, error) {
	url := "/customers/{customerID}:activate"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("customerID", req.CustomerID)

	request = request.SetHeader("Content-Type", "application/json")

	response, err := request.Post(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	return UnmarshalOperation(response)
}

func (c *consoleClient) SuspendCustomer(ctx context.Context, req *SuspendCustomerRequest) (*OperationResponse, error) {
	url := "/customers/{customerID}:suspend"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("customerID", req.CustomerID)

	request = request.SetHeader("Content-Type", "application/json")

	response, err := request.Post(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	return UnmarshalOperation(response)
}
