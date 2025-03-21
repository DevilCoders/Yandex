// Code generated by go-swagger; DO NOT EDIT.

package minions

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
)

// NewGetMinionMasterParams creates a new GetMinionMasterParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewGetMinionMasterParams() *GetMinionMasterParams {
	return &GetMinionMasterParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewGetMinionMasterParamsWithTimeout creates a new GetMinionMasterParams object
// with the ability to set a timeout on a request.
func NewGetMinionMasterParamsWithTimeout(timeout time.Duration) *GetMinionMasterParams {
	return &GetMinionMasterParams{
		timeout: timeout,
	}
}

// NewGetMinionMasterParamsWithContext creates a new GetMinionMasterParams object
// with the ability to set a context for a request.
func NewGetMinionMasterParamsWithContext(ctx context.Context) *GetMinionMasterParams {
	return &GetMinionMasterParams{
		Context: ctx,
	}
}

// NewGetMinionMasterParamsWithHTTPClient creates a new GetMinionMasterParams object
// with the ability to set a custom HTTPClient for a request.
func NewGetMinionMasterParamsWithHTTPClient(client *http.Client) *GetMinionMasterParams {
	return &GetMinionMasterParams{
		HTTPClient: client,
	}
}

/* GetMinionMasterParams contains all the parameters to send to the API endpoint
   for the get minion master operation.

   Typically these are written to a http.Request.
*/
type GetMinionMasterParams struct {

	/* XRequestID.

	   Unique request ID (must be generated for each separate request, even retries)
	*/
	XRequestID *string

	/* Fqdn.

	   fqdn of whatever
	*/
	Fqdn string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the get minion master params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *GetMinionMasterParams) WithDefaults() *GetMinionMasterParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the get minion master params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *GetMinionMasterParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the get minion master params
func (o *GetMinionMasterParams) WithTimeout(timeout time.Duration) *GetMinionMasterParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the get minion master params
func (o *GetMinionMasterParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the get minion master params
func (o *GetMinionMasterParams) WithContext(ctx context.Context) *GetMinionMasterParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the get minion master params
func (o *GetMinionMasterParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the get minion master params
func (o *GetMinionMasterParams) WithHTTPClient(client *http.Client) *GetMinionMasterParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the get minion master params
func (o *GetMinionMasterParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithXRequestID adds the xRequestID to the get minion master params
func (o *GetMinionMasterParams) WithXRequestID(xRequestID *string) *GetMinionMasterParams {
	o.SetXRequestID(xRequestID)
	return o
}

// SetXRequestID adds the xRequestId to the get minion master params
func (o *GetMinionMasterParams) SetXRequestID(xRequestID *string) {
	o.XRequestID = xRequestID
}

// WithFqdn adds the fqdn to the get minion master params
func (o *GetMinionMasterParams) WithFqdn(fqdn string) *GetMinionMasterParams {
	o.SetFqdn(fqdn)
	return o
}

// SetFqdn adds the fqdn to the get minion master params
func (o *GetMinionMasterParams) SetFqdn(fqdn string) {
	o.Fqdn = fqdn
}

// WriteToRequest writes these params to a swagger request
func (o *GetMinionMasterParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	if o.XRequestID != nil {

		// header param X-Request-Id
		if err := r.SetHeaderParam("X-Request-Id", *o.XRequestID); err != nil {
			return err
		}
	}

	// path param fqdn
	if err := r.SetPathParam("fqdn", o.Fqdn); err != nil {
		return err
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
