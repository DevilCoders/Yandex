// Code generated by go-swagger; DO NOT EDIT.

package downtimes

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

// NewV2DowntimesSetDowntimesParams creates a new V2DowntimesSetDowntimesParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewV2DowntimesSetDowntimesParams() *V2DowntimesSetDowntimesParams {
	return &V2DowntimesSetDowntimesParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewV2DowntimesSetDowntimesParamsWithTimeout creates a new V2DowntimesSetDowntimesParams object
// with the ability to set a timeout on a request.
func NewV2DowntimesSetDowntimesParamsWithTimeout(timeout time.Duration) *V2DowntimesSetDowntimesParams {
	return &V2DowntimesSetDowntimesParams{
		timeout: timeout,
	}
}

// NewV2DowntimesSetDowntimesParamsWithContext creates a new V2DowntimesSetDowntimesParams object
// with the ability to set a context for a request.
func NewV2DowntimesSetDowntimesParamsWithContext(ctx context.Context) *V2DowntimesSetDowntimesParams {
	return &V2DowntimesSetDowntimesParams{
		Context: ctx,
	}
}

// NewV2DowntimesSetDowntimesParamsWithHTTPClient creates a new V2DowntimesSetDowntimesParams object
// with the ability to set a custom HTTPClient for a request.
func NewV2DowntimesSetDowntimesParamsWithHTTPClient(client *http.Client) *V2DowntimesSetDowntimesParams {
	return &V2DowntimesSetDowntimesParams{
		HTTPClient: client,
	}
}

/* V2DowntimesSetDowntimesParams contains all the parameters to send to the API endpoint
   for the v2 downtimes set downtimes operation.

   Typically these are written to a http.Request.
*/
type V2DowntimesSetDowntimesParams struct {

	/* Authorization.

	   OAuth token. It is not in security section because we also use cookies and you can't specify those in swagger 2.0.
	*/
	Authorization *string

	// SetDowntimeRequest.
	SetDowntimeRequest *models.V2DowntimesSetDowntimesParamsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the v2 downtimes set downtimes params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2DowntimesSetDowntimesParams) WithDefaults() *V2DowntimesSetDowntimesParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the v2 downtimes set downtimes params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2DowntimesSetDowntimesParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) WithTimeout(timeout time.Duration) *V2DowntimesSetDowntimesParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) WithContext(ctx context.Context) *V2DowntimesSetDowntimesParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) WithHTTPClient(client *http.Client) *V2DowntimesSetDowntimesParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithAuthorization adds the authorization to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) WithAuthorization(authorization *string) *V2DowntimesSetDowntimesParams {
	o.SetAuthorization(authorization)
	return o
}

// SetAuthorization adds the authorization to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) SetAuthorization(authorization *string) {
	o.Authorization = authorization
}

// WithSetDowntimeRequest adds the setDowntimeRequest to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) WithSetDowntimeRequest(setDowntimeRequest *models.V2DowntimesSetDowntimesParamsBody) *V2DowntimesSetDowntimesParams {
	o.SetSetDowntimeRequest(setDowntimeRequest)
	return o
}

// SetSetDowntimeRequest adds the setDowntimeRequest to the v2 downtimes set downtimes params
func (o *V2DowntimesSetDowntimesParams) SetSetDowntimeRequest(setDowntimeRequest *models.V2DowntimesSetDowntimesParamsBody) {
	o.SetDowntimeRequest = setDowntimeRequest
}

// WriteToRequest writes these params to a swagger request
func (o *V2DowntimesSetDowntimesParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

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
	if o.SetDowntimeRequest != nil {
		if err := r.SetBodyParam(o.SetDowntimeRequest); err != nil {
			return err
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
