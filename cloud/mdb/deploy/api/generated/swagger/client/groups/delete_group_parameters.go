// Code generated by go-swagger; DO NOT EDIT.

package groups

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

// NewDeleteGroupParams creates a new DeleteGroupParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewDeleteGroupParams() *DeleteGroupParams {
	return &DeleteGroupParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewDeleteGroupParamsWithTimeout creates a new DeleteGroupParams object
// with the ability to set a timeout on a request.
func NewDeleteGroupParamsWithTimeout(timeout time.Duration) *DeleteGroupParams {
	return &DeleteGroupParams{
		timeout: timeout,
	}
}

// NewDeleteGroupParamsWithContext creates a new DeleteGroupParams object
// with the ability to set a context for a request.
func NewDeleteGroupParamsWithContext(ctx context.Context) *DeleteGroupParams {
	return &DeleteGroupParams{
		Context: ctx,
	}
}

// NewDeleteGroupParamsWithHTTPClient creates a new DeleteGroupParams object
// with the ability to set a custom HTTPClient for a request.
func NewDeleteGroupParamsWithHTTPClient(client *http.Client) *DeleteGroupParams {
	return &DeleteGroupParams{
		HTTPClient: client,
	}
}

/* DeleteGroupParams contains all the parameters to send to the API endpoint
   for the delete group operation.

   Typically these are written to a http.Request.
*/
type DeleteGroupParams struct {

	/* Authorization.

	   OAuth token. It is not in security section because we also use cookies and you can't specify those in swagger 2.0.
	*/
	Authorization *string

	/* XRequestID.

	   Unique request ID (must be generated for each separate request, even retries)
	*/
	XRequestID *string

	/* Groupname.

	   Group's name
	*/
	Groupname string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the delete group params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *DeleteGroupParams) WithDefaults() *DeleteGroupParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the delete group params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *DeleteGroupParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the delete group params
func (o *DeleteGroupParams) WithTimeout(timeout time.Duration) *DeleteGroupParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the delete group params
func (o *DeleteGroupParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the delete group params
func (o *DeleteGroupParams) WithContext(ctx context.Context) *DeleteGroupParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the delete group params
func (o *DeleteGroupParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the delete group params
func (o *DeleteGroupParams) WithHTTPClient(client *http.Client) *DeleteGroupParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the delete group params
func (o *DeleteGroupParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithAuthorization adds the authorization to the delete group params
func (o *DeleteGroupParams) WithAuthorization(authorization *string) *DeleteGroupParams {
	o.SetAuthorization(authorization)
	return o
}

// SetAuthorization adds the authorization to the delete group params
func (o *DeleteGroupParams) SetAuthorization(authorization *string) {
	o.Authorization = authorization
}

// WithXRequestID adds the xRequestID to the delete group params
func (o *DeleteGroupParams) WithXRequestID(xRequestID *string) *DeleteGroupParams {
	o.SetXRequestID(xRequestID)
	return o
}

// SetXRequestID adds the xRequestId to the delete group params
func (o *DeleteGroupParams) SetXRequestID(xRequestID *string) {
	o.XRequestID = xRequestID
}

// WithGroupname adds the groupname to the delete group params
func (o *DeleteGroupParams) WithGroupname(groupname string) *DeleteGroupParams {
	o.SetGroupname(groupname)
	return o
}

// SetGroupname adds the groupname to the delete group params
func (o *DeleteGroupParams) SetGroupname(groupname string) {
	o.Groupname = groupname
}

// WriteToRequest writes these params to a swagger request
func (o *DeleteGroupParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

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

	// path param groupname
	if err := r.SetPathParam("groupname", o.Groupname); err != nil {
		return err
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
