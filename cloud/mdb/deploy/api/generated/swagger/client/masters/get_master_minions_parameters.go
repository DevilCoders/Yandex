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
	"github.com/go-openapi/swag"
)

// NewGetMasterMinionsParams creates a new GetMasterMinionsParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewGetMasterMinionsParams() *GetMasterMinionsParams {
	return &GetMasterMinionsParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewGetMasterMinionsParamsWithTimeout creates a new GetMasterMinionsParams object
// with the ability to set a timeout on a request.
func NewGetMasterMinionsParamsWithTimeout(timeout time.Duration) *GetMasterMinionsParams {
	return &GetMasterMinionsParams{
		timeout: timeout,
	}
}

// NewGetMasterMinionsParamsWithContext creates a new GetMasterMinionsParams object
// with the ability to set a context for a request.
func NewGetMasterMinionsParamsWithContext(ctx context.Context) *GetMasterMinionsParams {
	return &GetMasterMinionsParams{
		Context: ctx,
	}
}

// NewGetMasterMinionsParamsWithHTTPClient creates a new GetMasterMinionsParams object
// with the ability to set a custom HTTPClient for a request.
func NewGetMasterMinionsParamsWithHTTPClient(client *http.Client) *GetMasterMinionsParams {
	return &GetMasterMinionsParams{
		HTTPClient: client,
	}
}

/* GetMasterMinionsParams contains all the parameters to send to the API endpoint
   for the get master minions operation.

   Typically these are written to a http.Request.
*/
type GetMasterMinionsParams struct {

	/* Authorization.

	   OAuth token. It is not in security section because we also use cookies and you can't specify those in swagger 2.0.
	*/
	Authorization *string

	/* XRequestID.

	   Unique request ID (must be generated for each separate request, even retries)
	*/
	XRequestID *string

	/* Fqdn.

	   fqdn of whatever
	*/
	Fqdn string

	/* PageSize.

	   Number of results per page

	   Format: int64
	   Default: 100
	*/
	PageSize *int64

	/* PageToken.

	   Token to request the next page in listing
	*/
	PageToken *string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the get master minions params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *GetMasterMinionsParams) WithDefaults() *GetMasterMinionsParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the get master minions params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *GetMasterMinionsParams) SetDefaults() {
	var (
		pageSizeDefault = int64(100)
	)

	val := GetMasterMinionsParams{
		PageSize: &pageSizeDefault,
	}

	val.timeout = o.timeout
	val.Context = o.Context
	val.HTTPClient = o.HTTPClient
	*o = val
}

// WithTimeout adds the timeout to the get master minions params
func (o *GetMasterMinionsParams) WithTimeout(timeout time.Duration) *GetMasterMinionsParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the get master minions params
func (o *GetMasterMinionsParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the get master minions params
func (o *GetMasterMinionsParams) WithContext(ctx context.Context) *GetMasterMinionsParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the get master minions params
func (o *GetMasterMinionsParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the get master minions params
func (o *GetMasterMinionsParams) WithHTTPClient(client *http.Client) *GetMasterMinionsParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the get master minions params
func (o *GetMasterMinionsParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithAuthorization adds the authorization to the get master minions params
func (o *GetMasterMinionsParams) WithAuthorization(authorization *string) *GetMasterMinionsParams {
	o.SetAuthorization(authorization)
	return o
}

// SetAuthorization adds the authorization to the get master minions params
func (o *GetMasterMinionsParams) SetAuthorization(authorization *string) {
	o.Authorization = authorization
}

// WithXRequestID adds the xRequestID to the get master minions params
func (o *GetMasterMinionsParams) WithXRequestID(xRequestID *string) *GetMasterMinionsParams {
	o.SetXRequestID(xRequestID)
	return o
}

// SetXRequestID adds the xRequestId to the get master minions params
func (o *GetMasterMinionsParams) SetXRequestID(xRequestID *string) {
	o.XRequestID = xRequestID
}

// WithFqdn adds the fqdn to the get master minions params
func (o *GetMasterMinionsParams) WithFqdn(fqdn string) *GetMasterMinionsParams {
	o.SetFqdn(fqdn)
	return o
}

// SetFqdn adds the fqdn to the get master minions params
func (o *GetMasterMinionsParams) SetFqdn(fqdn string) {
	o.Fqdn = fqdn
}

// WithPageSize adds the pageSize to the get master minions params
func (o *GetMasterMinionsParams) WithPageSize(pageSize *int64) *GetMasterMinionsParams {
	o.SetPageSize(pageSize)
	return o
}

// SetPageSize adds the pageSize to the get master minions params
func (o *GetMasterMinionsParams) SetPageSize(pageSize *int64) {
	o.PageSize = pageSize
}

// WithPageToken adds the pageToken to the get master minions params
func (o *GetMasterMinionsParams) WithPageToken(pageToken *string) *GetMasterMinionsParams {
	o.SetPageToken(pageToken)
	return o
}

// SetPageToken adds the pageToken to the get master minions params
func (o *GetMasterMinionsParams) SetPageToken(pageToken *string) {
	o.PageToken = pageToken
}

// WriteToRequest writes these params to a swagger request
func (o *GetMasterMinionsParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

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

	// path param fqdn
	if err := r.SetPathParam("fqdn", o.Fqdn); err != nil {
		return err
	}

	if o.PageSize != nil {

		// query param pageSize
		var qrPageSize int64

		if o.PageSize != nil {
			qrPageSize = *o.PageSize
		}
		qPageSize := swag.FormatInt64(qrPageSize)
		if qPageSize != "" {

			if err := r.SetQueryParam("pageSize", qPageSize); err != nil {
				return err
			}
		}
	}

	if o.PageToken != nil {

		// query param pageToken
		var qrPageToken string

		if o.PageToken != nil {
			qrPageToken = *o.PageToken
		}
		qPageToken := qrPageToken
		if qPageToken != "" {

			if err := r.SetQueryParam("pageToken", qPageToken); err != nil {
				return err
			}
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
