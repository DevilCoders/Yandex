package console

import (
	"context"
	"encoding/json"
)

type GetServiceRequest struct {
	AuthData
	ID string
}

type ServiceResponse struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	Description string `json:"description"`
}

type ListServiceRequest struct {
	AuthData
	Filter string
}

type ListServiceResponse struct {
	Services []ServiceResponse `json:"services"`
}

func (c *consoleClient) GetService(ctx context.Context, req *GetServiceRequest) (*ServiceResponse, error) {

	url := "/services/{id}"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("id", req.ID)

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &ServiceResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}
func (c *consoleClient) ListServices(ctx context.Context, req *ListServiceRequest) (*ListServiceResponse, error) {

	url := "/services"
	request := c.createRequest(ctx, &req.AuthData)
	request = request.SetQueryParam("filter", req.Filter)

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &ListServiceResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}
