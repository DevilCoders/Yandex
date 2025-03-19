package console

import (
	"context"
	"encoding/json"
	"fmt"
	"strconv"
)

type ListBillableObjectBindingsRequest struct {
	AuthData
	BillingAccountID string
	PageSize         *int64
	PageToken        *string
}

type BillableObjectBindingResponse struct {
	BillingAccountID   string `json:"billingAccountId"`
	BillableObjectType string `json:"serviceInstanceType"`
	BillableObjectID   string `json:"serviceInstanceId"`
	EffectiveTime      int64  `json:"effectiveTime"`
}

type BillableObjectBindingsListResponse struct {
	BillableObjectBindings []BillableObjectBindingResponse `json:"serviceInstanceBindings"`
	NextPageToken          *string                         `json:"nextPageToken"`
}

type BindBillableObjectRequest struct {
	AuthData
	BillingAccountID   string
	BillableObjectType string
	BillableObjectID   string
}

type BindBillableObjectOperationMetadata struct {
	BillableObjectID string `json:"serviceInstanceId"`
}

func (c *consoleClient) ListBillableObjectBindings(ctx context.Context, req *ListBillableObjectBindingsRequest) (*BillableObjectBindingsListResponse, error) {

	url := "billingAccounts/{billingAccountID}/serviceInstances"
	request := c.createRequest(ctx, &req.AuthData)
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

	res := &BillableObjectBindingsListResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}

func (c *consoleClient) BindBillableObject(ctx context.Context, req *BindBillableObjectRequest) (*OperationResponse, error) {

	url := "/billingAccounts/{billingAccountID}/serviceInstances:bind"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("billingAccountID", req.BillingAccountID)

	body := fmt.Sprintf(`{"serviceInstanceType":"%s", "serviceInstanceId":"%s"}`, req.BillableObjectType, req.BillableObjectID)
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
