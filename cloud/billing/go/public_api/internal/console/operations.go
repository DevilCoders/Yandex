package console

import (
	"context"
	"encoding/json"

	"github.com/go-resty/resty/v2"
)

var (
	CreateBudgetOperationType         = "create_budget"
	BindBillableObjectOperationType   = "service_instance_bind_to_billing_account"
	InviteCustomerOperationType       = "subaccount_create" // TODO: rename operation types
	ActivateCustomerOperationType     = "subaccount_activate"
	SuspendCustomerOperationType      = "subaccount_suspend"
	UpdateAccessBindingsOperationType = "access_bindings_update"
)

type GetOperationRequest struct {
	AuthData
	ID string
}

type typedOperationResponse struct {
	ID          string           `json:"id"`
	Type        string           `json:"type"`
	Description string           `json:"description"`
	CreatedAt   int64            `json:"createdAt"`
	CreatedBy   string           `json:"createdBy"`
	ModifiedAt  int64            `json:"modifiedAt"`
	Done        bool             `json:"done"`
	Metadata    json.RawMessage  `json:"metadata"`
	Response    *json.RawMessage `json:"response"`
	Error       *int32           `json:"error"`
}

type OperationResponse struct {
	ID          string
	Description string
	CreatedAt   int64
	CreatedBy   string
	ModifiedAt  int64
	Done        bool
	Metadata    interface{}
	Response    *interface{}
	Error       *int32
}

func (c *consoleClient) GetOperation(ctx context.Context, req *GetOperationRequest) (*OperationResponse, error) {

	url := "/operations/{id}"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("id", req.ID)

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	return UnmarshalOperation(response)
}

func UnmarshalOperation(httpResponse *resty.Response) (*OperationResponse, error) {
	res := &typedOperationResponse{}
	err := json.Unmarshal(httpResponse.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	if res.Error == nil && res.Response == nil {
		return nil, ErrUnexpectedResponse
	}

	if res.Error != nil && res.Response != nil {
		return nil, ErrUnexpectedResponse
	}

	operationResponse := &OperationResponse{
		ID:          res.ID,
		Description: formatOperationDescription(res),
		CreatedAt:   res.CreatedAt,
		CreatedBy:   res.CreatedBy,
		ModifiedAt:  res.ModifiedAt,
		Done:        res.Done,
		Metadata:    nil,
		Response:    nil,
		Error:       res.Error,
	}

	metadata, err := unmarshalOperationMetadata(res)
	if err != nil {
		return nil, err
	}
	operationResponse.Metadata = metadata

	if res.Error == nil {
		response, err := unmarshalOperationResponse(res)
		if err != nil {
			return nil, err
		}
		operationResponse.Response = &response
	}

	return operationResponse, nil

}

func unmarshalOperationMetadata(typedOperation *typedOperationResponse) (interface{}, error) {
	var metadata interface{}
	var err error

	if typedOperation.Type == CreateBudgetOperationType {
		metadata = &CreateBudgetMetadata{}
		err = json.Unmarshal(typedOperation.Metadata, metadata)
	} else if typedOperation.Type == BindBillableObjectOperationType {
		metadata = &BindBillableObjectOperationMetadata{}
		err = json.Unmarshal(typedOperation.Metadata, metadata)
	} else if typedOperation.Type == InviteCustomerOperationType || typedOperation.Type == ActivateCustomerOperationType || typedOperation.Type == SuspendCustomerOperationType {
		metadata = &CustomerMetadata{}
		err = json.Unmarshal(typedOperation.Metadata, metadata)
	} else if typedOperation.Type == UpdateAccessBindingsOperationType {
		metadata = &UpdateAccessBindingsMetadata{}
		err = json.Unmarshal(typedOperation.Metadata, metadata)
	} else {
		return nil, ErrUnexpectedResponse
	}

	if err != nil {
		return nil, ErrUnexpectedResponse
	}
	return metadata, nil
}

func unmarshalOperationResponse(typedOperation *typedOperationResponse) (interface{}, error) {
	var response interface{}
	var err error

	if typedOperation.Type == CreateBudgetOperationType {
		response = &BudgetResponse{}
		err = json.Unmarshal(*typedOperation.Response, response)
	} else if typedOperation.Type == BindBillableObjectOperationType {
		response = &BillableObjectBindingResponse{}
		err = json.Unmarshal(*typedOperation.Response, response)
	} else if typedOperation.Type == InviteCustomerOperationType || typedOperation.Type == ActivateCustomerOperationType || typedOperation.Type == SuspendCustomerOperationType {
		response = &Customer{}
		err = json.Unmarshal(*typedOperation.Response, response)
	} else if typedOperation.Type == UpdateAccessBindingsOperationType {
		response = &UpdateAccessBindingsResponse{}
		err = json.Unmarshal(*typedOperation.Response, response)
	} else {
		return nil, ErrUnexpectedResponse
	}

	if err != nil {
		return nil, ErrUnexpectedResponse
	}
	return response, nil
}

func formatOperationDescription(typedOperation *typedOperationResponse) string {

	if typedOperation.Type == BindBillableObjectOperationType {
		return "billable object bind to billing account" // in python it is "service instance bind to billing account"
	} else {
		return typedOperation.Description
	}

}
