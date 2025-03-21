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

	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
)

// NewCreateMinionParams creates a new CreateMinionParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewCreateMinionParams() *CreateMinionParams {
	return &CreateMinionParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewCreateMinionParamsWithTimeout creates a new CreateMinionParams object
// with the ability to set a timeout on a request.
func NewCreateMinionParamsWithTimeout(timeout time.Duration) *CreateMinionParams {
	return &CreateMinionParams{
		timeout: timeout,
	}
}

// NewCreateMinionParamsWithContext creates a new CreateMinionParams object
// with the ability to set a context for a request.
func NewCreateMinionParamsWithContext(ctx context.Context) *CreateMinionParams {
	return &CreateMinionParams{
		Context: ctx,
	}
}

// NewCreateMinionParamsWithHTTPClient creates a new CreateMinionParams object
// with the ability to set a custom HTTPClient for a request.
func NewCreateMinionParamsWithHTTPClient(client *http.Client) *CreateMinionParams {
	return &CreateMinionParams{
		HTTPClient: client,
	}
}

/* CreateMinionParams contains all the parameters to send to the API endpoint
   for the create minion operation.

   Typically these are written to a http.Request.
*/
type CreateMinionParams struct {

	/* Authorization.

	   OAuth token. It is not in security section because we also use cookies and you can't specify those in swagger 2.0.
	*/
	Authorization *string

	/* XRequestID.

	   Unique request ID (must be generated for each separate request, even retries)
	*/
	XRequestID *string

	// Body.
	Body *models.Minion

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the create minion params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *CreateMinionParams) WithDefaults() *CreateMinionParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the create minion params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *CreateMinionParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the create minion params
func (o *CreateMinionParams) WithTimeout(timeout time.Duration) *CreateMinionParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the create minion params
func (o *CreateMinionParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the create minion params
func (o *CreateMinionParams) WithContext(ctx context.Context) *CreateMinionParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the create minion params
func (o *CreateMinionParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the create minion params
func (o *CreateMinionParams) WithHTTPClient(client *http.Client) *CreateMinionParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the create minion params
func (o *CreateMinionParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithAuthorization adds the authorization to the create minion params
func (o *CreateMinionParams) WithAuthorization(authorization *string) *CreateMinionParams {
	o.SetAuthorization(authorization)
	return o
}

// SetAuthorization adds the authorization to the create minion params
func (o *CreateMinionParams) SetAuthorization(authorization *string) {
	o.Authorization = authorization
}

// WithXRequestID adds the xRequestID to the create minion params
func (o *CreateMinionParams) WithXRequestID(xRequestID *string) *CreateMinionParams {
	o.SetXRequestID(xRequestID)
	return o
}

// SetXRequestID adds the xRequestId to the create minion params
func (o *CreateMinionParams) SetXRequestID(xRequestID *string) {
	o.XRequestID = xRequestID
}

// WithBody adds the body to the create minion params
func (o *CreateMinionParams) WithBody(body *models.Minion) *CreateMinionParams {
	o.SetBody(body)
	return o
}

// SetBody adds the body to the create minion params
func (o *CreateMinionParams) SetBody(body *models.Minion) {
	o.Body = body
}

// WriteToRequest writes these params to a swagger request
func (o *CreateMinionParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	if o.Authorization != nil {

		// header param Authorization
		if err := r.SetHeaderParam("Authorization", *o.Authorization); err != nil {
			return err
		}
	}

	if o.XRequestID != nil {

		// header param X-Request-Id
		if err := r.SetHeaderParam("X-Request-Id", *o.XRequestID); err != nil {
			return err
		}
	}
	if o.Body != nil {
		if err := r.SetBodyParam(o.Body); err != nil {
			return err
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
