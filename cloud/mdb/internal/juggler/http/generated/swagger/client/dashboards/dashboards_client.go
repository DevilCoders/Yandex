// Code generated by go-swagger; DO NOT EDIT.

package dashboards

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"
)

// New creates a new dashboards API client.
func New(transport runtime.ClientTransport, formats strfmt.Registry) ClientService {
	return &Client{transport: transport, formats: formats}
}

/*
Client for dashboards API
*/
type Client struct {
	transport runtime.ClientTransport
	formats   strfmt.Registry
}

// ClientOption is the option for Client methods
type ClientOption func(*runtime.ClientOperation)

// ClientService is the interface for Client methods
type ClientService interface {
	V2DashboardsGetDashboards(params *V2DashboardsGetDashboardsParams, opts ...ClientOption) (*V2DashboardsGetDashboardsOK, error)

	V2DashboardsRemoveDashboard(params *V2DashboardsRemoveDashboardParams, opts ...ClientOption) (*V2DashboardsRemoveDashboardOK, error)

	V2DashboardsSetDashboard(params *V2DashboardsSetDashboardParams, opts ...ClientOption) (*V2DashboardsSetDashboardOK, error)

	SetTransport(transport runtime.ClientTransport)
}

/*
  V2DashboardsGetDashboards Experimental API, subject to breakage
*/
func (a *Client) V2DashboardsGetDashboards(params *V2DashboardsGetDashboardsParams, opts ...ClientOption) (*V2DashboardsGetDashboardsOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewV2DashboardsGetDashboardsParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "/v2/dashboards/get_dashboards",
		Method:             "POST",
		PathPattern:        "/v2/dashboards/get_dashboards",
		ProducesMediaTypes: []string{"application/json", "application/x-protobuf"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &V2DashboardsGetDashboardsReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*V2DashboardsGetDashboardsOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	unexpectedSuccess := result.(*V2DashboardsGetDashboardsDefault)
	return nil, runtime.NewAPIError("unexpected success response: content available as default response in error", unexpectedSuccess, unexpectedSuccess.Code())
}

/*
  V2DashboardsRemoveDashboard Experimental API, subject to breakage
*/
func (a *Client) V2DashboardsRemoveDashboard(params *V2DashboardsRemoveDashboardParams, opts ...ClientOption) (*V2DashboardsRemoveDashboardOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewV2DashboardsRemoveDashboardParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "/v2/dashboards/remove_dashboard",
		Method:             "POST",
		PathPattern:        "/v2/dashboards/remove_dashboard",
		ProducesMediaTypes: []string{"application/json", "application/x-protobuf"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &V2DashboardsRemoveDashboardReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*V2DashboardsRemoveDashboardOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	unexpectedSuccess := result.(*V2DashboardsRemoveDashboardDefault)
	return nil, runtime.NewAPIError("unexpected success response: content available as default response in error", unexpectedSuccess, unexpectedSuccess.Code())
}

/*
  V2DashboardsSetDashboard Experimental API, subject to breakage
*/
func (a *Client) V2DashboardsSetDashboard(params *V2DashboardsSetDashboardParams, opts ...ClientOption) (*V2DashboardsSetDashboardOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewV2DashboardsSetDashboardParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "/v2/dashboards/set_dashboard",
		Method:             "POST",
		PathPattern:        "/v2/dashboards/set_dashboard",
		ProducesMediaTypes: []string{"application/json", "application/x-protobuf"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &V2DashboardsSetDashboardReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*V2DashboardsSetDashboardOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	unexpectedSuccess := result.(*V2DashboardsSetDashboardDefault)
	return nil, runtime.NewAPIError("unexpected success response: content available as default response in error", unexpectedSuccess, unexpectedSuccess.Code())
}

// SetTransport changes the transport on the client
func (a *Client) SetTransport(transport runtime.ClientTransport) {
	a.transport = transport
}
