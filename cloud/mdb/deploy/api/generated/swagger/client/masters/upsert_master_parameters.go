// Code generated by go-swagger; DO NOT EDIT.

package masters

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

// NewUpsertMasterParams creates a new UpsertMasterParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewUpsertMasterParams() *UpsertMasterParams {
	return &UpsertMasterParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewUpsertMasterParamsWithTimeout creates a new UpsertMasterParams object
// with the ability to set a timeout on a request.
func NewUpsertMasterParamsWithTimeout(timeout time.Duration) *UpsertMasterParams {
	return &UpsertMasterParams{
		timeout: timeout,
	}
}

// NewUpsertMasterParamsWithContext creates a new UpsertMasterParams object
// with the ability to set a context for a request.
func NewUpsertMasterParamsWithContext(ctx context.Context) *UpsertMasterParams {
	return &UpsertMasterParams{
		Context: ctx,
	}
}

// NewUpsertMasterParamsWithHTTPClient creates a new UpsertMasterParams object
// with the ability to set a custom HTTPClient for a request.
func NewUpsertMasterParamsWithHTTPClient(client *http.Client) *UpsertMasterParams {
	return &UpsertMasterParams{
		HTTPClient: client,
	}
}

/* UpsertMasterParams contains all the parameters to send to the API endpoint
   for the upsert master operation.

   Typically these are written to a http.Request.
*/
type UpsertMasterParams struct {

	/* Authorization.

	   OAuth token. It is not in security section because we also use cookies and you can't specify those in swagger 2.0.
	*/
	Authorization *string

	/* XRequestID.

	   Unique request ID (must be generated for each separate request, even retries)
	*/
	XRequestID *string

	// Body.
	Body *models.Master

	/* Fqdn.

	   fqdn of whatever
	*/
	Fqdn string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the upsert master params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *UpsertMasterParams) WithDefaults() *UpsertMasterParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the upsert master params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *UpsertMasterParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the upsert master params
func (o *UpsertMasterParams) WithTimeout(timeout time.Duration) *UpsertMasterParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the upsert master params
func (o *UpsertMasterParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the upsert master params
func (o *UpsertMasterParams) WithContext(ctx context.Context) *UpsertMasterParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the upsert master params
func (o *UpsertMasterParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the upsert master params
func (o *UpsertMasterParams) WithHTTPClient(client *http.Client) *UpsertMasterParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the upsert master params
func (o *UpsertMasterParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithAuthorization adds the authorization to the upsert master params
func (o *UpsertMasterParams) WithAuthorization(authorization *string) *UpsertMasterParams {
	o.SetAuthorization(authorization)
	return o
}

// SetAuthorization adds the authorization to the upsert master params
func (o *UpsertMasterParams) SetAuthorization(authorization *string) {
	o.Authorization = authorization
}

// WithXRequestID adds the xRequestID to the upsert master params
func (o *UpsertMasterParams) WithXRequestID(xRequestID *string) *UpsertMasterParams {
	o.SetXRequestID(xRequestID)
	return o
}

// SetXRequestID adds the xRequestId to the upsert master params
func (o *UpsertMasterParams) SetXRequestID(xRequestID *string) {
	o.XRequestID = xRequestID
}

// WithBody adds the body to the upsert master params
func (o *UpsertMasterParams) WithBody(body *models.Master) *UpsertMasterParams {
	o.SetBody(body)
	return o
}

// SetBody adds the body to the upsert master params
func (o *UpsertMasterParams) SetBody(body *models.Master) {
	o.Body = body
}

// WithFqdn adds the fqdn to the upsert master params
func (o *UpsertMasterParams) WithFqdn(fqdn string) *UpsertMasterParams {
	o.SetFqdn(fqdn)
	return o
}

// SetFqdn adds the fqdn to the upsert master params
func (o *UpsertMasterParams) SetFqdn(fqdn string) {
	o.Fqdn = fqdn
}

// WriteToRequest writes these params to a swagger request
func (o *UpsertMasterParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

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

	// path param fqdn
	if err := r.SetPathParam("fqdn", o.Fqdn); err != nil {
		return err
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
