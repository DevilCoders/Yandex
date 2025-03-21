// Code generated by go-swagger; DO NOT EDIT.

package notifications

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"net/http"
	"time"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/runtime"
	cr "github.com/go-openapi/runtime/client"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

// NewV2EscalationsStopParams creates a new V2EscalationsStopParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewV2EscalationsStopParams() *V2EscalationsStopParams {
	return &V2EscalationsStopParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewV2EscalationsStopParamsWithTimeout creates a new V2EscalationsStopParams object
// with the ability to set a timeout on a request.
func NewV2EscalationsStopParamsWithTimeout(timeout time.Duration) *V2EscalationsStopParams {
	return &V2EscalationsStopParams{
		timeout: timeout,
	}
}

// NewV2EscalationsStopParamsWithContext creates a new V2EscalationsStopParams object
// with the ability to set a context for a request.
func NewV2EscalationsStopParamsWithContext(ctx context.Context) *V2EscalationsStopParams {
	return &V2EscalationsStopParams{
		Context: ctx,
	}
}

// NewV2EscalationsStopParamsWithHTTPClient creates a new V2EscalationsStopParams object
// with the ability to set a custom HTTPClient for a request.
func NewV2EscalationsStopParamsWithHTTPClient(client *http.Client) *V2EscalationsStopParams {
	return &V2EscalationsStopParams{
		HTTPClient: client,
	}
}

/* V2EscalationsStopParams contains all the parameters to send to the API endpoint
   for the v2 escalations stop operation.

   Typically these are written to a http.Request.
*/
type V2EscalationsStopParams struct {

	// StopEscalationRequest.
	StopEscalationRequest *models.V2EscalationsStopParamsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the v2 escalations stop params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2EscalationsStopParams) WithDefaults() *V2EscalationsStopParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the v2 escalations stop params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2EscalationsStopParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the v2 escalations stop params
func (o *V2EscalationsStopParams) WithTimeout(timeout time.Duration) *V2EscalationsStopParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the v2 escalations stop params
func (o *V2EscalationsStopParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the v2 escalations stop params
func (o *V2EscalationsStopParams) WithContext(ctx context.Context) *V2EscalationsStopParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the v2 escalations stop params
func (o *V2EscalationsStopParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the v2 escalations stop params
func (o *V2EscalationsStopParams) WithHTTPClient(client *http.Client) *V2EscalationsStopParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the v2 escalations stop params
func (o *V2EscalationsStopParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithStopEscalationRequest adds the stopEscalationRequest to the v2 escalations stop params
func (o *V2EscalationsStopParams) WithStopEscalationRequest(stopEscalationRequest *models.V2EscalationsStopParamsBody) *V2EscalationsStopParams {
	o.SetStopEscalationRequest(stopEscalationRequest)
	return o
}

// SetStopEscalationRequest adds the stopEscalationRequest to the v2 escalations stop params
func (o *V2EscalationsStopParams) SetStopEscalationRequest(stopEscalationRequest *models.V2EscalationsStopParamsBody) {
	o.StopEscalationRequest = stopEscalationRequest
}

// WriteToRequest writes these params to a swagger request
func (o *V2EscalationsStopParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error
	if o.StopEscalationRequest != nil {
		if err := r.SetBodyParam(o.StopEscalationRequest); err != nil {
			return err
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
