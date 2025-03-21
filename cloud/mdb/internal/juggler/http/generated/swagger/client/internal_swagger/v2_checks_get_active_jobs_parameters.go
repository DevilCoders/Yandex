// Code generated by go-swagger; DO NOT EDIT.

package internal_swagger

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

// NewV2ChecksGetActiveJobsParams creates a new V2ChecksGetActiveJobsParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewV2ChecksGetActiveJobsParams() *V2ChecksGetActiveJobsParams {
	return &V2ChecksGetActiveJobsParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewV2ChecksGetActiveJobsParamsWithTimeout creates a new V2ChecksGetActiveJobsParams object
// with the ability to set a timeout on a request.
func NewV2ChecksGetActiveJobsParamsWithTimeout(timeout time.Duration) *V2ChecksGetActiveJobsParams {
	return &V2ChecksGetActiveJobsParams{
		timeout: timeout,
	}
}

// NewV2ChecksGetActiveJobsParamsWithContext creates a new V2ChecksGetActiveJobsParams object
// with the ability to set a context for a request.
func NewV2ChecksGetActiveJobsParamsWithContext(ctx context.Context) *V2ChecksGetActiveJobsParams {
	return &V2ChecksGetActiveJobsParams{
		Context: ctx,
	}
}

// NewV2ChecksGetActiveJobsParamsWithHTTPClient creates a new V2ChecksGetActiveJobsParams object
// with the ability to set a custom HTTPClient for a request.
func NewV2ChecksGetActiveJobsParamsWithHTTPClient(client *http.Client) *V2ChecksGetActiveJobsParams {
	return &V2ChecksGetActiveJobsParams{
		HTTPClient: client,
	}
}

/* V2ChecksGetActiveJobsParams contains all the parameters to send to the API endpoint
   for the v2 checks get active jobs operation.

   Typically these are written to a http.Request.
*/
type V2ChecksGetActiveJobsParams struct {

	// GetActiveJobsRequest.
	GetActiveJobsRequest *models.V2ChecksGetActiveJobsParamsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the v2 checks get active jobs params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2ChecksGetActiveJobsParams) WithDefaults() *V2ChecksGetActiveJobsParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the v2 checks get active jobs params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2ChecksGetActiveJobsParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) WithTimeout(timeout time.Duration) *V2ChecksGetActiveJobsParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) WithContext(ctx context.Context) *V2ChecksGetActiveJobsParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) WithHTTPClient(client *http.Client) *V2ChecksGetActiveJobsParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithGetActiveJobsRequest adds the getActiveJobsRequest to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) WithGetActiveJobsRequest(getActiveJobsRequest *models.V2ChecksGetActiveJobsParamsBody) *V2ChecksGetActiveJobsParams {
	o.SetGetActiveJobsRequest(getActiveJobsRequest)
	return o
}

// SetGetActiveJobsRequest adds the getActiveJobsRequest to the v2 checks get active jobs params
func (o *V2ChecksGetActiveJobsParams) SetGetActiveJobsRequest(getActiveJobsRequest *models.V2ChecksGetActiveJobsParamsBody) {
	o.GetActiveJobsRequest = getActiveJobsRequest
}

// WriteToRequest writes these params to a swagger request
func (o *V2ChecksGetActiveJobsParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error
	if o.GetActiveJobsRequest != nil {
		if err := r.SetBodyParam(o.GetActiveJobsRequest); err != nil {
			return err
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
