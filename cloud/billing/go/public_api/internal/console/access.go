package console

import (
	"context"
	"encoding/json"
	"strconv"
)

type ListAccessBindingsRequest struct {
	BillingAccountID string
	PageSize         *int64
	PageToken        *string
}

type UpdateAccessBindingsRequest struct {
	BillingAccountID     string               `json:"-"`
	AccessBindingsDeltas []AccessBindingDelta `json:"accessBindingDeltas"`
}

type AccessBindingDelta struct {
	Action        string        `json:"action"`
	AccessBinding AccessBinding `json:"accessBinding"`
}

type AccessBinding struct {
	RoleID  string  `json:"roleId"`
	Subject Subject `json:"subject"`
}

type Subject struct {
	ID   string `json:"id"`
	Type string `json:"type"`
}

type ListAccessBindingsResponse struct {
	AccessBindings []AccessBinding `json:"accessBindings"`
	NextPageToken  *string         `json:"nextPageToken"`
}

type UpdateAccessBindingsMetadata struct {
	BillingAccountID string `json:"billingAccountId"`
}

type UpdateAccessBindingsResponse struct {
}

func (c *consoleClient) ListAccessBindings(ctx context.Context, req *ListAccessBindingsRequest, authData *AuthData) (*ListAccessBindingsResponse, error) {
	url := "billingAccounts/{billingAccountID}:listAccessBindings"
	request := c.createRequest(ctx, authData)
	request.SetPathParam("billingAccountID", req.BillingAccountID)

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

	res := &ListAccessBindingsResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}

func (c *consoleClient) UpdateAccessBindings(ctx context.Context, req *UpdateAccessBindingsRequest, authData *AuthData) (*OperationResponse, error) {
	url := "/billingAccounts/{billingAccountID}:updateAccessBindings"
	request := c.createRequest(ctx, authData)
	request.SetPathParam("billingAccountID", req.BillingAccountID)

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
